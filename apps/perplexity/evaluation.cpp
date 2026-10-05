#include "evaluation.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace ninfer::perplexity {

std::vector<WindowPlan> plan_windows(std::size_t tokens, std::uint32_t context,
                                     std::uint32_t stride) {
    if (tokens < 2) { throw std::invalid_argument("perplexity stream must contain two tokens"); }
    if (context < 2 || stride == 0 || stride >= context) {
        throw std::invalid_argument("perplexity requires context>=2 and 1<=stride<context");
    }

    std::vector<WindowPlan> windows;
    std::size_t previous_end = std::min<std::size_t>(tokens, context);
    windows.push_back(WindowPlan{.input_begin  = 0,
                                 .input_end    = previous_end,
                                 .target_begin = 1,
                                 .target_end   = previous_end,
                                 .first_target = 1});
    while (previous_end < tokens) {
        const std::size_t end          = std::min(tokens, previous_end + stride);
        const std::size_t begin        = end > context ? end - context : 0;
        const std::size_t local_target = previous_end - begin;
        if (local_target == 0 || local_target >= end - begin ||
            local_target > std::numeric_limits<std::uint32_t>::max()) {
            throw std::logic_error("perplexity window has an invalid target suffix");
        }
        windows.push_back(WindowPlan{
            .input_begin  = begin,
            .input_end    = end,
            .target_begin = previous_end,
            .target_end   = end,
            .first_target = static_cast<std::uint32_t>(local_target),
        });
        previous_end = end;
    }
    return windows;
}

std::vector<WindowPlan> plan_disjoint_windows(std::size_t tokens, std::uint32_t context) {
    if (context < 2) { throw std::invalid_argument("perplexity requires context>=2"); }
    if (tokens < context) { throw std::invalid_argument("stream is shorter than one window"); }
    std::vector<WindowPlan> windows;
    for (std::size_t begin = 0; begin + context <= tokens; begin += context) {
        windows.push_back(WindowPlan{.input_begin  = begin,
                                     .input_end    = begin + context,
                                     .target_begin = begin + 1,
                                     .target_end   = begin + context,
                                     .first_target = 1});
    }
    return windows;
}

void ScoreAggregate::add(std::span<const ninfer::ScoredTarget> targets) {
    for (const ninfer::ScoredTarget& target : targets) {
        if (!std::isfinite(target.logprob)) {
            throw std::runtime_error("causal scoring returned a non-finite logprob");
        }
        total_nll -= static_cast<double>(target.logprob);
    }
    if (targets.size() > std::numeric_limits<std::uint64_t>::max() - scored_tokens) {
        throw std::overflow_error("perplexity scored-token count overflowed");
    }
    scored_tokens += static_cast<std::uint64_t>(targets.size());
}

void ScoreAggregate::add(const ScoreAggregate& other) noexcept {
    scored_tokens += other.scored_tokens;
    total_nll += other.total_nll;
}

double ScoreAggregate::mean_nll() const {
    if (scored_tokens == 0) { throw std::logic_error("perplexity aggregate is empty"); }
    return total_nll / static_cast<double>(scored_tokens);
}

double ScoreAggregate::ppl() const { return std::exp(mean_nll()); }

