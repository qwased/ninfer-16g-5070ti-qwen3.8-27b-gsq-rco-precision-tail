#pragma once

// ninfer::ops::kvarn - exact-tail partial for the KVarN body (WP6, route (a)).
//
// The newest `tail_tokens` keys of a window are kept unquantized in the shared exact tail ring
// (`PagedKVExactTailView`: 64-token pages, page of position p = (p / 64) % ring_pages, no block
// table). This kernel produces the partial (acc, m, l) of exactly those keys and writes it at the
// split indices [body_active, total_active) that the quantized body leaves free; the body's own
// reduce (`kvarn::detail::reduce_output_hadamard_kernel`) then merges body and tail with one
// online-softmax pass, which is the merge itself -- there is no separate merge kernel.
//
// Domain. The body accumulates against rotated K/V and its reduce applies the inverse rotation
// once at the end, so its partial_acc holds sum p * W(V) (W = the D256 Sylvester-Hadamard,
// orthonormal and self-inverse). Two facts make the tail cheap and exact:
//   * the scores are a rotation invariant, <W q, W k> == <q, k>, so the tail may evaluate q . k on
//     the *original* rows the ring stores -- provided it holds an *original* query. The Op rotates
//     the query in place before this kernel runs (the body reads the rotated query), so the kernel
//     un-rotates it first, which is one warp-local W per row;
//   * W is linear, so W(sum p V) == sum p W(V) -- the tail accumulates in the original domain and
//     applies W to its FP32 accumulator exactly once per row (the "acc rotated once" implementation
//     of the plan; m and l are scalars and are unaffected by the domain).
//
// The body partial covers keys [0, body_window) in splits [0, body_active); the tail covers
// [body_window, window) in splits [body_active, total_active). The ranges are disjoint and
// adjacent, so no key is counted twice and none is dropped, and no split straddles the boundary.
//
// `tail_tokens <= 0` (or an absent ring) leaves the partition the identity -- body_window == window,
// body_active == total_active, tail_active == 0 -- so a launch without a tail never runs this
// kernel's key loop and the body path is unchanged.

#include "ops/common/kv_tail_element.cuh"
#include "ops/common/math.h"
#include "ops/common/warp.cuh"
#include "ops/kernel/paged_kv_address.cuh"
#include "ops/kvarn/config.cuh"
#include "ops/kvarn/decode_kernel.cuh"
#include "ops/kvarn/hadamard.cuh"
#include "ops/softmax_attention/dense/causal_cache/small_t.cuh"

#include <cuda_bf16.h>
#include <cuda_fp16.h>
#include <math_constants.h>

#include <cstdint>

