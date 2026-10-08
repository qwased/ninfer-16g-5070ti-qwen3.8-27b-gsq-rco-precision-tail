#include "models/qwen3_5/state/decoder_state.h"

#include "core/device.h"
#include "ops/kv_cache/d256_profile.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ninfer::models::qwen3_5 {
namespace {

std::uint32_t page_count(std::uint32_t capacity, std::uint32_t page_tokens) {
    if (capacity == 0) { throw std::invalid_argument("Paged KV capacity must be positive"); }
    return 1U + (capacity - 1U) / page_tokens;
}

PagedKVCacheLayout plan_cache(std::span<LayoutBuilder* const> builders,
                              std::span<const std::size_t> layer_rank, std::uint32_t layers,
                              std::uint32_t capacity, std::int32_t kv_heads, std::int32_t head_dim,
                              KvCacheStorage storage, std::int32_t table_rows,
                              std::uint32_t physical_page_groups, KvarnBits kvarn_bits) {
    if (layers == 0 ||
        layers > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) ||
        kv_heads <= 0 || head_dim <= 0 || table_rows <= 0) {
        throw std::invalid_argument("Paged KV cache geometry is invalid");
    }
    // KVarN is not a D256 vector format: it stores one joint record plane per layer and carries its
    // own rotated sink/tail slab, so it bypasses the D256 profile entirely.
    const bool kvarn = storage == KvCacheStorage::KvarnGroup128;
    if (kvarn && head_dim != ops::kKvarnHeadDim) {
        throw std::invalid_argument("KVarN requires D256 heads");
    }
    if (!kvarn && head_dim != ops::kD256KVCacheHeadDim) {
        throw std::invalid_argument("Paged KV cache dtype or quantization is invalid");
    }
    const ops::D256KVCacheProfile profile =
        kvarn ? ops::D256KVCacheProfile{} : ops::d256_kv_cache_profile(storage);
    const bool scaled = !kvarn && profile.quant_group != 0;

    const std::uint32_t logical_pages = page_count(capacity, kv_page_tokens(storage));
    if (physical_page_groups < logical_pages) {
        throw std::invalid_argument("Paged KV physical pages are below logical capacity");
    }

    if (!layer_rank.empty() && layer_rank.size() != layers) {
        throw std::invalid_argument("Paged KV layer rank map does not cover every layer");
    }
    const auto rank_of = [&](std::uint32_t layer) {
        return layer_rank.empty() ? std::size_t{0} : layer_rank[layer];
    };
    if (kvarn) {
        // The sink/tail slab is one rank-0 allocation that every layer's view reads, so KVarN is
        // admitted on a single rank only.
        for (std::uint32_t layer = 0; layer < layers; ++layer) {
            if (rank_of(layer) != 0) {
                throw std::invalid_argument("KVarN requires every attention layer on one rank");
            }
        }
    }

    KVPageGeometry geometry;
    geometry.page_tokens = kv_page_tokens(storage);
    std::vector<std::size_t> plane_rank;
    const std::size_t planes_per_layer = kvarn ? 1ULL : (scaled ? 4ULL : 2ULL);
    geometry.planes.reserve(static_cast<std::size_t>(layers) * planes_per_layer);
    plane_rank.reserve(static_cast<std::size_t>(layers) * planes_per_layer);
    for (std::uint32_t layer = 0; layer < layers; ++layer) {
        if (kvarn) {
            // One U8 plane holding every plane of a group record: record_bytes / 128 bytes per
            // token per KV head (35072 / 128 = 274 B for k4v4).
            const std::int32_t bits = static_cast<std::int32_t>(kvarn_bits);
            geometry.planes.push_back(
                {DType::U8, ops::kvarn_record_bytes(bits, bits) / ops::kKvarnGroup, kv_heads, 256});
            plane_rank.push_back(rank_of(layer));
            continue;
        }
        geometry.planes.push_back(
            {profile.key_code_dtype, profile.key_leading_extent, kv_heads, 256});
        geometry.planes.push_back(
            {profile.value_code_dtype, profile.value_leading_extent, kv_heads, 256});
        if (scaled) {
            geometry.planes.push_back(
                {profile.key_scale_dtype, profile.scale_leading_extent, kv_heads, 256});
            geometry.planes.push_back(
                {profile.value_scale_dtype, profile.value_scale_leading_extent, kv_heads, 256});
        }
        plane_rank.insert(plane_rank.end(), planes_per_layer, rank_of(layer));
    }

