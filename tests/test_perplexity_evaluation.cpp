#include "evaluation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int require(bool condition, const char* label) {
    if (condition) { return 0; }
    std::cerr << "FAIL: " << label << '\n';
    return 1;
}

int check_partition(std::size_t tokens, std::uint32_t context, std::uint32_t stride) {
    const auto windows   = ninfer::perplexity::plan_windows(tokens, context, stride);
    int failures         = 0;
    std::size_t frontier = 1;
    for (const auto& window : windows) {
        failures += require(window.input_begin < window.input_end, "window input is nonempty");
        failures +=
            require(window.input_end - window.input_begin <= context, "window respects context");
        failures += require(window.target_begin == frontier, "target ranges are contiguous");
        failures +=
            require(window.target_end <= window.input_end, "targets are inside input window");
        failures += require(window.first_target == window.target_begin - window.input_begin,
                            "local target matches global range");
        frontier = window.target_end;
    }
    failures += require(frontier == tokens, "targets cover every token after the first");
    return failures;
}

ninfer::ScoredTarget scored(float target_logprob, std::vector<ninfer::ScoreTopKEntry> topk) {
    ninfer::ScoredTarget target;
    target.logprob = target_logprob;
    target.topk    = std::move(topk);
    return target;
}

// The KLD contract: identical distributions give zero divergence and a perfect Same-top%; the
// support is the union of both top-K sets plus the target token; a token a side did not select is
// floored at that side's least probable top-K entry; the aggregates report the mean target-token
// log-probability difference and the fraction of matching argmax tokens.
int check_kld() {
    int failures = 0;
    {
        ninfer::perplexity::KldAccumulator kld;
        const ninfer::ScoredTarget reference =
            scored(std::log(0.5F), {{5, std::log(0.5F)}, {6, std::log(0.5F)}});
        kld.add(reference, reference, 5);
        kld.add(reference, reference, 6);
        const auto summary = kld.summary();
        failures += require(summary.targets == 2, "KLD counts every compared target");
        failures += require(summary.mean == 0.0 && summary.maximum == 0.0,
                            "identical distributions have zero divergence");
        failures += require(summary.same_top == 1.0, "identical distributions match at the top");
        failures += require(summary.mean_target_logprob_delta == 0.0,
                            "identical distributions have no target logprob shift");
    }
    {
        // Two-token support with known probabilities: p = (0.75, 0.25), q = (0.5, 0.5). The
        // target token is already the candidate's argmax, so it adds nothing to the union.
        ninfer::perplexity::KldAccumulator kld;
        const ninfer::ScoredTarget candidate =
            scored(std::log(0.75F), {{0, std::log(0.75F)}, {1, std::log(0.25F)}});
        const ninfer::ScoredTarget reference =
            scored(std::log(0.5F), {{0, std::log(0.5F)}, {1, std::log(0.5F)}});
        kld.add(candidate, reference, 0);
        const auto summary = kld.summary();
        const double expected = 0.75 * std::log(0.75 / 0.5) + 0.25 * std::log(0.25 / 0.5);
        failures += require(std::abs(summary.median - expected) < 1e-6,
                            "KLD matches the closed form over the declared support");
        failures += require(std::abs(summary.mean - expected) < 1e-6, "KLD mean matches the median");
        failures += require(summary.same_top == 1.0, "a shared argmax counts as Same-top");
        failures += require(std::abs(summary.mean_target_logprob_delta - std::log(1.5)) < 1e-6,
                            "the target logprob delta is the difference the runs produced");
    }
    {
        // Disjoint selections plus a target token outside both: the union holds four tokens, each
        // side's missing token falls back to its own top-K minimum, and the argmaxes differ.
        ninfer::perplexity::KldAccumulator kld;
        const ninfer::ScoredTarget candidate =
            scored(std::log(0.05F), {{0, std::log(0.6F)}, {1, std::log(0.4F)}});
        const ninfer::ScoredTarget reference =
            scored(std::log(0.02F), {{2, std::log(0.9F)}, {3, std::log(0.1F)}});
        kld.add(candidate, reference, 4);
        const auto summary = kld.summary();
        // Support {0,1,2,3,4}: candidate (0.6, 0.4, 0.4, 0.4, 0.05) and reference
        // (0.1, 0.1, 0.9, 0.1, 0.02) before renormalization.
        const std::array<double, 5> candidate_mass{0.6, 0.4, 0.4, 0.4, 0.05};
        const std::array<double, 5> reference_mass{0.1, 0.9, 0.1, 0.1, 0.02};
        double candidate_total = 0.0;
        double reference_total = 0.0;
        for (const double mass : candidate_mass) { candidate_total += mass; }
        for (const double mass : reference_mass) { reference_total += mass; }
        double expected = 0.0;
        for (std::size_t index = 0; index < candidate_mass.size(); ++index) {
            const double p = candidate_mass[index] / candidate_total;
            const double q = reference_mass[index] / reference_total;
            expected += p * std::log(p / q);
        }
        failures += require(std::abs(summary.mean - expected) < 1e-6,
                            "KLD uses the union support and the per-side top-K floor");
        failures += require(summary.same_top == 0.0, "differing argmax tokens are not Same-top");
    }
    {
        // Two targets, one identical and one reversed: the reported statistics must be ordered and
        // Same-top% must average over the compared targets.
        ninfer::perplexity::KldAccumulator kld;
        kld.add(scored(std::log(0.5F), {{0, std::log(0.5F)}, {1, std::log(0.5F)}}),
                scored(std::log(0.5F), {{0, std::log(0.5F)}, {1, std::log(0.5F)}}), 0);
        kld.add(scored(std::log(0.5F), {{0, std::log(0.9F)}, {1, std::log(0.1F)}}),
                scored(std::log(0.5F), {{1, std::log(0.9F)}, {0, std::log(0.1F)}}), 0);
        const auto summary = kld.summary();
        failures += require(summary.targets == 2, "the statistics cover every compared target");
        failures += require(summary.median <= summary.p99 && summary.p99 <= summary.maximum &&
                                summary.maximum <= summary.mean * 2.0,
                            "median, P99 and max are ordered over the collected values");
        failures += require(summary.same_top == 0.5, "Same-top averages over the compared targets");
    }
    return failures;
}

