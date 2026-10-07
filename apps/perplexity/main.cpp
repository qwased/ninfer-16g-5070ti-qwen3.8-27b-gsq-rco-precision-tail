#include "ninfer_build_id.h"
#include "corpus.h"
#include "evaluation.h"

#include "ninfer/engine.h"
#include "product/logging/engine_diagnostics.h"
#include "product/logging/logging.h"
#include "product/logging/pretty_format.h"
#include "product/logging/startup_log.h"
#include "product/rope_yarn_options.h"

#include <nlohmann/json.hpp>
#include <spdlog/logger.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using json  = nlohmann::json;
using ninfer::perplexity::CorpusSelection;
using ninfer::perplexity::KldAccumulator;
using ninfer::perplexity::ScoreAggregate;
using ninfer::perplexity::TopKReferenceProtocol;
using ninfer::perplexity::WindowPlan;

// Default top-K width of the KLD instrument: llama.cpp's --kl-divergence uses 100.
constexpr std::uint32_t kDefaultScoreTopK = 100;

struct Options {
    bool help_requested = false;
    std::filesystem::path artifact;
    std::optional<std::filesystem::path> corpus;
    std::optional<std::filesystem::path> text;
    std::optional<std::filesystem::path> output;
    std::uint32_t context     = 4096;
    std::uint32_t stride      = 2048;
    // Attention query tile used while scoring. Zero keeps the planned prefill chunk (1024), which
    // scores through the prompt attention route; a small value (<=8) selects the small-T decode
    // route that merges the exact KV tail. Preserves the scored token count and ordering.
    std::uint32_t score_width = 0;
    // Top-K width of the next-token distribution each scored target additionally carries. Zero
    // means "as requested by the flags below": off without them, kDefaultScoreTopK with either.
    std::uint32_t score_topk = 0;
    // Persist this run's per-target top-K distribution as a KLD reference.
    std::optional<std::filesystem::path> save_topk;
    // Compare this run against a persisted reference and report KLD statistics.
    std::optional<std::filesystem::path> kld_base;
    bool disjoint             = false;
    int device                = 0;
#if defined(NINFER_SM8X_COMPAT)
    // FP8 E4M3 KV attention has no SM86 implementation, so the upstream default would fail at
    // engine construction on this fork. INT8 group-64 is the qualified quantized profile here.
    ninfer::KvCacheStorage kv = ninfer::KvCacheStorage::Int8Group64;
#else
    ninfer::KvCacheStorage kv = ninfer::KvCacheStorage::Fp8E4M3Row256;
#endif
    // KVarN code width, meaningful only when kv is KvarnGroup128.
    ninfer::KvarnBits kvarn_bits = ninfer::KvarnBits::Bits4;
    // Exact KV tail: the newest N tokens per sequence stay unquantized and attention merges an exact
    // tail partial with the quantized body. Zero disables it; the ring element type is F16 or BF16
    // (see --kv-tail-type), independent of the body coding.
    std::int32_t kv_tail_tokens      = 0;
    ninfer::KvTailType kv_tail_type  = ninfer::KvTailType::Float16;
    bool quick                  = false;
    bool lm_head_q4                     = false;
    bool lm_head_q6                     = false;
    bool embedding_q4                   = false;
    bool embedding_q6                   = false;
    bool mtp_experts_q4                 = false;
    bool gdn_state_fp16                 = false;
    bool rope_yarn                      = false;
    float rope_yarn_factor              = 1.0F;
    float rope_scaling_factor                   = 1.0F;
    std::uint32_t rope_scaling_original_context = 0;
    bool mlp_a8_decode                  = false;
    bool prefill_a8                     = true;
    bool prefill_cublas                 = false;
    bool prefill_cublas_projections     = true;
    bool fast_prefill_kernel            = false;
    ninfer::product::LogLevel log_level = ninfer::product::LogLevel::Info;
};

