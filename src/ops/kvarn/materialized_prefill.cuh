#pragma once

#include "ninfer/ops/kvarn.h"
#include "ops/softmax_attention/dense/causal_cache/prompt_common.cuh"
#include "ops/kvarn/config.cuh"
#include "ops/kvarn/decode.cuh"

#include <cuda_bf16.h>
#include <cuda_fp16.h>

#include <cstdint>

namespace ninfer::ops::kvarn::detail {

struct PrefillCache {
    const std::uint8_t* records;
    const __nv_bfloat16* tail_k;
    const __nv_bfloat16* tail_v;
    const std::int32_t* markers;
    const std::int32_t* block_tables;
    const std::int32_t* table_rows;
    int table_stride;
    int heads;
};

template <typename Metadata, int KBits, int VBits>
__launch_bounds__(D) __global__
    void materialize_prefill_slab_kernel(PrefillCache cache, Metadata metadata,
                                         const std::int32_t* positions, int width, int slab_begin,
                                         int slab_tokens, __nv_bfloat16* materialized_k,
                                         __nv_bfloat16* materialized_v, CurrentKV current) {
    const int logical_page = slab_begin / Group + static_cast<int>(blockIdx.x);
    const int head         = static_cast<int>(blockIdx.y);
    const int d            = static_cast<int>(threadIdx.x);
    const int tokens       = metadata.valid_tokens(width);
    if (tokens <= 0) { return; }
    const int visible    = positions[tokens - 1] + 1;
    const int page_begin = logical_page * Group;
    if (page_begin >= visible) { return; }

    const int table_row = cache.table_rows[0];
    int tail_slot       = -1;
#pragma unroll
    for (int slot = 0; slot < kKvarnTailSlots; ++slot) {
        if (cache.markers[slot + kKvarnTailSlots * table_row] == logical_page) { tail_slot = slot; }
    }

    if (tail_slot >= 0) {
        for (int token = 0; token < Group && page_begin + token < visible; ++token) {
            const std::int64_t source =
                static_cast<std::int64_t>(d) +
                static_cast<std::int64_t>(D) *
                    (token +
                     Group * (head + cache.heads * (tail_slot + kKvarnTailSlots * table_row)));
            const std::int64_t destination =
                static_cast<std::int64_t>(d) +
                static_cast<std::int64_t>(D) * (page_begin + token - slab_begin +
                                                static_cast<std::int64_t>(slab_tokens) * head);
            materialized_k[destination] = cache.tail_k[source];
            materialized_v[destination] = cache.tail_v[source];
        }
        return;
    }

    if (current.key != nullptr && page_begin >= current.positions[0]) {
        const int begin = current.positions[0];
        for (int token = 0; token < Group && page_begin + token < visible; ++token) {
            const std::int64_t source = d + static_cast<std::int64_t>(D) *
                                                (head + cache.heads * (page_begin + token - begin));
            const std::int64_t destination =
                d + static_cast<std::int64_t>(D) * (page_begin + token - slab_begin +
                                                    static_cast<std::int64_t>(slab_tokens) * head);
            materialized_k[destination] = current.key[source];
            materialized_v[destination] = current.value[source];
        }
        return;
    }

    const int physical_page =
        cache
            .block_tables[logical_page + static_cast<std::int64_t>(cache.table_stride) * table_row];
    const std::uint8_t* record =
        cache.records +
        (static_cast<std::int64_t>(physical_page) * cache.heads + head) *
            kvarn_record_bytes(KBits, VBits);
    const auto* k_scale =
        reinterpret_cast<const __half*>(record + kvarn_k_scale_offset(KBits, VBits));
    const auto* k_zero =
        reinterpret_cast<const __half*>(record + kvarn_k_zero_offset(KBits, VBits));
    const auto* k_token_scale =
        reinterpret_cast<const __half*>(record + kvarn_k_token_scale_offset(KBits, VBits));
    const auto* v_channel =
        reinterpret_cast<const __half*>(record + kvarn_v_channel_scale_offset(KBits, VBits));
    const auto* v_scale =
        reinterpret_cast<const __half*>(record + kvarn_v_token_scale_offset(KBits, VBits));
    const auto* v_zero =
        reinterpret_cast<const __half*>(record + kvarn_v_token_zero_offset(KBits, VBits));
    const std::uint8_t* k_row =
        record + kvarn_k_packed_offset(KBits, VBits) + d * kvarn_k_row_bytes(KBits);
    std::uint32_t packed_k[KBits == 4 ? Group / 8 : 1];
    if constexpr (KBits == 4) {
#pragma unroll
        for (int word = 0; word < Group / 8; ++word) {
            packed_k[word] = reinterpret_cast<const std::uint32_t*>(k_row)[word];
        }
    }
    const float column_scale  = __half2float(k_scale[d]);
    const float column_zero   = __half2float(k_zero[d]);
    const float channel_scale = __half2float(v_channel[d]);
    for (int token = 0; token < Group && page_begin + token < visible; ++token) {
        const int k_code = KBits == 4
                               ? static_cast<int>((packed_k[token / 8] >> (4 * (token & 7))) & 15)
                               : static_cast<int>(kvarn_unpack_code(k_row, token * KBits, KBits));
        const std::uint8_t* v_row =
            record + kvarn_v_packed_offset(KBits, VBits) + token * kvarn_v_row_bytes(VBits);
        const int v_code = static_cast<int>(kvarn_unpack_code(v_row, d * VBits, VBits));
        const float key  = fmaf(static_cast<float>(k_code), column_scale, column_zero) *
                          __half2float(k_token_scale[token]);
        const float value = fmaf(static_cast<float>(v_code), __half2float(v_scale[token]),
                                 __half2float(v_zero[token])) *
                            channel_scale;
        const std::int64_t destination =
            static_cast<std::int64_t>(d) +
            static_cast<std::int64_t>(D) *
                (page_begin + token - slab_begin + static_cast<std::int64_t>(slab_tokens) * head);
        materialized_k[destination] = __float2bfloat16_rn(key);
        materialized_v[destination] = __float2bfloat16_rn(value);
    }
}

struct MaterializedPrefillInput {
    const __nv_bfloat16* key;
    const __nv_bfloat16* value;
    int slab_begin;
    int slab_tokens;