    // A block-table copy on every rank that runs attention layers of this cache, and always on
    // rank 0, which is where callers that address the tables directly look.
    std::vector<std::size_t> table_ranks{0};
    for (std::uint32_t layer = 0; layer < layers; ++layer) {
        const std::size_t rank = rank_of(layer);
        if (std::find(table_ranks.begin(), table_ranks.end(), rank) == table_ranks.end()) {
            table_ranks.push_back(rank);
        }
    }
    std::sort(table_ranks.begin(), table_ranks.end());

    PagedKVCacheLayout out{
        .pages = plan_device_kv_page_pool(
            builders, plane_rank,
            DeviceKVPagePoolSpec{.page_group_count = physical_page_groups,
                                 .geometry         = std::move(geometry)}),
        .execution_tables = {},
        .layers         = layers,
        .max_context    = capacity,
        .kv_heads       = kv_heads,
        .head_dim       = head_dim,
        .storage        = storage,
        .kvarn_bits     = kvarn_bits,
    };
    for (const std::size_t rank : table_ranks) {
        if (rank >= builders.size() || builders[rank] == nullptr) {
            throw std::invalid_argument("Paged KV layer names a rank with no layout builder");
        }
        out.execution_tables.push_back(plan_kv_execution_tables(
            *builders[rank],
            KVExecutionTableSpec{.logical_page_capacity = logical_pages, .table_rows = table_rows},
            rank));
    }
    if (kvarn) {
        const std::int32_t row_heads = table_rows * kv_heads * ops::kKvarnTailSlots;
        out.kvarn_tail_k             = builders[0]->add_tensor(
            DType::BF16, {head_dim, ops::kKvarnGroup, row_heads, static_cast<std::int32_t>(layers)},
            256, "KVarN rotated K sink/tail");
        out.kvarn_tail_v = builders[0]->add_tensor(
            DType::BF16, {head_dim, ops::kKvarnGroup, row_heads, static_cast<std::int32_t>(layers)},
            256, "KVarN rotated V sink/tail");
        out.kvarn_tail_logical_pages = builders[0]->add_tensor(
            DType::I32, {ops::kKvarnTailSlots, table_rows, static_cast<std::int32_t>(layers)}, 256,
            "KVarN sink/tail logical pages");
    }
    return out;
}

} // namespace

DecoderStateLayout plan_decoder_state(LayoutBuilder& builder, const DecoderStateSpec& spec) {
    LayoutBuilder* const builders[] = {&builder};
    return plan_decoder_state(builders, spec);
}

DecoderStateLayout plan_decoder_state(std::span<LayoutBuilder* const> builders,
                                      const DecoderStateSpec& spec) {
    DecoderStateLayout layout;
    layout.text_kv = plan_cache(builders, spec.text_layer_rank, spec.full_attention_layers,
                                spec.capacity, spec.kv_heads, spec.attention_head_dim,
                                spec.kv_storage, spec.kv_table_rows, spec.text_physical_page_groups,
                                spec.kvarn_bits);
    if (spec.enable_mtp) {
        layout.mtp_kv = plan_cache(builders.first(1), {}, spec.mtp_layers, spec.capacity,
                                   spec.kv_heads, spec.attention_head_dim, spec.kv_storage,
                                   spec.kv_table_rows, spec.mtp_physical_page_groups,
                                   spec.kvarn_bits);
    }
    if (spec.kv_tail_tokens > 0 && spec.full_attention_layers != 0) {
        if (spec.kv_tail_physical_page_groups == 0) {
            throw std::invalid_argument("exact KV tail is enabled with no page groups");
        }
        // Same geometry as the body, but unquantized: two planes (K, V) per full-attention layer on
        // the layer's own rank, page-major order replaced by head-major, and no execution table. The
        // element type is the configured tail dtype (BF16 or FP16, both 16-bit so the geometry and
        // page cost are identical).
        KVPageGeometry tail_geometry{
            .page_tokens        = kPagedKVPageSize,
            .device_plane_order = PagedKVPlaneOrder::HeadMajor,
            .planes =
                {
                    {spec.kv_tail_dtype, spec.attention_head_dim, spec.kv_heads, 256},
                    {spec.kv_tail_dtype, spec.attention_head_dim, spec.kv_heads, 256},
                },
        };
        const auto tail_planes = tail_geometry.planes;
        for (std::uint32_t layer = 1; layer < spec.full_attention_layers; ++layer) {
            tail_geometry.planes.insert(tail_geometry.planes.end(), tail_planes.begin(),
                                        tail_planes.end());
        }
        std::vector<std::size_t> tail_plane_rank;
        tail_plane_rank.reserve(tail_geometry.planes.size());
        for (std::uint32_t layer = 0; layer < spec.full_attention_layers; ++layer) {
            std::size_t rank = 0;
            if (!spec.text_layer_rank.empty()) {
                if (layer >= spec.text_layer_rank.size()) {
                    throw std::invalid_argument("exact KV tail layer names no rank");
                }
                rank = spec.text_layer_rank[layer];
            }
            tail_plane_rank.push_back(rank);
            tail_plane_rank.push_back(rank);
        }
        layout.exact_tail = ExactTailCacheLayout{
            .pages = plan_device_kv_page_pool(
                builders, tail_plane_rank,
                DeviceKVPagePoolSpec{.page_group_count = spec.kv_tail_physical_page_groups,
                                     .geometry         = std::move(tail_geometry)}),
            .retention = spec.kv_tail_tokens,
            .layers    = spec.full_attention_layers,
            .ring_pages = spec.kv_tail_ring_pages,
        };
    }
    return layout;
}

