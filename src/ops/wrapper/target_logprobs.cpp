// ninfer::ops - target_logprobs wrapper: public contract validation and launcher dispatch.
#include "ninfer/ops/target_logprobs.h"

#include "ninfer/types.h"
#include "ops/launcher/target_logprobs.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace ninfer::ops {
namespace {

void require_rank_two(const Tensor& tensor, const char* label) {
    if (tensor.ne[0] <= 0 || tensor.ne[1] <= 0 || tensor.ne[2] != 1 || tensor.ne[3] != 1) {
        throw std::invalid_argument(std::string("target_logprobs: ") + label +
                                    " must be rank-2 with positive dimensions");
    }
}

void require_vector(const Tensor& tensor, std::int32_t columns, const char* label) {
    if (tensor.ne[0] != columns || tensor.ne[1] != 1 || tensor.ne[2] != 1 || tensor.ne[3] != 1) {
        throw std::invalid_argument(std::string("target_logprobs: ") + label +
                                    " must have shape [columns]");
    }
}

void require_rows_columns(const Tensor& tensor, std::int32_t rows, std::int32_t columns,
                          const char* label) {
    if (tensor.ne[0] != rows || tensor.ne[1] != columns || tensor.ne[2] != 1 ||
        tensor.ne[3] != 1) {
        throw std::invalid_argument(std::string("target_logprobs_topk: ") + label +
                                    " must have shape [k,columns]");
    }
}

void require_accessible(const Tensor& tensor, std::size_t alignment, const char* label) {
    if (!tensor.is_contiguous()) {
        throw std::invalid_argument(std::string("target_logprobs: ") + label +
                                    " must be contiguous");
    }
    if (tensor.data == nullptr) {
        throw std::invalid_argument(std::string("target_logprobs: ") + label +
                                    " data must be non-null");
    }
    if ((reinterpret_cast<std::uintptr_t>(tensor.data) & (alignment - 1)) != 0) {
        throw std::invalid_argument(std::string("target_logprobs: ") + label +
                                    " data is not naturally aligned");
    }
}

bool overlaps(const Tensor& lhs, const Tensor& rhs) {
    const auto lhs_begin = reinterpret_cast<std::uintptr_t>(lhs.data);
    const auto rhs_begin = reinterpret_cast<std::uintptr_t>(rhs.data);
    if (lhs_begin <= rhs_begin) { return rhs_begin - lhs_begin < lhs.bytes(); }
    return lhs_begin - rhs_begin < rhs.bytes();
}

} // namespace

void target_logprobs(const Tensor& logits, const Tensor& target_ids, std::int32_t valid_rows,
                     Tensor& output, cudaStream_t stream) {
    if (logits.dtype != DType::BF16) {
        throw std::invalid_argument("target_logprobs: logits must be BF16");
    }
    if (target_ids.dtype != DType::I32) {
        throw std::invalid_argument("target_logprobs: target_ids must be I32");
    }
    if (output.dtype != DType::FP32) {
        throw std::invalid_argument("target_logprobs: output must be FP32");
    }

    require_rank_two(logits, "logits");
    const std::int32_t columns = logits.ne[1];
    require_vector(target_ids, columns, "target_ids");
    require_vector(output, columns, "output");
    if (valid_rows <= 0 || valid_rows > logits.ne[0]) {
        throw std::invalid_argument("target_logprobs: valid_rows must be in [1, physical_rows]");
    }

    (void)logits.bytes();
    (void)target_ids.bytes();
    (void)output.bytes();
    require_accessible(logits, alignof(std::uint16_t), "logits");
    require_accessible(target_ids, alignof(std::int32_t), "target_ids");
    require_accessible(output, alignof(float), "output");
    if (overlaps(output, logits) || overlaps(output, target_ids)) {
        throw std::invalid_argument("target_logprobs: output must not overlap either input");
    }

    detail::target_logprobs_launch(logits, target_ids, valid_rows, output, stream);
}

void target_logprobs_topk(const Tensor& logits, const Tensor& target_ids, std::int32_t valid_rows,
                          std::int32_t k, Tensor& target_out, Tensor& topk_ids,
                          Tensor& topk_logprobs, cudaStream_t stream) {
    if (logits.dtype != DType::BF16) {
        throw std::invalid_argument("target_logprobs_topk: logits must be BF16");
    }
    if (target_ids.dtype != DType::I32) {
        throw std::invalid_argument("target_logprobs_topk: target_ids must be I32");
    }
    if (target_out.dtype != DType::FP32) {
        throw std::invalid_argument("target_logprobs_topk: target_out must be FP32");
    }
    if (topk_ids.dtype != DType::I32) {
        throw std::invalid_argument("target_logprobs_topk: topk_ids must be I32");
    }
    if (topk_logprobs.dtype != DType::FP32) {
        throw std::invalid_argument("target_logprobs_topk: topk_logprobs must be FP32");
    }

    require_rank_two(logits, "logits");
    const std::int32_t columns = logits.ne[1];
    require_vector(target_ids, columns, "target_ids");
    require_vector(target_out, columns, "target_out");
    if (valid_rows <= 0 || valid_rows > logits.ne[0]) {
        throw std::invalid_argument(
            "target_logprobs_topk: valid_rows must be in [1, physical_rows]");
    }
    if (k <= 0 || k > kMaxScoreTopK) {
        throw std::invalid_argument("target_logprobs_topk: k must be in [1, kMaxScoreTopK]");
    }
    require_rows_columns(topk_ids, k, columns, "topk_ids");
    require_rows_columns(topk_logprobs, k, columns, "topk_logprobs");

    (void)logits.bytes();
    (void)target_ids.bytes();
    (void)target_out.bytes();
    (void)topk_ids.bytes();
    (void)topk_logprobs.bytes();
    require_accessible(logits, alignof(std::uint16_t), "logits");
    require_accessible(target_ids, alignof(std::int32_t), "target_ids");
    require_accessible(target_out, alignof(float), "target_out");
    require_accessible(topk_ids, alignof(std::int32_t), "topk_ids");
    require_accessible(topk_logprobs, alignof(float), "topk_logprobs");
    if (overlaps(target_out, logits) || overlaps(target_out, target_ids) ||
        overlaps(topk_ids, logits) || overlaps(topk_ids, target_ids) ||
        overlaps(topk_ids, target_out) || overlaps(topk_logprobs, logits) ||
        overlaps(topk_logprobs, target_ids) || overlaps(topk_logprobs, target_out) ||
        overlaps(topk_logprobs, topk_ids)) {
        throw std::invalid_argument(
            "target_logprobs_topk: no output may overlap an input or another output");
    }

    detail::target_logprobs_topk_launch(logits, target_ids, valid_rows, k, target_out, topk_ids,
                                        topk_logprobs, stream);
}

} // namespace ninfer::ops
