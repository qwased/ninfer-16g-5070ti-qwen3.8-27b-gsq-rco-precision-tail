#pragma once

#include "core/tensor.h"

#include <cuda_runtime_api.h>

#include <cstdint>

namespace ninfer::ops::kvarn {

// WP6: the shared exact tail ring (`PagedKVExactTailView`) as the KVarN route uses it. The ring is
// 64-token pages, one sequence per batch row, `ring_pages` pages each, page of absolute position
// p = (p / 64) % ring_pages, and holds the newest rows unquantized.

// WP6.3: write this step's rows into the ring, in the original domain, before the tail partial reads
// them. `small_t_tail.cuh`'s fused-append entry needs this because its body kernel never calls
// `ops::kv_cache_append`; the KVarN route needs it for the same reason -- `kvarn_kv_append` is its
// only append entry and it writes the quantized body itself. Call it before the K/V are rotated.
//
// `key`/`value` are the contiguous BF16 [D, kv_heads, width, batch] source rows, `positions` the
// contiguous I32 [width, batch] absolute positions, `valid_columns` an I32 [batch] prefix count per
// row (empty = every column is live). Only rows inside the newest 64 * ring_pages positions land:
// a launch wider than the ring (a prefill chunk) would otherwise alias two rows onto one slot with
// no ordering between them. A `ring_pages <= 0` or a null ring leaves the launch alone.
void stage_exact_tail(const Tensor& key, const Tensor& value, const Tensor& positions,
                      const Tensor& valid_columns, std::int32_t kv_heads, const Tensor& tail_k,
                      const Tensor& tail_v, std::int32_t ring_pages, cudaStream_t stream);

// WP6.1: launch the rotated-domain exact-tail partial for one launch of the KVarN body.
//
// The kernel writes only the tail splits `[body_active, total_active)`, for the query columns
// `[column_begin, column_begin + width)`; the body owns the rest. The accumulator is left in the
// rotated domain the body's reduce merges in. `tail_tokens` is the configured retention; `<= 0` (or
// a `ring_pages <= 0`) skips the launch entirely and leaves `partial_acc/m/l` untouched, so a launch
// without a tail is unchanged.
void exact_tail_partial(const Tensor& query, const Tensor& positions, const Tensor& valid_columns,
                        const Tensor& tail_k, const Tensor& tail_v, std::int32_t ring_pages,
                        std::int32_t tail_tokens, std::int32_t splits, std::int32_t column_begin,
                        std::int32_t width, std::int32_t logical_capacity, std::int32_t batch_size,
                        float scale, Tensor& partial_acc, Tensor& partial_m, Tensor& partial_l,
                        cudaStream_t stream);

} // namespace ninfer::ops::kvarn