namespace ninfer::ops::kvarn {

// One warp owns one (query head, token) row of the tail partial: it holds the whole D256 row of the
// query and of the accumulator in registers (8 values per lane, the `d = lane + 32r` layout the
// in-register Hadamard wants), so the rotation is a warp-local operation with no staged tile.
inline constexpr int kExactTailWarps = 8;
inline constexpr int kExactTailTile  = 32;

// `KvarnExactTailPartition` and `kvarn_exact_tail_partition` live in `decode_kernel.cuh`: the body
// kernel shares them so the body, the tail and the reducer agree on one partition.

template <typename Elem>
__device__ __forceinline__ float kvarn_exact_tail_to_float(Elem value);
template <>
__device__ __forceinline__ float kvarn_exact_tail_to_float<__nv_bfloat16>(__nv_bfloat16 value) {
    return __bfloat162float(value);
}
template <>
__device__ __forceinline__ float kvarn_exact_tail_to_float<__half>(__half value) {
    return __half2float(value);
}

// `positions` are the absolute positions of the launch's columns; `full_width` is the stride between
// sequences and `column_begin` the launch's first column, exactly as in the body and small-T tail
// launches. `partial_*` hold the whole launch's splits (grid.y == split_count): the tail writes only
// [body_active + split_local) for the split_indices it owns.
template <typename Geometry, typename Elem>
__launch_bounds__(kExactTailWarps * 32) __global__ void kvarn_exact_tail_partial_kernel(
    const __nv_bfloat16* __restrict__ q, const std::int32_t* __restrict__ positions,
    const Elem* __restrict__ tail_k, const Elem* __restrict__ tail_v, std::int32_t ring_pages,
    std::int32_t tail_tokens, std::int32_t tokens, std::int32_t full_width,
    std::int32_t column_begin, std::int32_t logical_capacity, std::int32_t batch_size,
    const std::int32_t* __restrict__ valid_columns, float scale, float* __restrict__ partial_acc,
    float* __restrict__ partial_m, float* __restrict__ partial_l) {
    constexpr int D       = ops::kvarn::D;
    constexpr int Threads = kExactTailWarps * 32;
    constexpr float Log2E = 1.4426950408889634074f;

    const int kv_head     = static_cast<int>(blockIdx.x);
    const int split_local = static_cast<int>(blockIdx.y);
    const int batch       = static_cast<int>(blockIdx.z);
    const int split_count = static_cast<int>(gridDim.y);
    const int tid         = static_cast<int>(threadIdx.x);
    const int warp        = tid >> 5;
    const int lane        = tid & 31;

    if (kv_head < 0 || kv_head >= Geometry::KVHeads || ring_pages <= 0 || tail_tokens <= 0 ||
        tokens < 1 || batch < 0 || batch >= batch_size || split_count <= 0) {
        return;
    }
    int valid_tokens = tokens;
    if (valid_columns != nullptr) {
        const int remaining = valid_columns[batch] - column_begin;
        valid_tokens        = remaining <= 0 ? 0 : (remaining < tokens ? remaining : tokens);
    }
    if (valid_tokens <= 0) { return; }

    const std::int64_t column_base =
        static_cast<std::int64_t>(column_begin) + static_cast<std::int64_t>(batch) * full_width;
    // A batched launch stacks its sequences along the split axis, the same offset the body kernel
    // applies; batch 0 adds nothing.
    partial_acc += static_cast<std::int64_t>(batch) * D * Geometry::QHeads * tokens * split_count;
    partial_m += static_cast<std::int64_t>(batch) * Geometry::QHeads * tokens * split_count;
    partial_l += static_cast<std::int64_t>(batch) * Geometry::QHeads * tokens * split_count;
    const std::int32_t last_pos = positions[column_base + valid_tokens - 1];
    if (last_pos < 0 || last_pos >= logical_capacity) { return; }
    const int window = last_pos + 1;

    const KvarnExactTailPartition partition =
        kvarn_exact_tail_partition<Geometry>(window, tail_tokens, split_count);
    const int tail_active = partition.tail_active;
    if (split_local >= tail_active) { return; }
    const int split = partition.body_active + split_local;

    const int row_count = tokens * Geometry::GroupSize;

    // A tail split this launch owns but that no key falls into must still publish a neutral partial
    // (the reducer reads every split in [body_active, total_active)). The body's neutral fill covers
    // only its own range, so the tail publishes its own.
    auto write_neutral = [&]() {
        for (int row = warp; row < row_count; row += kExactTailWarps) {
            int q_head = 0, token = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
            if (!causal_valid_q_head<Geometry>(kv_head, q_head)) { continue; }
            partial_m[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] =
                -CUDART_INF_F;
            partial_l[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = 0.0f;
        }
        for (int index = tid; index < row_count * D; index += Threads) {
            const int row = index / D;
            const int d   = index - row * D;
            int q_head = 0, token = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
            if (!causal_valid_q_head<Geometry>(kv_head, q_head)) { continue; }
            partial_acc[causal_partial_acc_index<Geometry>(q_head, d, token, split, tokens)] = 0.0f;
        }
    };

    const int tail_keys     = window - partition.body_window;
    const int logical_tiles = div_up(tail_keys, kExactTailTile);
    const bool tile_split   = logical_tiles >= tail_active;
    const int units_per_split =
        tile_split ? div_up(logical_tiles, tail_active) : div_up(tail_keys, tail_active);
    const int split_start = split_local * units_per_split * (tile_split ? kExactTailTile : 1);
    const int split_limit = split_start + units_per_split * (tile_split ? kExactTailTile : 1);
    const int split_end   = split_limit < tail_keys ? split_limit : tail_keys;
    if (split_start >= split_end) {
        write_neutral();
        return;
    }
    const int first_key = partition.body_window + split_start;
    const int limit_key = partition.body_window + split_end;

    for (int row = warp; row < row_count; row += kExactTailWarps) {
        int q_head = 0, token = 0;
        causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
        if (!causal_valid_q_head<Geometry>(kv_head, q_head)) { continue; }
        const int qabs = positions[column_base + token];

        float qv[D / 32];
#pragma unroll
        for (int r = 0; r < D / 32; ++r) {
            qv[r] = __bfloat162float(
                q[static_cast<std::int64_t>(D) * Geometry::QHeads * (column_base + token) +
                  causal_q_index<Geometry>(q_head, lane + 32 * r)]);
        }
        // The body rotates the query in place before this kernel is launched (it reads the rotated
        // query against the rotated K it stores), while the ring holds the *original* rows. W is
        // self-inverse and linear, so un-rotating the query here restores `dot(q, k_original)`
        // exactly, and it leaves the accumulator in the original domain that the single W at the end
        // of this row expects.
        detail::hadamard_warp(qv, lane);

        float m = -CUDART_INF_F;
        float l = 0.0f;
        float acc[D / 32];
#pragma unroll
        for (int r = 0; r < D / 32; ++r) { acc[r] = 0.0f; }

        for (int key = first_key; key < limit_key && key <= qabs; ++key) {
            const int ring        = batch * ring_pages + ((key >> kPagedKVPageShift) % ring_pages);
            const int page_offset = key & kPagedKVPageMask;
            const std::int64_t row_off =
                paged_kv_element_offset<D, Geometry::KVHeads>(ring, kv_head, page_offset, 0);
            float dot = 0.0f;
#pragma unroll
            for (int r = 0; r < D / 32; ++r) {
                dot += qv[r] * kvarn_exact_tail_to_float<Elem>(tail_k[row_off + lane + 32 * r]);
            }
            const float score = warp_sum<32>(dot) * scale;
            const float nm    = fmaxf(m, score);
            const float alpha = m == -CUDART_INF_F ? 0.0f : exp2_approx((m - nm) * Log2E);
            const float p     = (nm > -CUDART_INF_F && score > -CUDART_INF_F)
                                    ? exp2_approx((score - nm) * Log2E)
                                    : 0.0f;
            l = l * alpha + p;
            m = nm;
#pragma unroll
            for (int r = 0; r < D / 32; ++r) {
                acc[r] = acc[r] * alpha +
                         p * kvarn_exact_tail_to_float<Elem>(tail_v[row_off + lane + 32 * r]);
            }
        }
        if (m == -CUDART_INF_F) {
            // No key in this split is causal for this row (possible for the widest rows of a
            // speculative block). Publish a neutral partial rather than a zero accumulator.
            l = 0.0f;
#pragma unroll
            for (int r = 0; r < D / 32; ++r) { acc[r] = 0.0f; }
        }
        // W applied exactly once: acc <- W(acc_orig) == sum p W(V).
        detail::hadamard_warp(acc, lane);
#pragma unroll
        for (int r = 0; r < D / 32; ++r) {
            partial_acc[causal_partial_acc_index<Geometry>(q_head, lane + 32 * r, token, split,
                                                           tokens)] = acc[r];
        }
        if (lane == 0) {
            partial_m[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = m;
            partial_l[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = l;
        }
    }
}

} // namespace ninfer::ops::kvarn
