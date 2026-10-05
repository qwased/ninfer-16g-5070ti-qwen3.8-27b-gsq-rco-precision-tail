// Host (CPU-only) test pinning milestone M3's exact-tail concurrency model. The exact tail is a
// per-sequence ring of `page_count(N) + 1` pages -- the newest `N` tokens (`round_up(N, 64) / 64`
// pages) plus one rollback reserve -- replicated for every concurrent sequence. So the whole pool is
// `(page_count(N) + 1) * C` page groups with a fixed 4 MiB page-group cost, which is a constant
// `round_up(N, 64) * 65,536 * C` retention part plus `C` rollback pages of 4 MiB (plan §2).
//
// No GPU is touched: `plan_decoder_state` only lays regions out in a `LayoutBuilder`.

#include "core/layout.h"
#include "models/qwen3_5/state/decoder_state.h"

#include <cstdint>
#include <iostream>
#include <string_view>

namespace {

namespace q36 = ninfer::models::qwen3_5;

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) { return; }
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

// Mirrors decoder_state.cpp's page_count(): pages needed for `tokens` tokens at 64 tokens/page.
std::uint32_t page_count(std::uint32_t tokens) {
    return tokens == 0U ? 0U : 1U + (tokens - 1U) / 64U;
}

// One full-attention layer keeps a K and a V plane per page group, each 16-bit and carrying
// head_dim * 64 * kv_heads elements. The plan's §2 per-page-group cost then is
// 16 layers * 2 planes * 256 * 64 * 4 heads * 2 bytes = 4 MiB.
constexpr std::uint64_t kTailPageGroupBytes = 4ULL * 1024 * 1024;
// The same cost expressed per token and per concurrent sequence:
// 16 layers * 2 planes * 256 head_dim * 4 heads * 2 bytes = 65,536 B/token.
constexpr std::uint64_t kTailBytesPerTokenPerSequence = 65'536ULL;

constexpr std::uint32_t kFullAttentionLayers = 16;
constexpr std::uint32_t kMtpLayers           = 1;
constexpr std::uint32_t kKvHeads             = 4;
constexpr std::uint32_t kHeadDim             = 256;
constexpr std::uint32_t kCapacity            = 2048; // page_count(2048) == 32 logical pages
constexpr std::uint32_t kTextPageGroups      = 32;
constexpr std::uint32_t kMtpPageGroups       = 32;

// The tail fields follow production (startup.cpp): ring = page_count(N) + 1, pool = ring * C.
q36::DecoderStateSpec make_spec(std::uint32_t concurrency, std::int32_t tail_tokens) {
    q36::DecoderStateSpec spec;
    spec.full_attention_layers     = kFullAttentionLayers;
    spec.mtp_layers                = kMtpLayers;
    spec.capacity                  = kCapacity;
    spec.kv_heads                  = static_cast<std::int32_t>(kKvHeads);
    spec.attention_head_dim        = static_cast<std::int32_t>(kHeadDim);
    spec.kv_storage                = ninfer::KvCacheStorage::BFloat16;
    spec.enable_mtp                = true;
    spec.kv_table_rows             = static_cast<std::int32_t>(concurrency);
    spec.text_physical_page_groups = kTextPageGroups;
    spec.mtp_physical_page_groups  = kMtpPageGroups;
    if (tail_tokens > 0) {
        spec.kv_tail_tokens               = tail_tokens;
        spec.kv_tail_ring_pages           = page_count(static_cast<std::uint32_t>(tail_tokens)) + 1U;
        spec.kv_tail_physical_page_groups = spec.kv_tail_ring_pages * concurrency;
    }
    return spec;
}

