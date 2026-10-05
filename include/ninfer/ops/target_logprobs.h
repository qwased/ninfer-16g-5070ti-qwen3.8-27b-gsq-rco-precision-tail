#pragma once

#include "core/tensor.h"

#include <cstdint>

#include <cuda_runtime.h> // cudaStream_t

namespace ninfer::ops {

/**
 * Op: Target token log-probabilities
 *
 * Math / indexing:
 *   Let l[r,c] be the exact real value represented by logits[r,c]. For every column c,
 *
 *     ideal[c] = l[target_ids[c],c]
 *                - log(sum_{r=0..valid_rows-1} exp(l[r,c])).
 *
 * Logical shapes:
 *   logits is [physical_rows,C], target_ids is [C], and output is [C], with C>0 and
 *   1<=valid_rows<=physical_rows. Values in target_ids are in [0,valid_rows). Physical rows
 *   [valid_rows,physical_rows) do not participate in either the denominator or target lookup.
 *
 * Supported domain:
 *   logits is contiguous finite BF16, target_ids is contiguous I32, and output is contiguous
 *   FP32. Storage has its dtype's natural alignment.
 *
 * Numeric:
 *   output is the FP32 numerical approximation of ideal. Reduction association and private
 *   accumulator precision are implementation choices; the independent oracle evaluates the full
 *   formula in FP64 from the represented BF16 inputs.
 *
 * Effects:
 *   Writes every output element and preserves both inputs. Output must not overlap logits or
 *   target_ids.
 *
 * Workspace:
 *   None.
 *
 * Execution:
 *   Enqueues work on stream and owns no persistent state.
 */
void target_logprobs(const Tensor& logits, const Tensor& target_ids, std::int32_t valid_rows,
                     Tensor& output, cudaStream_t stream);

/**
 * Op: Target token log-probabilities plus the top-K next-token distribution
 *
 * Math / indexing:
 *   Let l[r,c] be the exact real value represented by logits[r,c], and let
 *   z[c] = max_{r<valid_rows} l[r,c], s[c] = sum_{r<valid_rows} exp(l[r,c] - z[c]), so that
 *   log p(r|c) = l[r,c] - z[c] - log s[c]. For every column c:
 *
 *     target_out[c] = log p(target_ids[c] | c);
 *     (topk_ids[j,c], topk_logprobs[j,c]), j = 0..k-1, is the j-th largest pair under the total
 *     order "greater value first, smaller token id first", with its log p. When fewer than k rows
 *     have a finite value, or a row's value compares equal to the k-th selected pair through NaN
 *     (undefined input, see below), the remaining entries are token id -1 with logprob -inf.
 *
 *   target_out is defined by exactly the same reduction as target_logprobs, so the two agree bit
 *   for bit on the same logits, and a token that is both a target and a top-K member carries the
 *   identical float in both outputs.
 *
 * Logical shapes:
 *   logits is [physical_rows,C], target_ids is [C], target_out is [C], topk_ids is [k,C] and
 *   topk_logprobs is [k,C], with C>0, k>0 and 1<=valid_rows<=physical_rows. Values in target_ids
 *   are in [0,valid_rows). Physical rows [valid_rows,physical_rows) participate in neither the
 *   denominator, the target lookup, nor the selection.
 *
 * Supported domain:
 *   logits is contiguous finite BF16, target_ids is contiguous I32, target_out and
 *   topk_logprobs are contiguous FP32, and topk_ids is contiguous I32, with k<=128. Storage has
 *   each dtype's natural alignment.
 *
 * Numeric:
 *   target_out is the FP32 numerical approximation of log p(target|c) and every topk_logprobs
 *   entry is the FP32 numerical approximation of log p of its token. The selection itself is
 *   exact over the represented BF16 values: it is the top-k of the stored values, not of an
 *   approximation of them.
 *
 * Effects:
 *   Writes every output element and preserves both inputs. No output may overlap either input or
 *   another output.
 *
 * Workspace:
 *   None.
 *
 * Execution:
 *   Enqueues work on stream and owns no persistent state. One CTA selects for one column; the
 *   column's max and sum are computed once and then reused by all k selection passes, so the
 *   operation costs one normalization pass plus k block-wide selections.
 */
void target_logprobs_topk(const Tensor& logits, const Tensor& target_ids, std::int32_t valid_rows,
                          std::int32_t k, Tensor& target_out, Tensor& topk_ids,
                          Tensor& topk_logprobs, cudaStream_t stream);

} // namespace ninfer::ops
