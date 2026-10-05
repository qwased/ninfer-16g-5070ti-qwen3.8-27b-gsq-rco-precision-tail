#pragma once

// Implements: include/ninfer/ops/target_logprobs.h
// Match: contiguous BF16 [physical_rows,C], I32 [C], and FP32 [C] (plus I32/FP32 [k,C] for the
// top-K variant).
// Algorithm assumptions: one 256-thread CTA performs a single-pass online logsumexp per column;
// the top-K variant then repeats a block-wide maximum over the column k times.

#include "ops/common/warp.cuh"

#include <cuda_bf16.h>
#include <math_constants.h>

#include <cstdint>

namespace ninfer::ops {

inline constexpr int kTargetLogprobsBlock = 256;

template <int BlockSize>
__device__ __forceinline__ float target_logprobs_block_max(float value) {
    static_assert(BlockSize >= kWarpSize && BlockSize <= 1024);
    static_assert((BlockSize & (BlockSize - 1)) == 0);
    constexpr int kWarps = BlockSize / kWarpSize;
    __shared__ float warp_maxima[kWarps];
    __shared__ float result;

    const int lane = static_cast<int>(threadIdx.x) & (kWarpSize - 1);
    const int warp = static_cast<int>(threadIdx.x) / kWarpSize;
    value          = warp_max(value);
    if (lane == 0) { warp_maxima[warp] = value; }
    __syncthreads();

    if (warp == 0) {
        value = lane < kWarps ? warp_maxima[lane] : -CUDART_INF_F;
        value = warp_max(value);
        if (lane == 0) { result = value; }
    }
    __syncthreads();
    return result;
}

// One column's log-softmax denominator: the maximum stored value and the sum of its exponentials.
// Both target-logprob shapes share this exact reduction, so a target token and a top-K member
// carrying the same stored value also carry the same log probability. Both members are always
// written by column_normalization before the value is read; no default initializer is used because
// the cudafe host pass parses this type too and CUDART_INF_F is device-only there.
struct ColumnNormalization {
    float maximum;
    float sum;
};

template <int BlockSize>
__device__ __forceinline__ ColumnNormalization
column_normalization(const __nv_bfloat16* column_logits, std::int32_t valid_rows) {
    float local_max = -CUDART_INF_F;
    float local_sum = 0.0f;
    // Requires finite logits (see target_logprobs.h): -inf before any finite value in a
    // thread's subsequence NaN-poisons local_sum through expf(-inf - -inf).
    for (std::int32_t row = static_cast<std::int32_t>(threadIdx.x); row < valid_rows;
         row += BlockSize) {
        const float value   = __bfloat162float(column_logits[row]);
        const float new_max = fmaxf(local_max, value);
        if (new_max > local_max) {
            local_sum *= expf(local_max - new_max);
        }
        local_max = new_max;
        local_sum += expf(value - local_max);
    }
    const float maximum = target_logprobs_block_max<BlockSize>(local_max);

    local_sum *= expf(local_max - maximum);
    __shared__ float warp_sums[BlockSize / kWarpSize];
    ColumnNormalization out;
    out.maximum = maximum;
    out.sum     = block_reduce_sum<BlockSize>(local_sum, warp_sums);
    return out;
}

template <int BlockSize>
__launch_bounds__(BlockSize) __global__
    void target_logprobs_kernel(const __nv_bfloat16* logits, const std::int32_t* target_ids,
                                float* output, std::int32_t valid_rows,
                                std::int32_t physical_rows) {
    const std::int32_t column = static_cast<std::int32_t>(blockIdx.x);
    const std::int64_t base   = static_cast<std::int64_t>(column) * physical_rows;

    const ColumnNormalization normalization =
        column_normalization<BlockSize>(logits + base, valid_rows);
    if (threadIdx.x == 0) {
        const float target = __bfloat162float(logits[base + target_ids[column]]);
        output[column]     = target - normalization.maximum - logf(normalization.sum);
    }
}

// ---------------------------------------------------------------------------------------------
// Top-K variant: the same column normalization, then k exact block-wide selections.
//
// Selection order is the total order "greater value first, smaller token id first", so equal
// values are broken deterministically by id and every row is selectable exactly once. One pass
// per rank scans the column and keeps the best pair strictly below the previously selected pair;
// the pass is a standard warp/shuffle reduction followed by a cross-warp reduction. The column's
// max and sum are computed once and reused, so each returned log probability is
// `value - maximum - logf(sum)` -- the identical expression, over the identical reduction, that
// target_logprobs uses for the target token.
// ---------------------------------------------------------------------------------------------

// Sentinel pair for "no candidate": below every real (finite) logit under topk_pair_better,
// because a finite value compares greater than -inf and equal values fall to the smaller id.
inline constexpr std::int32_t kTopKNoToken = 0x7fffffff;

__device__ __forceinline__ bool topk_pair_better(float lhs_value, std::int32_t lhs_token,
                                                 float rhs_value, std::int32_t rhs_token) {
    if (lhs_value != rhs_value) { return lhs_value > rhs_value; }
    return lhs_token < rhs_token;
}

// True when (value,token) sorts strictly below the pair selected at the previous rank. A NaN
// value is never below anything, which only arises for the non-finite input the op excludes.
__device__ __forceinline__ bool topk_pair_below(float value, std::int32_t token,
                                                float threshold_value,
                                                std::int32_t threshold_token) {
    return topk_pair_better(threshold_value, threshold_token, value, token);
}

__device__ __forceinline__ void topk_warp_reduce(float& value, std::int32_t& token) {
    constexpr unsigned int kFullMask = 0xffffffffU;
#pragma unroll
    for (int offset = kWarpSize / 2; offset > 0; offset /= 2) {
        const float other_value = __shfl_down_sync(kFullMask, value, offset);
        const std::int32_t other_token = __shfl_down_sync(kFullMask, token, offset);
        if (topk_pair_better(other_value, other_token, value, token)) {
            value = other_value;
            token = other_token;
        }
    }
}

// Reduces the block's best pair into lane 0 of warp 0 and synchronizes the block. The
// synchronization both orders the shared staging this reduction uses and, in the selection loop,
// keeps the published threshold of rank r ordered against the rank r+1 scan.
template <int BlockSize>
__device__ __forceinline__ void topk_block_reduce(float& value, std::int32_t& token) {
    constexpr int kWarps = BlockSize / kWarpSize;
    __shared__ float warp_values[kWarps];
    __shared__ std::int32_t warp_tokens[kWarps];

    const int lane = static_cast<int>(threadIdx.x) & (kWarpSize - 1);
    const int warp = static_cast<int>(threadIdx.x) / kWarpSize;
    topk_warp_reduce(value, token);
    if (lane == 0) {
        warp_values[warp] = value;
        warp_tokens[warp] = token;
    }
    __syncthreads();
    if (warp == 0) {
        value = lane < kWarps ? warp_values[lane] : -CUDART_INF_F;
        token = lane < kWarps ? warp_tokens[lane] : kTopKNoToken;
        topk_warp_reduce(value, token);
    }
}

template <int BlockSize>
__launch_bounds__(BlockSize) __global__
    void target_logprobs_topk_kernel(const __nv_bfloat16* logits, const std::int32_t* target_ids,
                                     float* target_out, std::int32_t* topk_ids,
                                     float* topk_logprobs, std::int32_t valid_rows,
                                     std::int32_t physical_rows, std::int32_t k,
                                     std::int32_t columns) {
    const std::int32_t column = static_cast<std::int32_t>(blockIdx.x);
    const std::int64_t base   = static_cast<std::int64_t>(column) * physical_rows;
    const __nv_bfloat16* column_logits = logits + base;

    const ColumnNormalization normalization =
        column_normalization<BlockSize>(column_logits, valid_rows);
    if (threadIdx.x == 0) {
        const float target = __bfloat162float(column_logits[target_ids[column]]);
        target_out[column] = target - normalization.maximum - logf(normalization.sum);
    }

    // Published selection threshold, uniform across the block after each reduction.
    __shared__ float selected_value;
    __shared__ std::int32_t selected_token;
    if (threadIdx.x == 0) {
        selected_value = CUDART_INF_F;
        selected_token = -1; // Never a valid row: the first rank admits every value.
    }
    __syncthreads();

    for (std::int32_t rank = 0; rank < k; ++rank) {
        const float threshold_value       = selected_value;
        const std::int32_t threshold_token = selected_token;

        float best_value       = -CUDART_INF_F;
        std::int32_t best_token = kTopKNoToken;
        for (std::int32_t row = static_cast<std::int32_t>(threadIdx.x); row < valid_rows;
             row += BlockSize) {
            const float value = __bfloat162float(column_logits[row]);
            if (!topk_pair_below(value, row, threshold_value, threshold_token)) { continue; }
            if (topk_pair_better(value, row, best_value, best_token)) {
                best_value = value;
                best_token = row;
            }
        }
        topk_block_reduce<BlockSize>(best_value, best_token);
        if (threadIdx.x == 0) {
            selected_value = best_value;
            selected_token = best_token;
        }
        __syncthreads();

        if (selected_token == kTopKNoToken) {
            // Fewer than k rows are selectable (valid_rows < k): pad with an impossible token at
            // log probability -inf and leave the remaining ranks unselected.
            if (threadIdx.x == 0) {
                for (std::int32_t rest = rank; rest < k; ++rest) {
                    topk_ids[static_cast<std::int64_t>(rest) * columns + column] = -1;
                    topk_logprobs[static_cast<std::int64_t>(rest) * columns + column] =
                        -CUDART_INF_F;
                }
            }
            break; // selected_token is block-uniform here, so the barrier exit is safe.
        }
        if (threadIdx.x == 0) {
            topk_ids[static_cast<std::int64_t>(rank) * columns + column] = selected_token;
            topk_logprobs[static_cast<std::int64_t>(rank) * columns + column] =
                selected_value - normalization.maximum - logf(normalization.sum);
        }
        // The published pair is read at the top of the next rank; that read is ordered against
        // this rank's publish by the barrier inside the next reduction, so no barrier is needed
        // after the write above.
    }
}

} // namespace ninfer::ops
