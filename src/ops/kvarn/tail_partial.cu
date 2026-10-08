#include "ops/kvarn/tail_partial.h"

#include "core/device.h"
#include "ops/common/kv_tail_element.cuh"
#include "ops/kvarn/tail_partial.cuh"

#include <cuda_bf16.h>

#include <stdexcept>

namespace ninfer::ops::kvarn {
namespace {

// One thread writes one eight-element vector of one (batch, token, head) row, so a warp covers a
// whole 256-wide K or V row's lane group without any staging.
template <typename Geometry, typename Elem>
__global__ void kvarn_exact_tail_stage_kernel(
    const __nv_bfloat16* __restrict__ k, const __nv_bfloat16* __restrict__ v,
    const std::int32_t* __restrict__ positions, const std::int32_t* __restrict__ valid_columns,
    Elem* __restrict__ tail_k, Elem* __restrict__ tail_v, std::int32_t ring_pages,
    std::int32_t tokens, std::int32_t full_width, std::int32_t batch_size) {
    constexpr int D       = ops::kvarn::D;
    constexpr int VecElem = 8;
    const int batch       = static_cast<int>(blockIdx.y);
    if (batch >= batch_size) { return; }
    int valid = tokens;
    if (valid_columns != nullptr) {
        const int remaining = valid_columns[batch];
        valid               = remaining <= 0 ? 0 : (remaining < tokens ? remaining : tokens);
    }
    const std::int64_t n = static_cast<std::int64_t>(valid) * Geometry::KVHeads * (D / VecElem);
    const std::int64_t idx = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (idx >= n || ring_pages <= 0) { return; }

    const int vec   = static_cast<int>(idx % (D / VecElem));
    const int tmp   = static_cast<int>(idx / (D / VecElem));
    const int head  = tmp % Geometry::KVHeads;
    const int token = tmp / Geometry::KVHeads;
    const int d     = vec * VecElem;

    const std::int64_t column_base = static_cast<std::int64_t>(batch) * full_width;
    const int position             = positions[column_base + token];
    // Keep only the rows inside the newest `ring_pages * 64` positions: a launch wider than the ring
    // (a Prompt prefill chunk, which also appends through here) would otherwise alias two rows onto
    // one slot with no ordering between them. The newest valid token is the launch's newest position,
    // the same `pos[last]` invariant the tail partition's window uses.
    if (!kv_tail_row_in_ring(positions[column_base + valid - 1], position, ring_pages)) { return; }
    const int ring = batch * ring_pages + ((position >> kPagedKVPageShift) % ring_pages);
    const std::int64_t src =
        static_cast<std::int64_t>(d) +
        static_cast<std::int64_t>(D) *
            (head + static_cast<std::int64_t>(Geometry::KVHeads) *
                        (token + static_cast<std::int64_t>(full_width) * batch));
    const std::int64_t dst =
        paged_kv_element_offset<D, Geometry::KVHeads>(ring, head, position & kPagedKVPageMask, d);
    store_tail_vec8(&tail_k[dst], &k[src]);
    store_tail_vec8(&tail_v[dst], &v[src]);
}

template <typename Geometry, typename Elem>
void launch_exact_tail_stage(const Tensor& key, const Tensor& value, const Tensor& positions,
                             const Tensor& valid_columns, const Tensor& tail_k, const Tensor& tail_v,
                             std::int32_t ring_pages, cudaStream_t stream) {
    constexpr int Threads = 256;
    const int width       = key.ne[2];
    const int batch       = key.ne[3];
    const std::int64_t count =
        static_cast<std::int64_t>(width) * Geometry::KVHeads * (D / 8);
    const dim3 grid(static_cast<unsigned>(div_up(static_cast<int>(count), Threads)),
                    static_cast<unsigned>(batch));
    kvarn_exact_tail_stage_kernel<Geometry, Elem><<<grid, Threads, 0, stream>>>(
        static_cast<const __nv_bfloat16*>(key.data),
        static_cast<const __nv_bfloat16*>(value.data),
        static_cast<const std::int32_t*>(positions.data),
        valid_columns.data == nullptr ? nullptr
                                      : static_cast<const std::int32_t*>(valid_columns.data),
        static_cast<Elem*>(tail_k.data), static_cast<Elem*>(tail_v.data), ring_pages, width, width,
        batch);
    CUDA_CHECK(cudaGetLastError());
}

template <typename Geometry, typename Elem>
void launch_exact_tail_partial(const Tensor& query, const Tensor& positions,
                               const Tensor& valid_columns, const Tensor& tail_k,
                               const Tensor& tail_v, std::int32_t ring_pages,
                               std::int32_t tail_tokens, std::int32_t splits,
                               std::int32_t column_begin, std::int32_t width,
                               std::int32_t logical_capacity, std::int32_t batch_size, float scale,
                               Tensor& partial_acc, Tensor& partial_m, Tensor& partial_l,
                               cudaStream_t stream) {
    const dim3 grid(Geometry::KVHeads, static_cast<unsigned>(splits),
                    static_cast<unsigned>(batch_size));
    kvarn_exact_tail_partial_kernel<Geometry, Elem><<<grid, kExactTailWarps * 32, 0, stream>>>(
        static_cast<const __nv_bfloat16*>(query.data),
        static_cast<const std::int32_t*>(positions.data),
        static_cast<const Elem*>(tail_k.data), static_cast<const Elem*>(tail_v.data), ring_pages,
        tail_tokens, width, query.ne[2], column_begin, logical_capacity, batch_size,
        valid_columns.data == nullptr ? nullptr
                                      : static_cast<const std::int32_t*>(valid_columns.data),
        scale, static_cast<float*>(partial_acc.data), static_cast<float*>(partial_m.data),
        static_cast<float*>(partial_l.data));
    CUDA_CHECK(cudaGetLastError());
}

} // namespace

void stage_exact_tail(const Tensor& key, const Tensor& value, const Tensor& positions,
                      const Tensor& valid_columns, std::int32_t kv_heads, const Tensor& tail_k,
                      const Tensor& tail_v, std::int32_t ring_pages, cudaStream_t stream) {
    // No ring, or nothing to write: leave the launch alone rather than dereference an absent plane.
    if (ring_pages <= 0 || tail_k.data == nullptr || tail_v.data == nullptr ||
        key.data == nullptr || key.ne[2] <= 0 || key.ne[3] <= 0) {
        return;
    }
    with_kv_tail_element(tail_k.dtype, [&]<typename Elem>() {
        if (kv_heads == CausalD256H24Kv4::KVHeads) {
            launch_exact_tail_stage<CausalD256H24Kv4, Elem>(key, value, positions, valid_columns,
                                                            tail_k, tail_v, ring_pages, stream);
        } else if (kv_heads == CausalD256H16Kv2::KVHeads) {
            launch_exact_tail_stage<CausalD256H16Kv2, Elem>(key, value, positions, valid_columns,
                                                            tail_k, tail_v, ring_pages, stream);
        } else {
            throw std::invalid_argument("KVarN exact tail: unsupported KV-head geometry");
        }
    });
}

void exact_tail_partial(const Tensor& query, const Tensor& positions, const Tensor& valid_columns,
                        const Tensor& tail_k, const Tensor& tail_v, std::int32_t ring_pages,
                        std::int32_t tail_tokens, std::int32_t splits, std::int32_t column_begin,
                        std::int32_t width, std::int32_t logical_capacity, std::int32_t batch_size,
                        float scale, Tensor& partial_acc, Tensor& partial_m, Tensor& partial_l,
                        cudaStream_t stream) {
    // No ring or no retention means no tail: the body's partials stand alone and the launch must
    // not touch them (an inactive tail is not an empty one).
    if (tail_tokens <= 0 || ring_pages <= 0 || splits <= 0 || batch_size <= 0 || width <= 0) {
        return;
    }
    const std::int32_t q_heads = query.ne[1];
    with_kv_tail_element(tail_k.dtype, [&]<typename Elem>() {
        if (q_heads == CausalD256H24Kv4::QHeads) {
            launch_exact_tail_partial<CausalD256H24Kv4, Elem>(
                query, positions, valid_columns, tail_k, tail_v, ring_pages, tail_tokens, splits,
                column_begin, width, logical_capacity, batch_size, scale, partial_acc, partial_m,
                partial_l, stream);
        } else if (q_heads == CausalD256H16Kv2::QHeads) {
            launch_exact_tail_partial<CausalD256H16Kv2, Elem>(
                query, positions, valid_columns, tail_k, tail_v, ring_pages, tail_tokens, splits,
                column_begin, width, logical_capacity, batch_size, scale, partial_acc, partial_m,
                partial_l, stream);
        } else {
            throw std::invalid_argument("KVarN exact tail: unsupported query-head geometry");
        }
    });
}

} // namespace ninfer::ops::kvarn