std::string usage_text() {
    return "usage: ninfer-perplexity <model.ninfer> "
           "(--corpus <manifest.json> [--quick] | --text <utf8-file>)\n"
           "       [--context N] [--stride N | --disjoint] [--device N]\n"
           "       [--score-width W]\n"
           "       (--score-width W scores in width-W attention query tiles; W<=8 selects the\n"
           "        small-T decode route that merges the exact KV tail, so the tail's on/off\n"
           "        difference becomes visible. Default 1024 = the prompt route)\n"
           "       [--score-topk K] [--save-topk <path>] [--kld-base <path>]\n"
           "       (--score-topk K materializes each scored target's K most probable next tokens\n"
           "        with their log probabilities. Either of the other two flags enables it at K=100\n"
           "        by default: --save-topk persists this run's per-target top-K distribution as a\n"
           "        KLD reference, and --kld-base loads one and reports KLD against it, over the\n"
           "        union of the two top-K sets plus the target token, both sides renormalized over\n"
           "        that support, with a token missing from one side floored at that side's K-th log\n"
           "        probability. Only incremental KLD between runs has meaning)\n"
           "       [--kv-dtype bf16|int8|fp8|rk8v4|rk4v4|rk4v4-e8|rk2v4-e8|nvfp4|k8v4|\n"
           "                   kvarn:k4v4|k5v5|k6v6] [--output "
           "<directory>]\n"
           "       [--kv-tail-tokens N] [--kv-tail-type bf16|f16]\n"
           "       [--lm-head-q4|--lm-head-q6] [--embedding-q4|--embedding-q6] [--mtp-experts-q4] "
           "[--gdn-state-fp16]\n"
           "       [--mlp-a8-decode] [--no-prefill-a8] [--rope-yarn] [--rope-yarn-factor F]\n"
           "       [--rope-scaling-factor F [--rope-scaling-original-context N]]\n"
           "       (--mlp-a8-decode is inert here: the route it enables is verify-phase"
           "        only, and scoring runs the prefill phase)\n"
           "       (--no-prefill-a8 is the opposite: scoring runs the prefill phase, so this is\n"
           "        how the integer prefill routes' perplexity cost is measured)\n"
           "       [--prefill-cublas [--no-prefill-cublas-projections]] [--fast-prefill-kernel]\n"
           "       (--prefill-cublas scores through the cuBLAS prefill route, which is how its\n"
           "        perplexity cost is measured; the projections flag keeps the attention and GDN\n"
           "        input projections off it)\n"
           "       [--log-level trace|debug|info|warning|error|critical|off]\n";
}

[[noreturn]] void usage_error(std::string_view message) {
    throw std::invalid_argument(std::string(message));
}

template <class Integer>
Integer parse_integer(std::string_view text, const char* label) {
    Integer value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        usage_error(std::string("invalid ") + label + ": " + std::string(text));
    }
    return value;
}