PagedKVCache::PagedKVCache(DeviceSpan backing, const PagedKVCacheLayout& layout)
    : PagedKVCache(std::span<const DeviceSpan>(&backing, 1), layout) {}

PagedKVCache::PagedKVCache(std::span<const DeviceSpan> backings, const PagedKVCacheLayout& layout)
    : pages_(backings, layout.pages), execution_tables_(backings, layout.execution_tables, pages_),
      layers_(layout.layers), max_context_(layout.max_context), kv_heads_(layout.kv_heads),
      head_dim_(layout.head_dim), storage_(layout.storage), kvarn_bits_(layout.kvarn_bits) {
    if (storage_ == KvCacheStorage::KvarnGroup128) {
        if (backings.empty()) { throw std::invalid_argument("KVarN cache needs a backing"); }
        kvarn_tail_k_             = layout.kvarn_tail_k.bind(backings[0]);
        kvarn_tail_v_             = layout.kvarn_tail_v.bind(backings[0]);
        kvarn_tail_logical_pages_ = layout.kvarn_tail_logical_pages.bind(backings[0]);
        // An all-ones marker names no page, so every slot starts empty.
        CUDA_CHECK(
            cudaMemset(kvarn_tail_logical_pages_.data, 0xff, kvarn_tail_logical_pages_.bytes()));
    }
}

PagedKVCacheView::PagedKVCacheView(const PagedKVCache& cache, KVExecutionRowHandle row) noexcept
    : cache_(&cache), row_(row) {}

std::uint32_t PagedKVCacheView::max_context() const noexcept {
    return cache_ == nullptr ? 0 : cache_->max_context();
}

PagedKVLayerView PagedKVCacheView::layer_view(std::uint32_t layer) const {
    if (cache_ == nullptr) { throw std::logic_error("Paged KV execution view is empty"); }
    return cache_->layer_view(layer, &row_);
}

ops::KvarnPagedLayerView PagedKVCacheView::kvarn_layer_view(std::uint32_t layer) const {
    if (cache_ == nullptr) { throw std::logic_error("Paged KV execution view is empty"); }
    return cache_->kvarn_layer_view(layer, &row_);
}

PagedKVCacheView PagedKVCache::execution_view(const KVExecutionRowLease& row) const {
    if (!row.belongs_to(execution_tables_)) {
        throw std::invalid_argument("Paged KV execution row belongs to another cache");
    }
    return PagedKVCacheView(*this, row.handle());
}

std::size_t PagedKVCache::layer_rank(std::uint32_t layer) const {
    if (layer >= layers_) { throw std::out_of_range("Paged KV layer is out of range"); }
    // Every KVarN layer lives on rank 0 (the sink/tail slab and its single record plane share it).
    if (storage_ == KvCacheStorage::KvarnGroup128) { return 0; }
    const bool scaled        = ops::d256_kv_cache_profile(storage_).quant_group != 0;
    const std::size_t stride = scaled ? 4ULL : 2ULL;
    return pages_.plane_rank(static_cast<std::size_t>(layer) * stride);
}

