#pragma once

#include "ninfer/types.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace ninfer::perplexity {

struct WindowPlan {
    std::size_t input_begin    = 0;
    std::size_t input_end      = 0;
    std::size_t target_begin   = 0;
    std::size_t target_end     = 0;
    std::uint32_t first_target = 0;
};

[[nodiscard]] std::vector<WindowPlan> plan_windows(std::size_t tokens, std::uint32_t context,
                                                   std::uint32_t stride);

// Back-to-back windows of `context` tokens, each scored from its second token on, and no partial
// window at the end: the protocol of the GPTQ-lineage WikiText-2 numbers papers quote.
[[nodiscard]] std::vector<WindowPlan> plan_disjoint_windows(std::size_t tokens,
                                                            std::uint32_t context);

struct ScoreAggregate {
    std::uint64_t scored_tokens = 0;
    double total_nll            = 0.0;

    void add(std::span<const ninfer::ScoredTarget> targets);
    void add(const ScoreAggregate& other) noexcept;
    [[nodiscard]] double mean_nll() const;
    [[nodiscard]] double ppl() const;
};

// ---------------------------------------------------------------------------------------------
// Top-K reference distributions and KLD.
//
// The reference is one run's per-target top-K next-token distributions, normally the BF16-body
// baseline. A candidate run over the same corpus, context, stride, score width and protocol
// compares against it with KLD(candidate || reference), matching the metric llama.cpp reports,
// over a support of the two runs' top-K sets.
// ---------------------------------------------------------------------------------------------

// The scorable protocol a reference was produced under. Every field is checked when the reference
// is loaded, so a KLD number is always against the same scored positions; only the KV
// representation may differ between the two runs, because comparing representations is the point.
struct TopKReferenceProtocol {
    std::string corpus_id;
    std::string model_name;
    // The KV representation this run scored, recorded for the report; not part of the identity.
    std::string kv_dtype;
    std::uint32_t context     = 0;
    std::uint32_t stride      = 0;
    std::uint32_t score_width = 0;
    bool disjoint_windows     = false;
    std::uint32_t top_k       = 0;
    std::uint64_t scored_tokens = 0;
};

// Binary reference file (private to this tool): the magic, version, byte-order marker and the
// protocol fields above, then three flat planes over the scored targets in scored order -- target
// log probabilities, top_k token ids, and top_k log probabilities, the last two in descending log
// probability order. Compact (4 + 8*top_k bytes per target) and self-describing, and carrying
// enough of the protocol to refuse a comparison that would not be like for like.
void save_topk_reference(const std::filesystem::path& path, const TopKReferenceProtocol& protocol,
                         std::span<const ninfer::ScoredTarget> targets);

// Loads `path` and rejects it when the protocol does not match `expected` in corpus, model,
// context, stride, score width, window mode or scored-token count. Throws std::runtime_error.
[[nodiscard]] std::vector<ninfer::ScoredTarget>
load_topk_reference(const std::filesystem::path& path, const TopKReferenceProtocol& expected);

struct KldSummary {
    std::uint64_t targets = 0;
    // KLD(candidate || reference) in nats, over the per-target support defined on KldAccumulator.
    double mean   = 0.0;
    double median = 0.0;
    double p99    = 0.0;
    double p999   = 0.0;
    double maximum = 0.0;
    // Fraction of targets whose most probable token is the same on both sides. The top-K argmax is
    // the vocabulary argmax, so this is exact.
    double same_top = 0.0;
    // Mean per-target difference of the scored target token's own log probability.
    double mean_target_logprob_delta = 0.0;
};

// KLD of a top-K candidate distribution against a top-K reference distribution.
//
// Support: the union of the two top-K token sets plus the target token itself. Each side then
// supplies a log probability for every support token -- its own top-K entry, its target log
// probability when the token is the target, or, for a token the side did not select, the log
// probability of that side's least probable top-K entry, which is the smallest value the side
// actually measured. Both distributions are renormalized over the support, so mass outside the
// union is dropped on both sides; that is the same top-K approximation llama.cpp's KLD makes, and
// it is why only incremental KLD differences are meaningful.
class KldAccumulator {
public:
    void add(const ninfer::ScoredTarget& candidate, const ninfer::ScoredTarget& reference,
             ninfer::TokenId target_token);

    [[nodiscard]] std::uint64_t targets() const noexcept {
        return static_cast<std::uint64_t>(kld_.size());
    }

    // Collects the statistics. The collected values are sorted here, so the reported percentiles
    // use the ascending index nearest to (fraction * count), clamped to the last value.
    [[nodiscard]] KldSummary summary() const;

private:
    std::vector<double> kld_;
    double total_ = 0.0;
    double target_logprob_delta_total_ = 0.0;
    std::uint64_t same_top_            = 0;
};

} // namespace ninfer::perplexity