Options parse_options(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        return Options{.help_requested = true};
    }
    if (argc < 2 || std::string_view(argv[1]).starts_with("--")) {
        usage_error("artifact path is required");
    }
    Options out;
    out.artifact = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string_view option = argv[i];
        const auto value              = [&](const char* label) -> std::string_view {
            if (++i >= argc) { usage_error(std::string(label) + " requires a value"); }
            return argv[i];
        };
        if (option == "--corpus") {
            out.corpus = std::filesystem::path(value("--corpus"));
        } else if (option == "--text") {
            out.text = std::filesystem::path(value("--text"));
        } else if (option == "--quick") {
            out.quick = true;
        } else if (option == "--context") {
            out.context = parse_integer<std::uint32_t>(value("--context"), "context");
        } else if (option == "--stride") {
            out.stride = parse_integer<std::uint32_t>(value("--stride"), "stride");
        } else if (option == "--score-width") {
            out.score_width = parse_integer<std::uint32_t>(value("--score-width"), "score-width");
            if (out.score_width == 0) { usage_error("--score-width must be positive"); }
        } else if (option == "--score-topk") {
            out.score_topk = parse_integer<std::uint32_t>(value("--score-topk"), "score-topk");
            if (out.score_topk == 0 ||
                out.score_topk > static_cast<std::uint32_t>(ninfer::kMaxScoreTopK)) {
                usage_error("--score-topk must be in [1,kMaxScoreTopK]");
            }
        } else if (option == "--save-topk") {
            out.save_topk = std::filesystem::path(value("--save-topk"));
        } else if (option == "--kld-base") {
            out.kld_base = std::filesystem::path(value("--kld-base"));
        } else if (option == "--disjoint") {
            out.disjoint = true;
        } else if (option == "--device") {
            out.device = parse_integer<int>(value("--device"), "device");
        } else if (option == "--fast-prefill-kernel") {
            out.fast_prefill_kernel = true;
        } else if (option == "--kv-dtype") {
            const std::string_view dtype = value("--kv-dtype");
            if (dtype == "bf16") {
                out.kv = ninfer::KvCacheStorage::BFloat16;
            } else if (dtype == "int8") {
                out.kv = ninfer::KvCacheStorage::Int8Group64;
            } else if (dtype == "fp8") {
                out.kv = ninfer::KvCacheStorage::Fp8E4M3Row256;
            } else if (dtype == "rk8v4") {
                out.kv = ninfer::KvCacheStorage::RotatedInt8KeyInt4ValueGroup64;
            } else if (dtype == "rk4v4") {
                out.kv = ninfer::KvCacheStorage::RotatedLloyd4KeyInt4Value;
            } else if (dtype == "rk4v4-e8") {
                out.kv = ninfer::KvCacheStorage::RotatedInt4KeyInt4ValueE8;
            } else if (dtype == "rk2v4-e8") {
                out.kv = ninfer::KvCacheStorage::RotatedE8RootKeyInt4Value;
            } else if (dtype == "nvfp4") {
                out.kv = ninfer::KvCacheStorage::Nvfp4Group16;
            } else if (dtype == "k8v4") {
                out.kv = ninfer::KvCacheStorage::Fp8KeyNvfp4Value;
            } else if (dtype == "kvarn" || dtype == "kvarn:k4v4") {
                out.kvarn_bits = ninfer::KvarnBits::Bits4;
                out.kv         = ninfer::KvCacheStorage::KvarnGroup128;
            } else if (dtype == "kvarn:k5v5") {
                out.kvarn_bits = ninfer::KvarnBits::Bits5;
                out.kv         = ninfer::KvCacheStorage::KvarnGroup128;
            } else if (dtype == "kvarn:k6v6") {
                out.kvarn_bits = ninfer::KvarnBits::Bits6;
                out.kv         = ninfer::KvCacheStorage::KvarnGroup128;
            } else {
                usage_error("--kv-dtype must be bf16, int8, fp8, rk8v4, rk4v4, rk4v4-e8, "
                            "rk2v4-e8, nvfp4, k8v4, or kvarn:k4v4|k5v5|k6v6");
            }
        } else if (option == "--kv-tail-tokens") {
            out.kv_tail_tokens =
                parse_integer<std::int32_t>(value("--kv-tail-tokens"), "kv-tail-tokens");
            if (out.kv_tail_tokens < 0) { usage_error("--kv-tail-tokens must be non-negative"); }
        } else if (option == "--kv-tail-type") {
            const std::string_view dtype = value("--kv-tail-type");
            if (dtype == "bf16") {
                out.kv_tail_type = ninfer::KvTailType::BFloat16;
            } else if (dtype == "f16") {
                out.kv_tail_type = ninfer::KvTailType::Float16;
            } else {
                usage_error("--kv-tail-type must be bf16 or f16");
            }
        } else if (option == "--output") {
            out.output = std::filesystem::path(value("--output"));
        } else if (option == "--lm-head-q4") {
            out.lm_head_q4 = true;
        } else if (option == "--lm-head-q6") {
            out.lm_head_q6 = true;
        } else if (option == "--embedding-q4") {
            out.embedding_q4 = true;
        } else if (option == "--embedding-q6") {
            out.embedding_q6 = true;
        } else if (option == "--mtp-experts-q4") {
            out.mtp_experts_q4 = true;
        } else if (option == "--gdn-state-fp16") {
            out.gdn_state_fp16 = true;
        } else if (option == "--rope-yarn") {
            out.rope_yarn = true;
        } else if (option == "--rope-yarn-factor") {
            out.rope_yarn_factor =
                ninfer::product::parse_rope_yarn_factor(value("--rope-yarn-factor"));
        } else if (option == "--rope-scaling-factor") {
            out.rope_scaling_factor =
                ninfer::product::parse_rope_scaling_factor(value("--rope-scaling-factor"));
        } else if (option == "--rope-scaling-original-context") {
            out.rope_scaling_original_context =
                ninfer::product::parse_rope_scaling_original_context(
                    value("--rope-scaling-original-context"));
        } else if (option == "--mlp-a8-decode") {
            out.mlp_a8_decode = true;
        } else if (option == "--no-prefill-a8") {
            out.prefill_a8 = false;
        } else if (option == "--prefill-cublas") {
            out.prefill_cublas = true;
        } else if (option == "--no-prefill-cublas-projections") {
            out.prefill_cublas_projections = false;
        } else if (option == "--log-level") {
            out.log_level = ninfer::product::parse_log_level(value("--log-level"));
        } else {
            usage_error("unknown option: " + std::string(option));
        }
    }
    if (out.corpus.has_value() == out.text.has_value()) {
        usage_error("exactly one of --corpus and --text is required");
    }
    if (out.quick && !out.corpus) { usage_error("--quick requires --corpus"); }
    if (out.context < 2 ||
        (!out.disjoint && (out.stride == 0 || out.stride >= out.context))) {
        usage_error("context/stride must satisfy context>=2 and 1<=stride<context");
    }
    // Both reference flags need a distribution per target; the default width matches llama.cpp's
    // --kl-divergence.
    if (out.score_topk == 0 && (out.save_topk.has_value() || out.kld_base.has_value())) {
        out.score_topk = kDefaultScoreTopK;
    }
    return out;
}

std::string kv_name(ninfer::KvCacheStorage value, ninfer::KvarnBits kvarn_bits) {
    switch (value) {
    case ninfer::KvCacheStorage::BFloat16:
        return "bf16";
    case ninfer::KvCacheStorage::Int8Group64:
        return "int8-g64";
    case ninfer::KvCacheStorage::Fp8E4M3Row256:
        return "fp8-e4m3-r256";
    case ninfer::KvCacheStorage::RotatedInt8KeyInt4ValueGroup64:
        return "rotated-k8g64-v4g32";
    case ninfer::KvCacheStorage::RotatedLloyd4KeyInt4Value:
        return "rotated-lloyd4g64-v4g32";
    case ninfer::KvCacheStorage::RotatedInt4KeyInt4ValueE8:
        return "rotated-k4e8g64-v4g32";
    case ninfer::KvCacheStorage::RotatedE8RootKeyInt4Value:
        return "rotated-k2e8g64-v4g32";
    case ninfer::KvCacheStorage::Nvfp4Group16:
        return "nvfp4";
    case ninfer::KvCacheStorage::Fp8KeyNvfp4Value:
        return "k8v4";
    case ninfer::KvCacheStorage::KvarnGroup128:
        switch (kvarn_bits) {
        case ninfer::KvarnBits::Bits4:
            return "kvarn:k4v4";
        case ninfer::KvarnBits::Bits5:
            return "kvarn:k5v5";
        case ninfer::KvarnBits::Bits6:
            return "kvarn:k6v6";
        }
        return "kvarn";
    }
    throw std::logic_error("unknown KV dtype");
}