void test_exact_tail_capacity_model() {
    const std::uint32_t tail_tokens[] = {512U, 1024U, 2048U};

    // The draft/MTP cache is a `PagedKVCacheLayout`, which has no tail member at all -- the exact
    // tail lives solely in `DecoderStateLayout::exact_tail` and belongs to the text cache. So MTP's
    // device cost is fixed: 1 layer * 2 planes (K, V) * head_dim * 64 tokens * kv_heads * 2 bytes per
    // page group, times the MTP page-group count. It must not move with the tail.
    const std::uint64_t mtp_expected_bytes = static_cast<std::uint64_t>(kMtpLayers) * 2ULL * kHeadDim *
                                             64ULL * kKvHeads * 2ULL * kMtpPageGroups;

    std::uint64_t reference_mtp_payload = 0;
    for (std::uint32_t concurrency = 1; concurrency <= 8; ++concurrency) {
        for (const std::uint32_t tokens : tail_tokens) {
            const std::uint32_t pages_per_sequence = page_count(tokens);      // round_up(N, 64) / 64
            const std::uint32_t ring_pages         = pages_per_sequence + 1U; // + rollback reserve
            const std::uint32_t pool_pages         = ring_pages * concurrency;

            ninfer::LayoutBuilder builder;
            const q36::DecoderStateLayout tailed = q36::plan_decoder_state(
                builder, make_spec(concurrency, static_cast<std::int32_t>(tokens)));
            (void)builder.finish(256);

            // (1) Whole-pool device bytes == (page_count(N) + 1) * C * 4 MiB, i.e. a constant
            // round_up(N, 64) * 65,536 * C retention part plus C rollback pages of 4 MiB (§2).
            expect(tailed.exact_tail.has_value(), "a positive tail length engages the exact tail");
            if (tailed.exact_tail) {
                const std::uint64_t retention_bytes = static_cast<std::uint64_t>(pages_per_sequence) *
                                                      64ULL * kTailBytesPerTokenPerSequence *
                                                      concurrency;
                const std::uint64_t rollback_bytes =
                    static_cast<std::uint64_t>(concurrency) * kTailPageGroupBytes;
                expect(tailed.exact_tail->pages.payload_bytes() ==
                           static_cast<std::uint64_t>(ring_pages) * concurrency * kTailPageGroupBytes,
                       "exact tail pool is one ring per concurrent sequence");
                expect(tailed.exact_tail->pages.payload_bytes() == retention_bytes + rollback_bytes,
                       "exact tail payload is the retention part plus C rollback pages");
                expect(tailed.exact_tail->pages.spec.page_group_count == pool_pages,
                       "exact tail pool is ring * C page groups");
                expect(tailed.exact_tail->ring_pages == ring_pages,
                       "exact tail ring is page_count(N) + 1");
                expect(tailed.exact_tail->layers == kFullAttentionLayers,
                       "exact tail covers every full-attention layer");
                // (3) When the tail is on, the layout records retention N ...
                expect(tailed.exact_tail->retention == static_cast<std::int32_t>(tokens),
                       "exact tail layout records retention N");
            }

            // (2) The draft/MTP cache stays tail-free: only its own K/V planes, fixed payload size,
            // and never any exact-tail allocation.
            expect(tailed.mtp_kv.has_value(), "enabled MTP has a paged KV cache");
            if (tailed.mtp_kv) {
                expect(tailed.mtp_kv->pages.planes.size() == 2U * kMtpLayers,
                       "MTP cache carries only its own K/V planes, no tail allocation");
                expect(tailed.mtp_kv->payload_bytes() == mtp_expected_bytes,
                       "MTP payload is the tail-free fixed size");
            }

            // Build the same shape with the tail off. Everything except the tail fields is identical,
            // so MTP's payload must be byte-for-byte the same.
            ninfer::LayoutBuilder off_builder;
            const q36::DecoderStateLayout untailed =
                q36::plan_decoder_state(off_builder, make_spec(concurrency, 0));
            (void)off_builder.finish(256);
            expect(!untailed.exact_tail.has_value(),
                   "kv_tail_tokens == 0 leaves the exact tail disabled");
            expect(untailed.mtp_kv && tailed.mtp_kv &&
                       untailed.mtp_kv->payload_bytes() == tailed.mtp_kv->payload_bytes(),
                   "MTP payload is identical with the tail on and off");
            expect(untailed.mtp_kv && untailed.mtp_kv->pages.planes.size() == 2U * kMtpLayers,
                   "MTP cache has no exact-tail allocation when the tail is off");
            expect(untailed.text_kv.payload_bytes() == tailed.text_kv.payload_bytes(),
                   "the exact tail does not enlarge the text body cache");

            if (reference_mtp_payload == 0 && tailed.mtp_kv) {
                reference_mtp_payload = tailed.mtp_kv->payload_bytes();
            }
            expect(tailed.mtp_kv && tailed.mtp_kv->payload_bytes() == reference_mtp_payload,
                   "MTP payload does not grow with the tail length N");
        }
    }

    // (3) The consumer-facing switch is `PagedKVExactTailView::enabled()` (retention > 0).
    // `ExactTailCacheLayout` itself has no `enabled()`; the engaged optional asserted above is its
    // layout-level switch. Here we pin the view-level predicate both ways.
    ninfer::PagedKVExactTailView disabled;
    expect(!disabled.enabled(), "a zero-retention tail view is disabled");
    ninfer::PagedKVExactTailView enabled;
    enabled.retention = 512;
    expect(enabled.enabled(), "a positive-retention tail view is enabled");
}

} // namespace

int main() {
    test_exact_tail_capacity_model();
    if (failures != 0) {
        std::cerr << failures << " exact-tail capacity checks failed\n";
        return 1;
    }
    std::cout << "Exact-tail per-concurrency capacity checks passed\n";
    return 0;
}
