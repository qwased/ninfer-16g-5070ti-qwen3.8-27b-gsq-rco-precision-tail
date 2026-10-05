#include "guarded_main.h"
#include "ninfer/engine.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

int run() {
    const char* artifact = std::getenv("NINFER_TEST_ARTIFACT");
    if (artifact == nullptr || *artifact == '\0') {
        std::cout << "SKIP: NINFER_TEST_ARTIFACT is not set\n";
        return 77;
    }

    ninfer::EngineOptions options;
    options.artifact_path = artifact;
    options.purpose       = ninfer::EnginePurpose::CausalScoring;
    options.max_context   = 2048;
    options.kv_cache      = ninfer::KvCacheStorage::Fp8E4M3Row256;

    // The suffix window's target log probabilities, carried out of the first Engine so the top-K
    // Engine below can be the only resident model.
    std::vector<ninfer::TokenId> tokens;
    std::vector<float> suffix_logprobs;
    constexpr std::uint32_t second_start = 513;
    {
        ninfer::Engine engine(options);
        const auto& effective = engine.options();
        if (effective.max_concurrency != 1 || effective.prefill_chunk != 1024 ||
            effective.kv_capacity.mode != ninfer::KvCapacityMode::Explicit ||
            effective.kv_capacity.explicit_tokens != effective.max_context ||
            effective.context_cache.enabled ||
            effective.speculative.backend != ninfer::SpeculativeBackend::None ||
            effective.kv_cache != ninfer::KvCacheStorage::Fp8E4M3Row256 ||
            effective.score_topk != 0) {
            std::cerr << "causal scoring options were not normalized correctly\n";
            return 1;
        }

        std::string text;
        const std::string paragraph =
            "NInfer scores each target token from the preceding hidden state. "
            "Every evaluation window owns fresh state and a fresh KV address space.\n";
        while (tokens.size() < 1537) {
            text += paragraph;
            tokens = engine.tokenize_text(text);
        }
        tokens.resize(1537);

        const std::vector<ninfer::ScoredTarget> all = engine.score_tokens(tokens, 1);
        const std::vector<ninfer::ScoredTarget> suffix =
            engine.score_tokens(tokens, second_start);
        const std::vector<ninfer::ScoredTarget> repeated =
            engine.score_tokens(tokens, second_start);
        if (all.size() != 1536 || suffix.size() != 1024 || repeated.size() != suffix.size()) {
            std::cerr << "causal scoring returned an invalid result shape\n";
            return 1;
        }
        for (const ninfer::ScoredTarget& target : all) {
            if (!std::isfinite(target.logprob) || target.logprob > 0.0F) {
                std::cerr << "causal scoring returned an invalid log probability\n";
                return 1;
            }
            if (!target.topk.empty()) {
                std::cerr << "causal scoring returned a top-K distribution without score_topk\n";
                return 1;
            }
        }
        for (std::size_t i = 0; i < suffix.size(); ++i) {
            if (!std::isfinite(suffix[i].logprob) || suffix[i].logprob > 0.0F) {
                std::cerr << "causal scoring returned a non-finite logprob\n";
                return 1;
            }
            if (suffix[i].logprob != repeated[i].logprob) {
                std::cerr << "a repeated score window inherited prior State/KV\n";
                return 1;
            }
        }
        suffix_logprobs.reserve(suffix.size());
        for (const ninfer::ScoredTarget& target : suffix) {
            suffix_logprobs.push_back(target.logprob);
        }
    }

    // Enabling the top-K must not disturb the target log probabilities: the selection shares the
    // target op's per-column normalization, so the same window returns the identical floats. It
    // must also produce a descending, uniquely identified distribution whose entries agree bit for
    // bit with the target log probability whenever the target token is a member.
    options.score_topk = 8;
    {
        ninfer::Engine topk_engine(options);
        const std::vector<ninfer::ScoredTarget> scored =
            topk_engine.score_tokens(tokens, second_start);
        if (scored.size() != suffix_logprobs.size()) {
            std::cerr << "top-K causal scoring returned an invalid result shape\n";
            return 1;
        }
        for (std::size_t i = 0; i < scored.size(); ++i) {
            const ninfer::ScoredTarget& target = scored[i];
            if (target.logprob != suffix_logprobs[i]) {
                std::cerr << "top-K scoring changed a target log probability\n";
                return 1;
            }
            if (target.topk.size() != 8) {
                std::cerr << "top-K scoring returned the wrong distribution width\n";
                return 1;
            }
            if (target.topk[0].logprob < target.logprob) {
                std::cerr << "top-K scoring ranked the target above the selected maximum\n";
                return 1;
            }
            const ninfer::TokenId expected_target = tokens[second_start + i];
            for (std::size_t rank = 0; rank < target.topk.size(); ++rank) {
                const ninfer::ScoreTopKEntry& entry = target.topk[rank];
                if (entry.token < 0 || !std::isfinite(entry.logprob) || entry.logprob > 0.0F) {
                    std::cerr << "top-K scoring returned an invalid distribution entry\n";
                    return 1;
                }
                if (rank != 0 && entry.logprob > target.topk[rank - 1].logprob) {
                    std::cerr << "top-K scoring returned an unordered distribution\n";
                    return 1;
                }
                if (rank != 0 && entry.token == target.topk[rank - 1].token) {
                    std::cerr << "top-K scoring returned a repeated token\n";
                    return 1;
                }
                if (entry.token == expected_target && entry.logprob != target.logprob) {
                    std::cerr << "the target token's top-K and target log probabilities "
                                 "disagree\n";
                    return 1;
                }
            }
        }
    }
    std::cout << "OK causal_score_real\n";
    return 0;
}

} // namespace

NINFER_GUARDED_TEST_MAIN(run)