std::string safe_component(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const unsigned char c : value) {
        out.push_back(std::isalnum(c) || c == '-' || c == '_' || c == '.' ? static_cast<char>(c)
                                                                          : '-');
    }
    return out.empty() ? "unknown" : out;
}

// gmtime_r is POSIX. MSVC provides gmtime_s with the destination first, the reverse of the
// POSIX argument order, so the two cannot be swapped by macro alone.
bool to_utc(const std::time_t& source, std::tm& out) {
#ifdef _WIN32
    return ::gmtime_s(&out, &source) == 0;
#else
    return ::gmtime_r(&source, &out) != nullptr;
#endif
}

std::string timestamp() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm utc{};
    if (!to_utc(now, utc)) { throw std::runtime_error("failed to convert timestamp to UTC"); }
    std::ostringstream out;
    out << std::put_time(&utc, "%Y%m%d-%H%M%S");
    return out.str();
}

std::filesystem::path prepare_output_directory(const Options& options,
                                               const ninfer::LoadSummary& load,
                                               const CorpusSelection& corpus) {
    std::filesystem::path output = options.output.value_or(
        std::filesystem::path("profiles/perplexity") / safe_component(load.model_name) /
        safe_component(load.prefill_signature) /
        safe_component(kv_name(options.kv, options.kvarn_bits)) /
        safe_component(corpus.corpus_id) / safe_component(corpus.mode) / timestamp());
    if (std::filesystem::exists(output)) {
        if (!std::filesystem::is_directory(output) ||
            std::filesystem::directory_iterator(output) != std::filesystem::directory_iterator()) {
            throw std::runtime_error("output directory exists and is not empty: " +
                                     output.string());
        }
    } else if (!std::filesystem::create_directories(output)) {
        throw std::runtime_error("cannot create output directory: " + output.string());
    }
    return std::filesystem::absolute(output).lexically_normal();
}

double seconds_since(Clock::time_point begin) {
    return std::chrono::duration<double>(Clock::now() - begin).count();
}

json aggregate_json(const ScoreAggregate& value) {
    return json{{"scored_tokens", value.scored_tokens},
                {"total_nll", value.total_nll},
                {"mean_nll", value.mean_nll()},
                {"perplexity", value.ppl()}};
}

struct EvaluationStream {
    ninfer::perplexity::CorpusStream source;
    std::vector<ninfer::TokenId> tokens;
    std::vector<WindowPlan> windows;
};