namespace {

constexpr char kReferenceMagic[8]              = {'N', 'I', 'N', 'F', 'T', 'O', 'P', 'K'};
constexpr std::uint32_t kReferenceVersion      = 1;
constexpr std::uint32_t kReferenceEndianMarker = 0x01020304U;

// The file is written in the producing host's byte order and carries that marker, so a reference
// from a host of a different order is rejected instead of silently misread.
template <class T>
void write_pod(std::ostream& out, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>, "top-K reference fields must be plain values");
    out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <class T>
void write_plane(std::ostream& out, const std::vector<T>& values) {
    static_assert(std::is_trivially_copyable_v<T>, "top-K reference planes must be plain values");
    if (!values.empty()) {
        out.write(reinterpret_cast<const char*>(values.data()),
                  static_cast<std::streamsize>(values.size() * sizeof(T)));
    }
}

void write_string(std::ostream& out, const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("top-K reference string field is too long");
    }
    write_pod(out, static_cast<std::uint32_t>(value.size()));
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

void read_exact(std::istream& in, void* destination, std::size_t bytes, const char* label,
                const std::filesystem::path& path) {
    in.read(static_cast<char*>(destination), static_cast<std::streamsize>(bytes));
    if (!in) {
        throw std::runtime_error("top-K reference " + path.string() + " is truncated at " + label);
    }
}

template <class T>
T read_pod(std::istream& in, const char* label, const std::filesystem::path& path) {
    static_assert(std::is_trivially_copyable_v<T>, "top-K reference fields must be plain values");
    T value{};
    read_exact(in, &value, sizeof(T), label, path);
    return value;
}

std::string read_string(std::istream& in, const char* label, const std::filesystem::path& path) {
    const std::uint32_t bytes = read_pod<std::uint32_t>(in, label, path);
    if (bytes > (1U << 20U)) {
        throw std::runtime_error("top-K reference " + path.string() +
                                 " has an implausible string field");
    }
    std::string value(bytes, '\0');
    read_exact(in, value.data(), value.size(), label, path);
    return value;
}

struct SupportEntry {
    ninfer::TokenId token   = -1;
    float candidate_logprob = 0.0F;
    float reference_logprob = 0.0F;
};

// The support token's log probability on one side: its own selected entry, its exact target log
// probability when it is the target token, or the floor of that side's least probable entry.
float support_logprob(const ninfer::ScoredTarget& side, ninfer::TokenId token,
                      ninfer::TokenId target_token, float floor_value) {
    for (const ninfer::ScoreTopKEntry& entry : side.topk) {
        if (entry.token == token) { return entry.logprob; }
    }
    if (token == target_token) { return side.logprob; }
    return floor_value;
}

} // namespace

void save_topk_reference(const std::filesystem::path& path, const TopKReferenceProtocol& protocol,
                         std::span<const ninfer::ScoredTarget> targets) {
    if (protocol.top_k == 0) { throw std::invalid_argument("a top-K reference needs top_k > 0"); }
    for (const ninfer::ScoredTarget& target : targets) {
        if (target.topk.size() != protocol.top_k) {
            throw std::invalid_argument("a top-K reference target has the wrong width");
        }
    }
    const std::filesystem::path temporary = path.string() + ".tmp";
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    if (!file) { throw std::runtime_error("cannot create top-K reference: " + temporary.string()); }

    file.write(kReferenceMagic, sizeof(kReferenceMagic));
    write_pod(file, kReferenceVersion);
    write_pod(file, kReferenceEndianMarker);
    write_pod(file, protocol.top_k);
    write_pod(file, std::uint32_t{0}); // flags: reserved, no optional sections
    write_pod(file, protocol.context);
    write_pod(file, protocol.stride);
    write_pod(file, static_cast<std::uint32_t>(protocol.disjoint_windows ? 1U : 0U));
    write_pod(file, protocol.score_width);
    write_pod(file, protocol.scored_tokens);
    write_pod(file, static_cast<std::uint64_t>(targets.size()));
    write_string(file, protocol.corpus_id);
    write_string(file, protocol.model_name);
    write_string(file, protocol.kv_dtype);

    // One record per target, in scored order: the target log probability, the top-K token ids and
    // the top-K log probabilities, each plane in descending log probability order.
    std::vector<float> target_logprobs;
    std::vector<std::int32_t> ids;
    std::vector<float> logprobs;
    target_logprobs.reserve(targets.size());
    ids.reserve(targets.size() * protocol.top_k);
    logprobs.reserve(targets.size() * protocol.top_k);
    for (const ninfer::ScoredTarget& target : targets) {
        target_logprobs.push_back(target.logprob);
        for (const ninfer::ScoreTopKEntry& entry : target.topk) {
            ids.push_back(static_cast<std::int32_t>(entry.token));
            logprobs.push_back(entry.logprob);
        }
    }
    write_plane(file, target_logprobs);
    write_plane(file, ids);
    write_plane(file, logprobs);

    file.flush();
    if (!file) { throw std::runtime_error("cannot write top-K reference: " + temporary.string()); }
    file.close();
    std::error_code error;
    std::filesystem::rename(temporary, path, error);
    if (error) {
        throw std::runtime_error("cannot publish top-K reference " + path.string() + ": " +
                                 error.message());
    }
}

