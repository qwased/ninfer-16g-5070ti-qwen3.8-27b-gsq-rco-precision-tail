#pragma once

// ninfer::ops - split-KV causal small-T attention, exact-tail partial kernel (WP3).
//
// The newest `tail_tokens` keys of a window are kept unquantized in the exact BF16 ring (one
// contiguous ring per sequence, page of position p = (p / 64) % ring_pages, no block table). This
// kernel produces the partial (acc, m, l) of exactly those keys and writes it at the split indices
// [body_active, total_active) the quantized body partial leaves free; the shared reducer then
// merges body and tail with one online-softmax pass, which is the merge itself -- no separate
// merge kernel exists.
//
// It is storage-independent: whatever the body's coding is (INT8 family, fp8, nvfp4, k8v4, bf16),
// the tail is always read as BF16, so the merge unifies the body's scaled-in-code domain with the
// exact domain through (m, l) only. `Int8` and `wave_splits` mirror the body kernel's active-split
// policy so the two sides agree on where the body ends and the tail begins.
//
// The body partial covers keys [0, body_window) in splits [0, body_active); the tail covers
// [body_window, window) in splits [body_active, total_active). The ranges are disjoint and
// adjacent, so no key is counted twice and none is dropped, and a split never straddles the
// boundary.

#include <cuda_bf16.h>
#include <math_constants.h>

#include "ops/softmax_attention/dense/causal_cache/small_t.cuh"

#include <cstdint>