// The reference file must round-trip the protocol and every distribution exactly, and must refuse
// a comparison whose protocol differs: a KLD is only meaningful over the same scored positions.
int check_reference_round_trip() {
    using ninfer::perplexity::TopKReferenceProtocol;
    int failures = 0;
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "ninfer-perplexity-topk-reference-test.bin";
    std::vector<ninfer::ScoredTarget> targets{
        scored(std::log(0.5F), {{5, std::log(0.5F)}, {6, std::log(0.5F)}}),
        scored(std::log(0.2F), {{9, std::log(0.8F)}, {7, std::log(0.2F)}}),
    };
    const TopKReferenceProtocol protocol{.corpus_id        = "test-corpus",
                                        .model_name       = "test-model",
                                        .kv_dtype         = "bf16",
                                        .context          = 4096,
                                        .stride           = 2048,
                                        .score_width      = 8,
                                        .disjoint_windows = false,
                                        .top_k            = 2,
                                        .scored_tokens    = 2};
    ninfer::perplexity::save_topk_reference(path, protocol, targets);
    const auto loaded = ninfer::perplexity::load_topk_reference(path, protocol);
    failures += require(loaded.size() == targets.size(), "reference keeps every target");
    for (std::size_t index = 0; index < std::min(loaded.size(), targets.size()); ++index) {
        failures += require(loaded[index].logprob == targets[index].logprob,
                            "reference keeps the target log probability exactly");
        failures += require(loaded[index].topk.size() == targets[index].topk.size(),
                            "reference keeps the top-K width");
        for (std::size_t rank = 0;
             rank < std::min(loaded[index].topk.size(), targets[index].topk.size()); ++rank) {
            failures += require(loaded[index].topk[rank].token == targets[index].topk[rank].token &&
                                    loaded[index].topk[rank].logprob ==
                                        targets[index].topk[rank].logprob,
                                "reference keeps the top-K order and values exactly");
        }
    }
    {
        ninfer::perplexity::KldAccumulator kld;
        kld.add(loaded[0], targets[0], 5);
        failures += require(kld.summary().mean == 0.0,
                            "a reference compared with itself has zero divergence");
    }
    {
        TopKReferenceProtocol other = protocol;
        other.scored_tokens         = 3;
        bool rejected               = false;
        try {
            (void)ninfer::perplexity::load_topk_reference(path, other);
        } catch (const std::exception&) { rejected = true; }
        failures += require(rejected, "a reference with a different protocol is refused");
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    return failures;
}

} // namespace