PagedKVLayerView PagedKVCache::layer_view(std::uint32_t layer,
                                          const KVExecutionRowHandle* row) const {
    if (layer >= layers_) { throw std::out_of_range("Paged KV layer is out of range"); }
    if (storage_ == KvCacheStorage::KvarnGroup128) {
        throw std::logic_error("KVarN cache requires kvarn_layer_view");
    }
    const bool scaled        = ops::d256_kv_cache_profile(storage_).quant_group != 0;
    const std::size_t stride = scaled ? 4ULL : 2ULL;
    const std::size_t base   = static_cast<std::size_t>(layer) * stride;
    // The layer reads the copy of the table on its own rank; a kernel cannot follow a pointer into
    // another device's memory.
    const Tensor block_table =
        row == nullptr ? Tensor() : execution_tables_.row(*row, layer_rank(layer));
    PagedKVExactTailView tail;
    if (exact_tail_ != nullptr && tail_retention_ > 0 && layer < layers_) {
        const std::size_t tail_base = static_cast<std::size_t>(layer) * 2;
        tail.k_pages    = exact_tail_->plane(tail_base);
        tail.v_pages    = exact_tail_->plane(tail_base + 1);
        tail.page_count = static_cast<std::int32_t>(tail_ring_pages_);
        tail.retention  = tail_retention_;
    }
    return PagedKVLayerView{
        .k_pages       = pages_.plane(base),
        .v_pages       = pages_.plane(base + 1),
        .k_scale_pages = scaled ? pages_.plane(base + 2) : Tensor(),
        .v_scale_pages = scaled ? pages_.plane(base + 3) : Tensor(),
        .block_table   = block_table,
        .head_dim      = head_dim_,
        .num_kv_heads  = kv_heads_,
        .storage       = storage_,
        .tail          = tail,
    };
}

PagedKVBatchLayerView PagedKVCache::batch_layer_view(std::uint32_t layer) const {
    const PagedKVLayerView direct = layer_view(layer, nullptr);
    return PagedKVBatchLayerView{
        .k_pages       = direct.k_pages,
        .v_pages       = direct.v_pages,
        .k_scale_pages = direct.k_scale_pages,
        .v_scale_pages = direct.v_scale_pages,
        .block_tables  = execution_tables_.matrix(layer_rank(layer)),
        .head_dim      = direct.head_dim,
        .num_kv_heads  = direct.num_kv_heads,
        .storage       = direct.storage,
        .tail          = direct.tail,
    };
}

ops::KvarnPagedLayerView PagedKVCache::kvarn_layer_view(std::uint32_t layer,
                                                        const KVExecutionRowHandle* row) const {
    if (storage_ != KvCacheStorage::KvarnGroup128 || layer >= layers_) {
        throw std::invalid_argument("invalid KVarN layer view");
    }
    // The sink/tail slab is per table row, so this needs a row rather than a rank replica.
    const std::int32_t table_row = row == nullptr ? 0 : row->row_index();
    if (table_row < 0 || table_row >= kvarn_tail_logical_pages_.ne[1]) {
        throw std::invalid_argument("invalid KVarN layer view");
    }
    const std::int32_t row_heads = kv_heads_ * ops::kKvarnTailSlots;
    const std::int32_t begin     = table_row * row_heads;
    const std::int32_t layer_i   = static_cast<std::int32_t>(layer);
    return {
        .records = pages_.plane(layer),
        .tail_k  = kvarn_tail_k_.slice(3, layer_i, 1)
                      .slice(2, begin, row_heads)
                      .view({ops::kKvarnHeadDim, ops::kKvarnGroup, row_heads}),
        .tail_v = kvarn_tail_v_.slice(3, layer_i, 1)
                      .slice(2, begin, row_heads)
                      .view({ops::kKvarnHeadDim, ops::kKvarnGroup, row_heads}),
        .tail_logical_pages = kvarn_tail_logical_pages_.slice(2, layer_i, 1)
                                  .slice(1, table_row, 1)
                                  .view({ops::kKvarnTailSlots}),
        .block_table =
            row == nullptr ? Tensor() : execution_tables_.row(*row, layer_rank(layer)),
        .num_kv_heads = kv_heads_,
        .bits         = static_cast<std::int32_t>(kvarn_bits_),
    };
}