    template <bool Key>
    __device__ __forceinline__ void stage(__nv_bfloat16* destination, int kv_head, int k0,
                                          int max_query_abs, int tid) const {
        constexpr int Bc        = kCausalPromptBc;
        constexpr int Threads   = kCausalPromptThreads;
        constexpr int VecPerRow = D / 8;
        const auto* source_block =
            (Key ? key : value) +
            static_cast<std::int64_t>(D) *
                (static_cast<std::int64_t>(slab_tokens) * kv_head + k0 - slab_begin);
        if (k0 + Bc - 1 <= max_query_abs) {
#pragma unroll
            for (int chunk = tid; chunk < Bc * VecPerRow; chunk += Threads) {
                const int token = chunk >> 5;
                const int d     = (chunk & 31) << 3;
                auto* output    = destination + token * D + causal_prompt_swz(token, d);
                cp_async<16, Cache::cg>(output, source_block + token * D + d);
            }
        } else {
#pragma unroll
            for (int chunk = tid; chunk < Bc * VecPerRow; chunk += Threads) {
                const int token = chunk >> 5;
                const int d     = (chunk & 31) << 3;
                auto* output    = destination + token * D + causal_prompt_swz(token, d);
                if (k0 + token <= max_query_abs) {
                    cp_async<16, Cache::cg>(output, source_block + token * D + d);
                } else {
                    store_vec(output, make_int4(0, 0, 0, 0));
                }
            }
        }
    }
};

} // namespace ninfer::ops::kvarn::detail