int main() {
    int failures = 0;
    failures += check_partition(2, 4096, 2048);
    failures += check_partition(4096, 4096, 2048);
    failures += check_partition(4097, 4096, 2048);
    failures += check_partition(12001, 4096, 2048);

    const std::array<ninfer::perplexity::WindowPlan, 3> expected{{
        {.input_begin = 0, .input_end = 6, .target_begin = 1, .target_end = 6, .first_target = 1},
        {.input_begin = 2, .input_end = 8, .target_begin = 6, .target_end = 8, .first_target = 4},
        {.input_begin = 4, .input_end = 10, .target_begin = 8, .target_end = 10, .first_target = 4},
    }};
    const auto planned = ninfer::perplexity::plan_windows(10, 6, 2);
    failures += require(planned.size() == expected.size(), "window count matches the protocol");
    for (std::size_t index = 0; index < std::min(planned.size(), expected.size()); ++index) {
        const auto& actual = planned[index];
        const auto& wanted = expected[index];
        failures += require(actual.input_begin == wanted.input_begin &&
                                actual.input_end == wanted.input_end &&
                                actual.target_begin == wanted.target_begin &&
                                actual.target_end == wanted.target_end &&
                                actual.first_target == wanted.first_target,
                            "window boundaries match the protocol");
    }

    const auto disjoint = ninfer::perplexity::plan_disjoint_windows(10, 4);
    failures += require(disjoint.size() == 2, "disjoint windows drop the partial tail");
    for (std::size_t index = 0; index < disjoint.size(); ++index) {
        const auto& window = disjoint[index];
        failures += require(window.input_begin == 4 * index && window.input_end == 4 * index + 4 &&
                                window.target_begin == window.input_begin + 1 &&
                                window.target_end == window.input_end && window.first_target == 1,
                            "disjoint windows score from their second token");
    }

    const std::vector<ninfer::ScoredTarget> first{
        {.logprob = -1.0F, .topk = {}},
        {.logprob = -2.0F, .topk = {}},
    };
    const std::vector<ninfer::ScoredTarget> second{{.logprob = -3.0F, .topk = {}}};
    ninfer::perplexity::ScoreAggregate a;
    ninfer::perplexity::ScoreAggregate b;
    a.add(first);
    b.add(second);
    a.add(b);
    failures += require(a.scored_tokens == 3, "aggregate counts target tokens");
    failures += require(std::abs(a.total_nll - 6.0) < 1e-12, "aggregate accumulates FP64 NLL");
    failures += require(std::abs(a.mean_nll() - 2.0) < 1e-12, "aggregate computes mean NLL");
    failures += require(std::abs(a.ppl() - std::exp(2.0)) < 1e-12, "aggregate computes perplexity");

    failures += check_kld();
    failures += check_reference_round_trip();

    std::cout << (failures == 0 ? "OK" : "FAIL") << " perplexity_evaluation\n";
    return failures == 0 ? 0 : 1;
}
