#pragma once

// ninfer::ops - fused-append exact-tail shadow write (WP2/WP3 fused).
//
// The fused small-T entry writes the quantized body from inside its partial kernel and never
// calls ops::kv_cache_append, so the exact ring it merges against must be written from the same
// source here. This kernel copies this step's tokens, unquantized, into each sequence's ring; it
// runs before the tail partial and is storage-independent (the source is BF16 whatever the body
// coding is). The cached entry gets its ring from ops::kv_cache_append instead, so this kernel
// no-ops for CacheInput::writes_cache == false.

#include <cuda_bf16.h>

#include "ops/softmax_attention/dense/causal_cache/small_t.cuh"

#include <cstdint>

namespace ninfer::ops {

template <typename Geometry, typename CacheInput>
__global__ void causal_attention_small_t_tail_shadow_kernel(
    CacheInput input, const std::int32_t* __restrict__ pos, __nv_bfloat16* __restrict__ tail_k,
    __nv_bfloat16* __restrict__ tail_v, std::int32_t ring_pages, std::int32_t tokens,
    std::int32_t full_width, std::int32_t column_begin,
    const std::int32_t* __restrict__ valid_columns) {
    if constexpr (!CacheInput::writes_cache) { return; }
    constexpr int D       = kCausalHeadDim;
    constexpr int VecElem = 8;
    const int batch       = static_cast<int>(blockIdx.y);
    int valid             = tokens;
    if (valid_columns != nullptr) {
        const int remaining = valid_columns[batch] - column_begin;
        valid               = remaining <= 0 ? 0 : (remaining < tokens ? remaining : tokens);
    }
    const std::int64_t idx = static_cast<std::int64_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    const std::int64_t n   = static_cast<std::int64_t>(valid) * Geometry::KVHeads * (D / VecElem);
    if (idx >= n || ring_pages <= 0) { return; }

    const int vec     = static_cast<int>(idx % (D / VecElem));
    const int tmp     = static_cast<int>(idx / (D / VecElem));
    const int kv_head = tmp % Geometry::KVHeads;
    const int token   = tmp / Geometry::KVHeads;
    const int d       = vec * VecElem;

    const std::int64_t column_base =
        static_cast<std::int64_t>(column_begin) + static_cast<std::int64_t>(batch) * full_width;
    const int position = pos[column_base + token];
    const int ring     = batch * ring_pages + ((position >> kPagedKVPageShift) % ring_pages);
    const std::int64_t src_off = static_cast<std::int64_t>(d) +
                                 static_cast<std::int64_t>(D) *
                                     (kv_head + static_cast<std::int64_t>(Geometry::KVHeads) * token);
    const int4 k_value = load_vec<int4>(&input.k[src_off]);
    const int4 v_value = load_vec<int4>(&input.v[src_off]);
    const std::int64_t dst = paged_kv_element_offset<D, Geometry::KVHeads>(
        ring, kv_head, position & kPagedKVPageMask, d);
    store_vec(&tail_k[dst], k_value);
    store_vec(&tail_v[dst], v_value);
}

} // namespace ninfer::ops