int run(const Options& options, const std::shared_ptr<spdlog::logger>& logger,
        ninfer::product::StartupLogRenderer& startup_log,
        const std::shared_ptr<ninfer::product::TerminalProgress>& progress) {
    const Clock::time_point total_started = Clock::now();
    ninfer::EngineOptions engine_options;
    engine_options.artifact_path    = options.artifact;
    engine_options.purpose          = ninfer::EnginePurpose::CausalScoring;
    engine_options.device           = options.device;
    engine_options.max_context      = options.context;
    engine_options.score_width      = options.score_width;
    engine_options.score_topk       = static_cast<int>(options.score_topk);
    engine_options.kv_cache         = options.kv;
    engine_options.kvarn_bits       = options.kvarn_bits;
    engine_options.kv_tail_tokens   = options.kv_tail_tokens;
    engine_options.kv_tail_type     = options.kv_tail_type;
    engine_options.lm_head_q4       = options.lm_head_q4;
    engine_options.lm_head_q6       = options.lm_head_q6;
    engine_options.embedding_q4     = options.embedding_q4;
    engine_options.embedding_q6     = options.embedding_q6;
    engine_options.mtp_experts_q4   = options.mtp_experts_q4;
    engine_options.gdn_state_fp16   = options.gdn_state_fp16;
    engine_options.rope_yarn        = options.rope_yarn;
    engine_options.rope_yarn_factor = options.rope_yarn_factor;
    engine_options.rope_scaling_factor           = options.rope_scaling_factor;
    engine_options.rope_scaling_original_context = options.rope_scaling_original_context;
    engine_options.mlp_a8_decode    = options.mlp_a8_decode;
    engine_options.prefill_a8       = options.prefill_a8;
    engine_options.prefill_cublas   = options.prefill_cublas;
    engine_options.prefill_cublas_projections = options.prefill_cublas_projections;
    engine_options.fast_prefill_kernel        = options.fast_prefill_kernel;
    engine_options.startup_observer = startup_log.observer();
    engine_options.diagnostic_observer        = ninfer::product::engine_diagnostic_observer(logger);
    ninfer::Engine engine(std::move(engine_options));
    const ninfer::LoadSummary load = engine.load_summary();
    startup_log.engine_ready(load);

    const Clock::time_point preflight_started = Clock::now();
    logger->info("preparing corpus");
    CorpusSelection corpus = options.corpus
                                 ? ninfer::perplexity::load_corpus(*options.corpus, options.quick)
                                 : ninfer::perplexity::load_custom_text(*options.text);
    std::vector<EvaluationStream> streams;
    streams.reserve(corpus.streams.size());
    std::uint64_t total_scored_tokens = 0;
    std::uint64_t total_input_tokens  = 0;
    std::uint64_t total_windows       = 0;
    for (auto& source : corpus.streams) {
        std::vector<ninfer::TokenId> tokens = engine.tokenize_text(source.text);
        if (tokens.size() < 2) {
            throw std::runtime_error("stream tokenized to fewer than two tokens: " + source.id);
        }
        std::vector<WindowPlan> windows =
            options.disjoint
                ? ninfer::perplexity::plan_disjoint_windows(tokens.size(), options.context)
                : ninfer::perplexity::plan_windows(tokens.size(), options.context, options.stride);
        total_input_tokens += static_cast<std::uint64_t>(tokens.size());
        for (const WindowPlan& window : windows) {
            total_scored_tokens += static_cast<std::uint64_t>(window.target_end - window.target_begin);
        }
        total_windows += static_cast<std::uint64_t>(windows.size());
        streams.push_back(EvaluationStream{.source  = std::move(source),
                                           .tokens  = std::move(tokens),
                                           .windows = std::move(windows)});
    }
    const double preflight_seconds = seconds_since(preflight_started);
    logger->info("corpus ready | {} streams | {} input tokens | {} scored tokens | {} windows | {}",
                 ninfer::product::format_pretty_count(streams.size()),
                 ninfer::product::format_pretty_count(total_input_tokens),
                 ninfer::product::format_pretty_count(total_scored_tokens),
                 ninfer::product::format_pretty_count(total_windows),
                 ninfer::product::format_pretty_duration(preflight_seconds));

    const std::filesystem::path output_directory = prepare_output_directory(options, load, corpus);
    const std::uint32_t effective_score_width =
        options.score_width == 0 ? 1024 : options.score_width;
    const TopKReferenceProtocol reference_protocol{
        .corpus_id        = corpus.corpus_id,
        .model_name       = load.model_name,
        .kv_dtype         = kv_name(options.kv, options.kvarn_bits),
        .context          = options.context,
        .stride           = options.disjoint ? options.context : options.stride,
        .score_width      = effective_score_width,
        .disjoint_windows = options.disjoint,
        .top_k            = options.score_topk,
        .scored_tokens    = total_scored_tokens,
    };
    // The KLD reference is consumed window by window in scored order, so it is loaded once up
    // front and its protocol verified against this run before anything is scored.
    std::vector<ninfer::ScoredTarget> kld_reference;
    std::size_t kld_cursor = 0;
    KldAccumulator kld;
    if (options.kld_base) {
        kld_reference =
            ninfer::perplexity::load_topk_reference(*options.kld_base, reference_protocol);
        logger->info("KLD reference | {} | {} targets | top-K {}",
                     options.kld_base->string(),
                     ninfer::product::format_pretty_count(kld_reference.size()),
                     options.score_topk);
    }
    // This run's own distribution, collected only to be persisted by --save-topk.
    std::vector<ninfer::ScoredTarget> saved_targets;
    if (options.save_topk) { saved_targets.reserve(total_scored_tokens); }

    const Clock::time_point scoring_started      = Clock::now();
    logger->info("scoring | {} streams | {} tokens | {} windows",
                 ninfer::product::format_pretty_count(streams.size()),
                 ninfer::product::format_pretty_count(total_scored_tokens),
                 ninfer::product::format_pretty_count(total_windows));
    Clock::time_point next_progress = scoring_started + std::chrono::seconds(10);
    ScoreAggregate overall;
    std::map<std::string, ScoreAggregate> domains;
    json stream_reports             = json::array();
    std::uint64_t completed_windows = 0;

    for (std::size_t stream_index = 0; stream_index < streams.size(); ++stream_index) {
        EvaluationStream& stream = streams[stream_index];
        std::ostringstream stream_status;
        stream_status << "  scoring [" << stream_index + 1 << '/' << streams.size() << "] "
                      << ninfer::product::format_pretty_text(stream.source.id) << " | "
                      << ninfer::product::format_pretty_count(stream.tokens.size()) << " tokens | "
                      << ninfer::product::format_pretty_count(stream.windows.size()) << " windows";
        if (progress->enabled()) {
            progress->update(stream_status.str());
        } else {
            logger->debug("{}", stream_status.str());
        }
        const Clock::time_point stream_started = Clock::now();
        ScoreAggregate stream_score;
        json window_reports = json::array();
        for (std::size_t window_index = 0; window_index < stream.windows.size(); ++window_index) {
            const WindowPlan& window = stream.windows[window_index];
            std::vector<ninfer::TokenId> input(
                stream.tokens.begin() + static_cast<std::ptrdiff_t>(window.input_begin),
                stream.tokens.begin() + static_cast<std::ptrdiff_t>(window.input_end));
            const Clock::time_point window_started = Clock::now();
            std::vector<ninfer::ScoredTarget> scored;
            try {
                scored = engine.score_tokens(std::move(input), window.first_target);
            } catch (const std::exception& error) {
                throw std::runtime_error("scoring " + stream.source.id + " window " +
                                         std::to_string(window_index) + " failed: " + error.what());
            }
            const std::size_t expected = window.target_end - window.target_begin;
            if (scored.size() != expected) {
                throw std::runtime_error("scoring returned an invalid target count for " +
                                         stream.source.id);
            }
            ScoreAggregate window_score;
            window_score.add(scored);
            if (options.kld_base) {
                // Target j of this window is the stream token at target_begin + j: that is the
                // token both distributions were conditioned on producing.
                for (std::size_t index = 0; index < expected; ++index) {
                    if (kld_cursor + index >= kld_reference.size()) {
                        throw std::runtime_error("KLD reference ran out of targets");
                    }
                    kld.add(scored[index], kld_reference[kld_cursor + index],
                            stream.tokens[window.target_begin + index]);
                }
                kld_cursor += expected;
            }
            if (options.save_topk) {
                // Last use of this window's targets: they move into the persisted reference.
                saved_targets.insert(saved_targets.end(),
                                     std::make_move_iterator(scored.begin()),
                                     std::make_move_iterator(scored.end()));
            }
            stream_score.add(window_score);
            overall.add(window_score);
            domains[stream.source.domain].add(window_score);
            ++completed_windows;
            json window_report            = aggregate_json(window_score);
            window_report["index"]        = window_index;
            window_report["input_begin"]  = window.input_begin;
            window_report["input_end"]    = window.input_end;
            window_report["target_begin"] = window.target_begin;
            window_report["target_end"]   = window.target_end;
            window_report["first_target"] = window.first_target;
            window_report["seconds"]      = seconds_since(window_started);
            window_reports.push_back(std::move(window_report));

            if (Clock::now() >= next_progress) {
                const double elapsed = seconds_since(scoring_started);
                const double rate    = static_cast<double>(overall.scored_tokens) / elapsed;
                const std::uint64_t remaining = total_scored_tokens - overall.scored_tokens;
                const double eta = rate > 0 ? static_cast<double>(remaining) / rate : 0.0;
                std::ostringstream line;
                line << "scoring | " << ninfer::product::format_pretty_count(overall.scored_tokens)
                     << '/' << ninfer::product::format_pretty_count(total_scored_tokens)
                     << " tokens | " << completed_windows << '/' << total_windows
                     << " windows | PPL " << std::fixed << std::setprecision(4) << overall.ppl()
                     << " | " << ninfer::product::format_pretty_rate(rate, "tok") << " | elapsed "
                     << ninfer::product::format_pretty_duration(elapsed) << " | ETA "
                     << ninfer::product::format_pretty_duration(eta);
                if (progress->enabled()) {
                    progress->update("  " + line.str());
                } else {
                    logger->info("{}", line.str());
                }
                next_progress = Clock::now() + std::chrono::seconds(10);
            }
        }
        const double stream_seconds = seconds_since(stream_started);
        progress->clear();
        logger->info("[{}/{}] {} | {} scored tokens | PPL {:.6g} | {}", stream_index + 1,
                     streams.size(), ninfer::product::format_pretty_text(stream.source.id),
                     ninfer::product::format_pretty_count(stream_score.scored_tokens),
                     stream_score.ppl(), ninfer::product::format_pretty_duration(stream_seconds));
        json stream_report               = aggregate_json(stream_score);
        stream_report["id"]              = stream.source.id;
        stream_report["domain"]          = stream.source.domain;
        stream_report["path"]            = stream.source.path.string();
        stream_report["input_tokens"]    = stream.tokens.size();
        stream_report["unscored_tokens"] = 1;
        stream_report["seconds"]         = stream_seconds;
        stream_report["windows"]         = std::move(window_reports);
        stream_reports.push_back(std::move(stream_report));
    }

    const double scoring_seconds = seconds_since(scoring_started);
    progress->clear();
    logger->info("scoring complete | {} tokens | {} windows | PPL {:.6g} | {} | {}",
                 ninfer::product::format_pretty_count(overall.scored_tokens), completed_windows,
                 overall.ppl(), ninfer::product::format_pretty_duration(scoring_seconds),
                 ninfer::product::format_pretty_rate(
                     static_cast<double>(overall.scored_tokens) / scoring_seconds, "tok"));

    // The top-K instrument. --save-topk persists this run as a future baseline; --kld-base
    // compares this run against a persisted one. Both are gated on the scored positions rather
    // than on the run, and the reference's protocol was checked before scoring began.
    if (options.save_topk) {
        if (saved_targets.size() != total_scored_tokens) {
            throw std::runtime_error("top-K reference does not cover every scored token");
        }
        ninfer::perplexity::save_topk_reference(*options.save_topk, reference_protocol,
                                                saved_targets);
        logger->info("top-K reference saved | {} | {} targets | top-K {}",
                     options.save_topk->string(),
                     ninfer::product::format_pretty_count(saved_targets.size()),
                     options.score_topk);
    }
    std::optional<ninfer::perplexity::KldSummary> kld_summary;
    if (options.kld_base) {
        if (kld_cursor != kld_reference.size()) {
            throw std::runtime_error("KLD reference has unused targets");
        }
        kld_summary = kld.summary();
        logger->info("KLD vs {} | {} targets | median {:.6g} | mean {:.6g} | same-top {:.4f}",
                     options.kld_base->string(), kld_summary->targets, kld_summary->median,
                     kld_summary->mean, kld_summary->same_top);
    }

    json domain_reports = json::array();
    for (const auto& [domain, aggregate] : domains) {
        json item      = aggregate_json(aggregate);
        item["domain"] = domain;
        domain_reports.push_back(std::move(item));
    }

    const ninfer::MemorySummary memory = engine.memory_summary();
    json kld_report = nullptr;
    if (kld_summary) {
        kld_report = json{
            {"targets", kld_summary->targets},
            {"top_k", options.score_topk},
            {"support", "union of both top-K sets plus the target token"},
            {"direction", "KLD(candidate || reference)"},
            {"median", kld_summary->median},
            {"mean", kld_summary->mean},
            {"p99", kld_summary->p99},
            {"p99_9", kld_summary->p999},
            {"max", kld_summary->maximum},
            {"same_top", kld_summary->same_top},
            {"mean_target_logprob_delta", kld_summary->mean_target_logprob_delta},
        };
    }
    json report{
        {"schema_version", 5},
        {"metric",
         {{"name", "fixed-window truncated-context causal perplexity"}, {"log_base", "natural"}}},
        {"artifact",
         {{"path", std::filesystem::absolute(options.artifact).lexically_normal().string()},
          {"architecture", load.architecture},
          {"name", load.model_name},
          {"prefill_signature", load.prefill_signature},
          {"formats", load.weight_formats}}},
        {"corpus",
         {{"id", corpus.corpus_id},
          {"mode", corpus.mode},
          {"source", corpus.source.string()},
          {"stream_count", streams.size()}}},
        {"memory",
         {{"runtime_reservation_bytes", memory.runtime_reservation_bytes},
          {"kv_payload_bytes", memory.kv_payload_bytes},
          {"kv_exact_history_bytes", memory.kv_exact_history_bytes},
          {"kv_rollback_reserve_bytes", memory.kv_rollback_reserve_bytes},
          {"minimum_runtime_reservation_bytes", memory.minimum_runtime_reservation_bytes},
          {"cuda_graph_measured_bytes", memory.cuda_graph_measured_bytes}}},
        {"execution",
         {{"purpose", "causal_scoring"},
          {"device", options.device},
          {"context_tokens", options.context},
          {"rope_yarn", options.rope_yarn},
          {"rope_yarn_factor", options.rope_yarn_factor},
          {"rope_scaling_factor", options.rope_scaling_factor},
          {"rope_scaling_original_context", options.rope_scaling_original_context},
          {"fast_prefill_kernel", options.fast_prefill_kernel},
          {"stride_tokens", options.disjoint ? options.context : options.stride},
          {"windows", options.disjoint ? "disjoint" : "sliding"},
          {"prefill_chunk_tokens", 1024},
          {"score_tile_tokens", 1024},
          {"score_width_tokens", options.score_width == 0 ? 1024 : options.score_width},
          {"score_topk_tokens", options.score_topk},
          {"kv_dtype", kv_name(options.kv, options.kvarn_bits)},
          {"kv_tail_tokens", options.kv_tail_tokens},
          {"kv_tail_type", options.kv_tail_type == ninfer::KvTailType::BFloat16 ? "bf16" : "f16"}}},
        {"timing",
         {{"load_seconds", load.load_seconds},
          {"read_and_tokenize_seconds", preflight_seconds},
          {"score_seconds", scoring_seconds},
          {"total_seconds", seconds_since(total_started)},
          {"scored_tokens_per_second",
           static_cast<double>(overall.scored_tokens) / scoring_seconds}}},
        {"streams", std::move(stream_reports)},
        {"domains", std::move(domain_reports)},
        {"overall", aggregate_json(overall)},
        {"topk",
         {{"enabled", options.score_topk != 0},
          {"top_k", options.score_topk},
          {"reference_saved",
           options.save_topk ? options.save_topk->string() : std::string()},
          {"reference_loaded",
           options.kld_base ? options.kld_base->string() : std::string()}}},
        {"kld", kld_report},
    };

    const std::filesystem::path temporary = output_directory / "report.json.tmp";
    const std::filesystem::path final     = output_directory / "report.json";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) { throw std::runtime_error("cannot create report: " + temporary.string()); }
        output << std::setw(2) << report << '\n';
        output.flush();
        if (!output) { throw std::runtime_error("cannot write report: " + temporary.string()); }
    }
    std::filesystem::rename(temporary, final);

    std::cout << "Perplexity result\n"
              << "artifact: " << load.model_name << '\n'
              << "kv: " << kv_name(options.kv, options.kvarn_bits) << ", corpus: " << corpus.corpus_id << " / "
              << corpus.mode << ", context/stride: " << options.context << '/'
              << (options.disjoint ? options.context : options.stride)
              << (options.disjoint ? " (disjoint windows)" : "") << ", score-width: "
              << (options.score_width == 0 ? 1024 : options.score_width) << '\n';
    if (options.score_topk != 0) {
        std::cout << "top-K: " << options.score_topk << " tokens per scored target";
        if (options.save_topk) { std::cout << ", reference saved to " << *options.save_topk; }
        if (options.kld_base) { std::cout << ", reference loaded from " << *options.kld_base; }
        std::cout << '\n';
    }
    std::cout << '\n';
    std::cout << std::left << std::setw(24) << "domain" << std::right << std::setw(16) << "tokens"
              << std::setw(16) << "mean_nll" << std::setw(16) << "ppl" << '\n';
    for (const auto& [domain, aggregate] : domains) {
        std::cout << std::left << std::setw(24) << domain << std::right << std::setw(16)
                  << aggregate.scored_tokens << std::setw(16) << std::fixed << std::setprecision(6)
                  << aggregate.mean_nll() << std::setw(16) << aggregate.ppl() << '\n';
    }
    std::cout << std::left << std::setw(24) << "overall" << std::right << std::setw(16)
              << overall.scored_tokens << std::setw(16) << std::fixed << std::setprecision(6)
              << overall.mean_nll() << std::setw(16) << overall.ppl() << "\n\n";
    if (kld_summary) {
        std::cout << "KLD(candidate || reference) over " << kld_summary->targets
                  << " targets, top-K " << options.score_topk << '\n'
                  << "support: union of both top-K sets plus the target token, both sides\n"
                  << "         renormalized over it; a missing side value is floored at that\n"
                  << "         side's top-K minimum. Only incremental KLD has meaning.\n"
                  << std::left << std::setw(24) << "median" << std::right << std::setw(16)
                  << std::fixed << std::setprecision(6) << kld_summary->median << '\n'
                  << std::left << std::setw(24) << "mean" << std::right << std::setw(16)
                  << kld_summary->mean << '\n'
                  << std::left << std::setw(24) << "P99" << std::right << std::setw(16)
                  << kld_summary->p99 << '\n'
                  << std::left << std::setw(24) << "P99.9" << std::right << std::setw(16)
                  << kld_summary->p999 << '\n'
                  << std::left << std::setw(24) << "max" << std::right << std::setw(16)
                  << kld_summary->maximum << '\n'
                  << std::left << std::setw(24) << "same_top" << std::right << std::setw(16)
                  << std::setprecision(4) << kld_summary->same_top << '\n'
                  << std::left << std::setw(24) << "mean_target_dlogp" << std::right
                  << std::setw(16) << std::setprecision(6)
                  << kld_summary->mean_target_logprob_delta << "\n\n";
    }
    std::cout << "score rate: " << std::setprecision(1)
              << static_cast<double>(overall.scored_tokens) / scoring_seconds << " tok/s\n"
              << "report: " << final << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    try {
        options = parse_options(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "ninfer-perplexity: " << error.what() << '\n';
        std::cerr << usage_text();
        return 1;
    }
    if (options.help_requested) {
        std::cout << usage_text();
        return 0;
    }

    ninfer::product::LoggingRuntime logging(
        {.logger_name  = "ninfer-perplexity",
         .level        = options.log_level,
         .presentation = ninfer::product::LogPresentation::Tool});
    const std::shared_ptr<spdlog::logger> logger = logging.logger();
    logger->info("build {}", NINFER_BUILD_ID);
    ninfer::product::StartupLogRenderer startup_log(logging);
    try {
        return run(options, logger, startup_log, logging.terminal_progress());
    } catch (const std::exception& error) {
        logging.terminal_progress()->clear();
        logger->error("{}", ninfer::product::format_pretty_text(error.what()));
        return 1;
    }
}