std::vector<ninfer::ScoredTarget> load_topk_reference(const std::filesystem::path& path,
                                                      const TopKReferenceProtocol& expected) {
    std::ifstream file(path, std::ios::binary);
    if (!file) { throw std::runtime_error("cannot open top-K reference: " + path.string()); }

    char magic[8] = {0};
    read_exact(file, magic, sizeof(magic), "magic", path);
    if (std::memcmp(magic, kReferenceMagic, sizeof(magic)) != 0) {
        throw std::runtime_error("not a top-K reference file: " + path.string());
    }
    const std::uint32_t version = read_pod<std::uint32_t>(file, "version", path);
    if (version != kReferenceVersion) {
        throw std::runtime_error("top-K reference " + path.string() +
                                 " has an unsupported version: " + std::to_string(version));
    }
    const std::uint32_t endian_marker =
        read_pod<std::uint32_t>(file, "endianness", path);
    if (endian_marker != kReferenceEndianMarker) {
        throw std::runtime_error("top-K reference " + path.string() +
                                 " was written on a host of a different byte order");
    }
    const std::uint32_t top_k = read_pod<std::uint32_t>(file, "top_k", path);
    (void)read_pod<std::uint32_t>(file, "flags", path);
    const std::uint32_t context = read_pod<std::uint32_t>(file, "context", path);
    const std::uint32_t stride  = read_pod<std::uint32_t>(file, "stride", path);
    const std::uint32_t disjoint_windows =
        read_pod<std::uint32_t>(file, "disjoint_windows", path);
    const std::uint32_t score_width   = read_pod<std::uint32_t>(file, "score_width", path);
    const std::uint64_t scored_tokens = read_pod<std::uint64_t>(file, "scored_tokens", path);
    const std::uint64_t target_count  = read_pod<std::uint64_t>(file, "target_count", path);
    const std::string corpus_id       = read_string(file, "corpus_id", path);
    const std::string model_name      = read_string(file, "model_name", path);
    const std::string kv_dtype        = read_string(file, "kv_dtype", path);

    const auto reject = [&](const char* field) {
        throw std::runtime_error("top-K reference " + path.string() + " was scored under a "
                                 "different " + field +
                                 "; KLD requires the same corpus, context, stride, score width "
                                 "and protocol");
    };
    if (top_k != expected.top_k) { reject("top-K width"); }
    if (context != expected.context) { reject("context"); }
    if (stride != expected.stride) { reject("stride"); }
    if ((disjoint_windows != 0) != expected.disjoint_windows) { reject("window mode"); }
    if (score_width != expected.score_width) { reject("score width"); }
    if (scored_tokens != expected.scored_tokens) { reject("scored-token count"); }
    if (corpus_id != expected.corpus_id) { reject("corpus"); }
    if (model_name != expected.model_name) { reject("model"); }

    if (target_count > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("top-K reference " + path.string() +
                                 " has an unreasonable target count");
    }
    const std::size_t count = static_cast<std::size_t>(target_count);
    std::vector<float> target_logprobs(count, 0.0F);
    std::vector<std::int32_t> ids(count * top_k, 0);
    std::vector<float> logprobs(count * top_k, 0.0F);
    read_exact(file, target_logprobs.data(), target_logprobs.size() * sizeof(float),
               "target logprobs", path);
    read_exact(file, ids.data(), ids.size() * sizeof(std::int32_t), "top-K ids", path);
    read_exact(file, logprobs.data(), logprobs.size() * sizeof(float), "top-K logprobs", path);

    std::vector<ninfer::ScoredTarget> targets;
    targets.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        ninfer::ScoredTarget target;
        target.logprob = target_logprobs[index];
        target.topk.reserve(top_k);
        for (std::uint32_t rank = 0; rank < top_k; ++rank) {
            const std::size_t at = index * top_k + rank;
            target.topk.push_back(
                ninfer::ScoreTopKEntry{.token = ids[at], .logprob = logprobs[at]});
        }
        targets.push_back(std::move(target));
    }
    return targets;
}