ops::KvarnPagedBatchLayerView PagedKVCache::kvarn_batch_layer_view(std::uint32_t layer) const {
    if (storage_ != KvCacheStorage::KvarnGroup128 || layer >= layers_) {
        throw std::invalid_argument("invalid KVarN batch layer view");
    }
    const std::int32_t rows      = kvarn_tail_logical_pages_.ne[1];
    const std::int32_t row_heads = kv_heads_ * ops::kKvarnTailSlots;
    const std::int32_t layer_i   = static_cast<std::int32_t>(layer);
    // WP6: the same per-layer exact ring `layer_view` exposes for the other storages. The KVarN
    // route reads it directly instead of going through a block table.
    PagedKVExactTailView tail;
    if (exact_tail_ != nullptr && tail_retention_ > 0) {
        const std::size_t tail_base = static_cast<std::size_t>(layer) * 2;
        tail.k_pages    = exact_tail_->plane(tail_base);
        tail.v_pages    = exact_tail_->plane(tail_base + 1);
        tail.page_count = static_cast<std::int32_t>(tail_ring_pages_);
        tail.retention  = tail_retention_;
    }
    return {
        .records = pages_.plane(layer),
        .tail_k  = kvarn_tail_k_.slice(3, layer_i, 1).view(
            {ops::kKvarnHeadDim, ops::kKvarnGroup, row_heads, rows}),
        .tail_v = kvarn_tail_v_.slice(3, layer_i, 1).view(
            {ops::kKvarnHeadDim, ops::kKvarnGroup, row_heads, rows}),
        .tail_logical_pages = kvarn_tail_logical_pages_.slice(2, layer_i, 1).view(
            {ops::kKvarnTailSlots, rows}),
        .block_tables = execution_tables_.matrix(layer_rank(layer)),
        .num_kv_heads = kv_heads_,
        .bits         = static_cast<std::int32_t>(kvarn_bits_),
        .tail         = tail,
    };
}

void PagedKVCache::reset_kvarn_tail_row(std::int32_t table_row, cudaStream_t stream) const {
    if (storage_ != KvCacheStorage::KvarnGroup128 || table_row < 0 ||
        table_row >= kvarn_tail_logical_pages_.ne[1]) {
        return;
    }
    for (std::uint32_t layer = 0; layer < layers_; ++layer) {
        Tensor markers = kvarn_tail_logical_pages_
                             .slice(2, static_cast<std::int32_t>(layer), 1)
                             .slice(1, table_row, 1);
        CUDA_CHECK(cudaMemsetAsync(markers.data, 0xff, markers.bytes(), stream));
    }
}

std::size_t DecoderStateLayout::kv_payload_bytes() const noexcept {
    return text_kv.payload_bytes() + (mtp_kv ? mtp_kv->payload_bytes() : 0) +
           (exact_tail ? exact_tail->payload_bytes() : 0);
}

DecoderState::DecoderState(DeviceSpan backing, const DecoderStateLayout& layout)
    : DecoderState(std::span<const DeviceSpan>(&backing, 1), layout) {}

DecoderState::DecoderState(std::span<const DeviceSpan> backings, const DecoderStateLayout& layout)
    : text_kv(backings, layout.text_kv) {
    if (layout.mtp_kv) { mtp_kv.emplace(backings, *layout.mtp_kv); }
    if (layout.exact_tail) {
        exact_tail.emplace(backings, layout.exact_tail->pages);
        text_kv.attach_exact_tail(*exact_tail, layout.exact_tail->retention,
                                  layout.exact_tail->ring_pages);
    }
}

void PagedKVCache::attach_exact_tail(const DeviceKVPagePool& pool, std::int32_t retention,
                                     std::uint32_t ring_pages) noexcept {
    exact_tail_      = &pool;
    tail_retention_  = retention;
    tail_ring_pages_ = ring_pages;
}

PagedKVCache* DecoderState::mtp_cache() noexcept { return mtp_kv ? &*mtp_kv : nullptr; }

const PagedKVCache* DecoderState::mtp_cache() const noexcept { return mtp_kv ? &*mtp_kv : nullptr; }

DeviceKVPagePool* DecoderState::exact_tail_pool() noexcept {
    return exact_tail ? &*exact_tail : nullptr;
}

const DeviceKVPagePool* DecoderState::exact_tail_pool() const noexcept {
    return exact_tail ? &*exact_tail : nullptr;
}

} // namespace ninfer::models::qwen3_5