namespace ninfer::ops {

// Row-tile width of the tail kernel: enough 16-row tiles for one token tile's rows and no more.
// Mirrors the BF16 body's own choice (two warps for a single token, four above it), and stays
// independent of the body kernel's producer geometry.
template <int TokenTile>
inline constexpr int kCausalSmallTTailWarps = TokenTile == 1 ? 2 : 4;

template <typename Geometry, int TokenTile, int WarpsPerCta, bool Int8>
__launch_bounds__(WarpsPerCta * 32, 2) __global__ void causal_attention_small_t_tail_bf16_kernel(
    const __nv_bfloat16* q, const std::int32_t* pos, const __nv_bfloat16* tail_k,
    const __nv_bfloat16* tail_v, std::int32_t ring_pages, std::int32_t tail_tokens,
    std::int32_t wave_splits, std::int32_t tokens, std::int32_t full_width,
    std::int32_t column_begin, std::int32_t logical_capacity, std::int32_t batch_size,
    const std::int32_t* valid_columns, float scale, float* partial_acc, float* partial_m,
    float* partial_l) {
    static_assert(TokenTile >= 1 && TokenTile * Geometry::GroupSize <= 48);
    static_assert(WarpsPerCta >= 1 && WarpsPerCta <= 4);

    constexpr int Wc      = WarpsPerCta;
    constexpr int Br      = Wc * 16;
    constexpr int Bc      = 32;
    constexpr int D       = kCausalHeadDim;
    constexpr int Threads = Wc * 32;
    constexpr int QKNt    = Bc / 8;
    constexpr int QKKs    = D / 16;
    constexpr int PVNt    = D / 8;
    constexpr int PVKs    = Bc / 16;
    constexpr float Log2E       = 1.4426950408889634074f;
    constexpr unsigned FullMask = 0xffffffffu;
    constexpr int QkvRows       = 2 * Bc;

    static_assert(QkvRows >= Br);

    __shared__ __align__(16) __nv_bfloat16 qkv_s[QkvRows * D];
    __shared__ __align__(16) __nv_bfloat16 p_s[Wc * 16 * Bc];
    __nv_bfloat16* k_s = qkv_s;
    __nv_bfloat16* v_s = qkv_s + Bc * D;

    const int kv_head     = static_cast<int>(blockIdx.x);
    const int split_local = static_cast<int>(blockIdx.y);
    const int batch       = static_cast<int>(blockIdx.z);
    const int split_count = static_cast<int>(gridDim.y);
    const int tid         = static_cast<int>(threadIdx.x);
    const int warp        = tid >> 5;
    const int lane        = tid & 31;
    if (kv_head < 0 || kv_head >= Geometry::KVHeads || ring_pages <= 0 || tail_tokens <= 0 ||
        tokens < 1 || tokens > TokenTile || batch < 0 || batch >= batch_size || split_count <= 0) {
        return;
    }
    int valid_tokens = tokens;
    if (valid_columns != nullptr) {
        const int remaining = valid_columns[batch] - column_begin;
        valid_tokens        = remaining <= 0 ? 0 : (remaining < tokens ? remaining : tokens);
    }
    const int row_count = tokens * Geometry::GroupSize;
    if (row_count > Br) { return; }

    const std::int64_t column_base =
        static_cast<std::int64_t>(column_begin) + static_cast<std::int64_t>(batch) * full_width;
    q += static_cast<std::int64_t>(D) * Geometry::QHeads * column_base;
    pos += column_base;
    partial_acc += static_cast<std::int64_t>(batch) * D * Geometry::QHeads * tokens * split_count;
    partial_m += static_cast<std::int64_t>(batch) * Geometry::QHeads * tokens * split_count;
    partial_l += static_cast<std::int64_t>(batch) * Geometry::QHeads * tokens * split_count;

    // The body kernel's neutral fill already covers every split of the launch (its early exits
    // happen before the split-ownership check), so the tail kernel must not write anything in the
    // same situations: it would only race the body's own writes.
    if (valid_tokens == 0) { return; }

    const std::int32_t last_pos = pos[tokens - 1];
    if (last_pos < 0 || last_pos >= logical_capacity) { return; }

    const int window = last_pos + 1;
    const CausalSmallTTailPartition partition = causal_small_t_tail_partition<Geometry, Int8>(
        window, tail_tokens, split_count, tokens, wave_splits);
    const int body_window = partition.body_window;
    const int tail_active = partition.tail_active;
    if (split_local >= tail_active) { return; }

    const int split = partition.body_active + split_local;
    auto write_neutral = [&]() {
        for (int row = tid; row < row_count; row += Threads) {
            int q_head = 0;
            int token  = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
            if (causal_valid_q_head<Geometry>(kv_head, q_head)) {
                partial_m[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] =
                    -CUDART_INF_F;
                partial_l[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = 0.0f;
            }
        }
        for (int idx = tid; idx < row_count * D; idx += Threads) {
            const int row = idx / D;
            const int d   = idx - row * D;
            int q_head    = 0;
            int token     = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
            if (causal_valid_q_head<Geometry>(kv_head, q_head)) {
                partial_acc[causal_partial_acc_index<Geometry>(q_head, d, token, split, tokens)] =
                    0.0f;
            }
        }
    };

    const int tail_keys     = window - body_window;
    const int logical_tiles = div_up(tail_keys, Bc);
    const bool tile_split   = logical_tiles >= tail_active;
    const int units_per_split =
        tile_split ? div_up(logical_tiles, tail_active) : div_up(tail_keys, tail_active);
    const int split_start = split_local * units_per_split * (tile_split ? Bc : 1);
    const int split_limit = split_start + units_per_split * (tile_split ? Bc : 1);
    const int split_end   = (split_limit < tail_keys) ? split_limit : tail_keys;
    // A split this launch owns but that no key falls into must still publish a neutral partial:
    // the reducer reads every split in [body_active, total_active). Splits past tail_active are
    // outside the reducer's range and are left untouched.
    if (split_start >= split_end) {
        write_neutral();
        return;
    }
    const int first_tile = (split_start / Bc) * Bc;
    const int key_blocks = div_up(split_end - first_tile, Bc);
    // Absolute key range this split reads: [first_key, limit_key) with first_key = body_window +
    // split_start. The ring page follows from the absolute key alone.
    const int first_key = body_window + split_start;
    const int limit_key = body_window + split_end;

    for (int idx = tid; idx < Br * D; idx += Threads) {
        const int row = idx / D;
        const int d   = idx - row * D;
        int q_head    = 0;
        int token     = 0;
        causal_small_t_tc_row_to_qt<Geometry>(row, tokens, kv_head, q_head, token);
        __nv_bfloat16 value = __float2bfloat16(0.0f);
        if (row < row_count && causal_valid_q_head<Geometry>(kv_head, q_head)) {
            value = q[causal_q_index<Geometry>(q_head, d, token)];
        }
        qkv_s[row * D + causal_small_t_tc_swz(row, d)] = value;
    }
    __syncthreads();

    const int gid = lane >> 2;
    const int lid = lane & 3;

    const int a_mat    = lane >> 3;
    const int a_rin    = lane & 7;
    const int a_rowoff = a_rin + ((a_mat & 1) << 3);
    const int a_coloff = (a_mat >> 1) << 3;
    const int b_rin    = lane & 7;
    const int b_koff   = ((lane >> 3) & 1) << 3;

    const int warp_row0 = warp * 16;
    __nv_bfloat16* p_sw = &p_s[warp * 16 * Bc];

    unsigned af_q[QKKs][4];
#pragma unroll
    for (int k = 0; k < QKKs; ++k) {
        const int arow = warp_row0 + a_rowoff;
        const int acol = k * 16 + a_coloff;
        ldmatrix_x4(af_q[k][0], af_q[k][1], af_q[k][2], af_q[k][3],
                    smem_addr(&qkv_s[arow * D + causal_small_t_tc_swz(arow, acol)]));
    }
    __syncthreads();
    float acc[PVNt][4];
#pragma unroll
    for (int n = 0; n < PVNt; ++n) {
#pragma unroll
        for (int i = 0; i < 4; ++i) { acc[n][i] = 0.0f; }
    }
    float m0 = -CUDART_INF_F, m1 = -CUDART_INF_F, l0 = 0.0f, l1 = 0.0f;

    for (int kb = 0; kb < key_blocks; ++kb) {
        const int tile_first_key = body_window + first_tile + kb * Bc;
        // Stage the exact K/V key tile; keys outside this split read as zero and are masked below.
#pragma unroll 1
        for (int chunk = tid; chunk < Bc * (D / 8); chunk += Threads) {
            const int key_l      = chunk / (D / 8);
            const int d          = (chunk - key_l * (D / 8)) * 8;
            const int key        = tile_first_key + key_l;
            __nv_bfloat16* k_dst = &k_s[key_l * D + causal_small_t_tc_swz(key_l, d)];
            __nv_bfloat16* v_dst = &v_s[key_l * D + causal_small_t_tc_swz(key_l, d)];
            if (key >= first_key && key < limit_key) {
                const int physical_page =
                    batch * ring_pages + ((key >> kPagedKVPageShift) % ring_pages);
                const std::int64_t off = causal_cache_index<Geometry>(
                    physical_page, kv_head, d, key & kPagedKVPageMask);
                ninfer::ops::cp_async<16>(k_dst, &tail_k[off]);
                ninfer::ops::cp_async<16>(v_dst, &tail_v[off]);
            } else {
                store_vec(k_dst, make_int4(0, 0, 0, 0));
                store_vec(v_dst, make_int4(0, 0, 0, 0));
            }
        }
        ninfer::ops::cp_commit();
        ninfer::ops::cp_wait<0>();
        __syncthreads();

        float score[QKNt][4];
#pragma unroll
        for (int nt = 0; nt < QKNt; ++nt) {
            score[nt][0] = score[nt][1] = score[nt][2] = score[nt][3] = 0.0f;
#pragma unroll
            for (int k = 0; k < QKKs; ++k) {
                unsigned bf[2];
                const int brow = nt * 8 + b_rin;
                const int bcol = k * 16 + b_koff;
                ldmatrix_x2(bf[0], bf[1],
                            smem_addr(&k_s[brow * D + causal_small_t_tc_swz(brow, bcol)]));
                mma_bf16(score[nt][0], score[nt][1], score[nt][2], score[nt][3], af_q[k][0],
                         af_q[k][1], af_q[k][2], af_q[k][3], bf[0], bf[1]);
            }
        }

        const int row0 = warp_row0 + gid;
        const int row1 = row0 + 8;
        int q_head0 = 0, token0 = 0, q_head1 = 0, token1 = 0;
        causal_small_t_tc_row_to_qt<Geometry>(row0, tokens, kv_head, q_head0, token0);
        causal_small_t_tc_row_to_qt<Geometry>(row1, tokens, kv_head, q_head1, token1);
        const int qabs0 = (row0 < row_count) ? pos[token0] : -1;
        const int qabs1 = (row1 < row_count) ? pos[token1] : -1;

        float bm0 = -CUDART_INF_F, bm1 = -CUDART_INF_F;
#pragma unroll
        for (int nt = 0; nt < QKNt; ++nt) {
            const int col0 = nt * 8 + 2 * lid;
            const int col1 = col0 + 1;
            const int key0 = tile_first_key + col0;
            const int key1 = tile_first_key + col1;
            score[nt][0] =
                (row0 < row_count && key0 >= first_key && key0 < limit_key && key0 <= qabs0)
                    ? score[nt][0] * scale
                    : -CUDART_INF_F;
            score[nt][1] =
                (row0 < row_count && key1 >= first_key && key1 < limit_key && key1 <= qabs0)
                    ? score[nt][1] * scale
                    : -CUDART_INF_F;
            score[nt][2] =
                (row1 < row_count && key0 >= first_key && key0 < limit_key && key0 <= qabs1)
                    ? score[nt][2] * scale
                    : -CUDART_INF_F;
            score[nt][3] =
                (row1 < row_count && key1 >= first_key && key1 < limit_key && key1 <= qabs1)
                    ? score[nt][3] * scale
                    : -CUDART_INF_F;
            bm0 = fmaxf(bm0, fmaxf(score[nt][0], score[nt][1]));
            bm1 = fmaxf(bm1, fmaxf(score[nt][2], score[nt][3]));
        }
        bm0 = warp_max<4>(bm0, FullMask);
        bm1 = warp_max<4>(bm1, FullMask);

        const float nm0    = fmaxf(m0, bm0);
        const float nm1    = fmaxf(m1, bm1);
        const float alpha0 = (m0 == -CUDART_INF_F) ? 0.0f : exp2_approx((m0 - nm0) * Log2E);
        const float alpha1 = (m1 == -CUDART_INF_F) ? 0.0f : exp2_approx((m1 - nm1) * Log2E);

        float bl0 = 0.0f, bl1 = 0.0f;
#pragma unroll
        for (int nt = 0; nt < QKNt; ++nt) {
            const int col0  = nt * 8 + 2 * lid;
            const int col1  = col0 + 1;
            const float p00 = (nm0 > -CUDART_INF_F && score[nt][0] > -CUDART_INF_F)
                                  ? exp2_approx((score[nt][0] - nm0) * Log2E)
                                  : 0.0f;
            const float p01 = (nm0 > -CUDART_INF_F && score[nt][1] > -CUDART_INF_F)
                                  ? exp2_approx((score[nt][1] - nm0) * Log2E)
                                  : 0.0f;
            const float p10 = (nm1 > -CUDART_INF_F && score[nt][2] > -CUDART_INF_F)
                                  ? exp2_approx((score[nt][2] - nm1) * Log2E)
                                  : 0.0f;
            const float p11 = (nm1 > -CUDART_INF_F && score[nt][3] > -CUDART_INF_F)
                                  ? exp2_approx((score[nt][3] - nm1) * Log2E)
                                  : 0.0f;
            bl0 += p00 + p01;
            bl1 += p10 + p11;
            p_sw[gid * Bc + causal_small_t_tc_swz32(gid, col0)]           = __float2bfloat16(p00);
            p_sw[gid * Bc + causal_small_t_tc_swz32(gid, col1)]           = __float2bfloat16(p01);
            p_sw[(gid + 8) * Bc + causal_small_t_tc_swz32(gid + 8, col0)] = __float2bfloat16(p10);
            p_sw[(gid + 8) * Bc + causal_small_t_tc_swz32(gid + 8, col1)] = __float2bfloat16(p11);
        }
        bl0 = warp_sum<4>(bl0, FullMask);
        bl1 = warp_sum<4>(bl1, FullMask);

        l0 = l0 * alpha0 + bl0;
        l1 = l1 * alpha1 + bl1;
        m0 = nm0;
        m1 = nm1;
#pragma unroll
        for (int n = 0; n < PVNt; ++n) {
            acc[n][0] *= alpha0;
            acc[n][1] *= alpha0;
            acc[n][2] *= alpha1;
            acc[n][3] *= alpha1;
        }
        __syncwarp();

#pragma unroll
        for (int n = 0; n < PVNt; ++n) {
#pragma unroll
            for (int k = 0; k < PVKs; ++k) {
                unsigned pf[4];
                const int pcol = k * 16 + a_coloff;
                ldmatrix_x4(
                    pf[0], pf[1], pf[2], pf[3],
                    smem_addr(&p_sw[a_rowoff * Bc + causal_small_t_tc_swz32(a_rowoff, pcol)]));
                unsigned vf[2];
                const int vrow = k * 16 + b_koff + b_rin;
                const int vcol = n * 8;
                ldmatrix_x2_t(vf[0], vf[1],
                              smem_addr(&v_s[vrow * D + causal_small_t_tc_swz(vrow, vcol)]));
                mma_bf16(acc[n][0], acc[n][1], acc[n][2], acc[n][3], pf[0], pf[1], pf[2], pf[3],
                         vf[0], vf[1]);
            }
        }
        __syncthreads();
    }

    if (lid == 0) {
        const int row0 = warp_row0 + gid;
        const int row1 = row0 + 8;
        if (row0 < row_count) {
            int q_head = 0;
            int token  = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row0, tokens, kv_head, q_head, token);
            partial_m[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = m0;
            partial_l[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = l0;
        }
        if (row1 < row_count) {
            int q_head = 0;
            int token  = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row1, tokens, kv_head, q_head, token);
            partial_m[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = m1;
            partial_l[causal_partial_stat_index<Geometry>(q_head, token, split, tokens)] = l1;
        }
    }

#pragma unroll
    for (int n = 0; n < PVNt; ++n) {
        const int d0   = n * 8 + 2 * lid;
        const int row0 = warp_row0 + gid;
        const int row1 = row0 + 8;
        if (row0 < row_count) {
            int q_head = 0;
            int token  = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row0, tokens, kv_head, q_head, token);
            const std::int64_t dst =
                causal_partial_acc_index<Geometry>(q_head, d0, token, split, tokens);
            *reinterpret_cast<float2*>(&partial_acc[dst]) = make_float2(acc[n][0], acc[n][1]);
        }
        if (row1 < row_count) {
            int q_head = 0;
            int token  = 0;
            causal_small_t_tc_row_to_qt<Geometry>(row1, tokens, kv_head, q_head, token);
            const std::int64_t dst =
                causal_partial_acc_index<Geometry>(q_head, d0, token, split, tokens);
            *reinterpret_cast<float2*>(&partial_acc[dst]) = make_float2(acc[n][2], acc[n][3]);
        }
    }
}

} // namespace ninfer::ops