void KldAccumulator::add(const ninfer::ScoredTarget& candidate,
                         const ninfer::ScoredTarget& reference, ninfer::TokenId target_token) {
    if (candidate.topk.empty() || reference.topk.empty()) {
        throw std::invalid_argument("KLD requires top-K distributions on both sides");
    }
    // Support: the union of both top-K sets plus the target token, deduplicated by sorting.
    std::vector<SupportEntry> support;
    support.reserve(candidate.topk.size() + reference.topk.size() + 1);
    for (const ninfer::ScoreTopKEntry& entry : candidate.topk) {
        support.push_back(SupportEntry{.token = entry.token});
    }
    for (const ninfer::ScoreTopKEntry& entry : reference.topk) {
        support.push_back(SupportEntry{.token = entry.token});
    }
    support.push_back(SupportEntry{.token = target_token});
    std::sort(support.begin(), support.end(),
              [](const SupportEntry& lhs, const SupportEntry& rhs) { return lhs.token < rhs.token; });
    support.erase(std::unique(support.begin(), support.end(),
                              [](const SupportEntry& lhs, const SupportEntry& rhs) {
                                  return lhs.token == rhs.token;
                              }),
                  support.end());

    const float candidate_floor = candidate.topk.back().logprob;
    const float reference_floor = reference.topk.back().logprob;
    for (SupportEntry& entry : support) {
        entry.candidate_logprob =
            support_logprob(candidate, entry.token, target_token, candidate_floor);
        entry.reference_logprob =
            support_logprob(reference, entry.token, target_token, reference_floor);
    }

    // Both sides are renormalized over the support. The union holds each side's most probable
    // token, so both sums are dominated by a value near one and neither underflows.
    double candidate_total = 0.0;
    double reference_total = 0.0;
    for (const SupportEntry& entry : support) {
        candidate_total += std::exp(static_cast<double>(entry.candidate_logprob));
        reference_total += std::exp(static_cast<double>(entry.reference_logprob));
    }
    if (!(candidate_total > 0.0) || !(reference_total > 0.0)) {
        throw std::runtime_error("KLD support has no probability mass");
    }
    double divergence = 0.0;
    for (const SupportEntry& entry : support) {
        const double p = std::exp(static_cast<double>(entry.candidate_logprob)) / candidate_total;
        const double q = std::exp(static_cast<double>(entry.reference_logprob)) / reference_total;
        divergence += p * (std::log(p) - std::log(q));
    }
    kld_.push_back(divergence);
    total_ += divergence;
    target_logprob_delta_total_ +=
        static_cast<double>(candidate.logprob) - static_cast<double>(reference.logprob);
    if (candidate.topk.front().token == reference.topk.front().token) { ++same_top_; }
}

KldSummary KldAccumulator::summary() const {
    KldSummary out;
    out.targets = targets();
    if (kld_.empty()) { return out; }
    std::vector<double> sorted = kld_;
    std::sort(sorted.begin(), sorted.end());
    // Ascending index nearest the requested quantile, clamped to the last sample.
    const auto quantile = [&sorted](double fraction) {
        const double position = std::floor(fraction * static_cast<double>(sorted.size()));
        const auto index      = static_cast<std::size_t>(position);
        return sorted[index < sorted.size() ? index : sorted.size() - 1];
    };
    out.mean    = total_ / static_cast<double>(sorted.size());
    out.median  = quantile(0.5);
    out.p99     = quantile(0.99);
    out.p999    = quantile(0.999);
    out.maximum = sorted.back();
    out.same_top = static_cast<double>(same_top_) / static_cast<double>(sorted.size());
    out.mean_target_logprob_delta = target_logprob_delta_total_ / static_cast<double>(sorted.size());
    return out;
}

} // namespace ninfer::perplexity
