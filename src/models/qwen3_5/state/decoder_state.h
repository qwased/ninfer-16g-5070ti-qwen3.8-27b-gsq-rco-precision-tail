#pragma once

#include "core/layout.h"
#include "core/paged_kv_cache.h"

#include "ninfer/ops/kvarn.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ninfer::models::qwen3_5 {

inline constexpr std::int32_t kKvInt8QuantGroup = 64;
inline constexpr std::int32_t kKvFp8QuantGroup  = 256;

struct DecoderStateSpec {
    std::uint32_t full_attention_layers     = 0;
    std::uint32_t mtp_layers                = 0;
    std::uint32_t capacity                  = 0;
    std::int32_t kv_heads                   = 0;
    std::int32_t attention_head_dim         = 0;
    KvCacheStorage kv_storage                = KvCacheStorage::BFloat16;
    // KVarN's packed width (K == V). Only read when `kv_storage == KvarnGroup128`.
    KvarnBits kvarn_bits                     = KvarnBits::Bits4;
    bool enable_mtp                         = false;
    std::int32_t kv_table_rows              = 1;
    std::uint32_t text_physical_page_groups = 0;
    std::uint32_t mtp_physical_page_groups  = 0;
    // Exact (unquantized) KV tail: the newest `kv_tail_tokens` tokens of every sequence are kept in a
    // second BF16 pool. Zero disables it. `kv_tail_physical_page_groups` is the whole pool's page-group
    // extent, i.e. the per-sequence ring times max concurrency.
    std::int32_t kv_tail_tokens              = 0;
    // Element type of the tail's unquantized planes (BF16 or FP16; both 16-bit).
    DType kv_tail_dtype                      = DType::BF16;
    std::uint32_t kv_tail_physical_page_groups = 0;
    // The per-sequence ring length in pages; the pool holds this many for each concurrent sequence.
    std::uint32_t kv_tail_ring_pages         = 0;
    // The rank holding each full-attention layer's KV planes; empty puts every layer on rank 0. The
    // MTP cache always lives on rank 0.
    std::vector<std::size_t> text_layer_rank;
};

struct PagedKVCacheLayout {
    DeviceKVPagePoolLayout pages;
    // One copy of the block tables per rank that runs attention layers of this cache.
    std::vector<KVExecutionTableLayout> execution_tables;
    // KVarN's rotated sink/tail state, bound only for a KVarN body. `kvarn_tail_k`/`kvarn_tail_v`
    // each hold one BF16 `{head_dim, kKvarnGroup, table_rows*kv_heads*kKvarnTailSlots, layers}`
    // slab; `kvarn_tail_logical_pages` holds one `{kKvarnTailSlots, table_rows, layers}` int32
    // marker block naming the committed logical page of every slot (0xffffffff = empty).
    TensorRegion kvarn_tail_k;
    TensorRegion kvarn_tail_v;
    TensorRegion kvarn_tail_logical_pages;
    std::uint32_t layers      = 0;
    std::uint32_t max_context = 0;
    std::int32_t kv_heads     = 0;
    std::int32_t head_dim     = 0;
    KvCacheStorage storage    = KvCacheStorage::BFloat16;
    // KVarN's packed width (K == V); every layer view carries it to the ops layer.
    KvarnBits kvarn_bits      = KvarnBits::Bits4;

    [[nodiscard]] std::size_t payload_bytes() const noexcept {
        return pages.payload_bytes() + kvarn_tail_k.region.bytes + kvarn_tail_v.region.bytes +
               kvarn_tail_logical_pages.region.bytes;
    }
};

class PagedKVCache;

class PagedKVCacheView {
public:
    PagedKVCacheView() noexcept = default;

    [[nodiscard]] bool valid() const noexcept { return cache_ != nullptr; }

    [[nodiscard]] std::uint32_t max_context() const noexcept;
    [[nodiscard]] PagedKVLayerView layer_view(std::uint32_t layer) const;
    // The KVarN body of this row's layer. Only a KVarN cache has one.
    [[nodiscard]] ops::KvarnPagedLayerView kvarn_layer_view(std::uint32_t layer) const;

private:
    friend class PagedKVCache;
    PagedKVCacheView(const PagedKVCache& cache, KVExecutionRowHandle row) noexcept;

    const PagedKVCache* cache_ = nullptr;
    // The row, not a tensor: each layer reads the copy of its row held by the layer's own rank.
    KVExecutionRowHandle row_;
};

class PagedKVCache {
public:
    // `backings[r]` is rank r's persistent device memory.
    PagedKVCache(std::span<const DeviceSpan> backings, const PagedKVCacheLayout& layout);
    // Every plane and table on rank 0.
    PagedKVCache(DeviceSpan backing, const PagedKVCacheLayout& layout);

    PagedKVCache(const PagedKVCache&)            = delete;
    PagedKVCache& operator=(const PagedKVCache&) = delete;
    PagedKVCache(PagedKVCache&&)                 = delete;
    PagedKVCache& operator=(PagedKVCache&&)      = delete;

    [[nodiscard]] std::uint32_t max_context() const noexcept { return max_context_; }

    [[nodiscard]] std::uint32_t layers() const noexcept { return layers_; }

    [[nodiscard]] KvCacheStorage storage() const noexcept { return storage_; }

    [[nodiscard]] std::int32_t kv_heads() const noexcept { return kv_heads_; }

    [[nodiscard]] DeviceKVPagePool& page_pool() noexcept { return pages_; }

    [[nodiscard]] const DeviceKVPagePool& page_pool() const noexcept { return pages_; }

    [[nodiscard]] KVExecutionTablePool& execution_tables() noexcept { return execution_tables_; }

    [[nodiscard]] const KVExecutionTablePool& execution_tables() const noexcept {
        return execution_tables_;
    }

    [[nodiscard]] PagedKVCacheView execution_view(const KVExecutionRowLease& row) const;

    // Binds the device-only exact tail pool this cache's layer views expose. The pool must outlive
    // the cache. Without a binding the views carry a disabled tail.
    void attach_exact_tail(const DeviceKVPagePool& pool, std::int32_t retention,
                           std::uint32_t ring_pages) noexcept;

    [[nodiscard]] PagedKVBatchLayerView batch_layer_view(std::uint32_t layer) const;
    // The KVarN body of every row of `layer`, for batched decode/verify.
    [[nodiscard]] ops::KvarnPagedBatchLayerView kvarn_batch_layer_view(std::uint32_t layer) const;
    // Rewrites the slot markers of `table_row` back to empty. No-op on a non-KVarN cache.
    void reset_kvarn_tail_row(std::int32_t table_row, cudaStream_t stream = nullptr) const;
    // The rank holding this layer's KV planes, which is also the rank whose block table it reads.
    [[nodiscard]] std::size_t layer_rank(std::uint32_t layer) const;

private:
    friend class PagedKVCacheView;
    [[nodiscard]] PagedKVLayerView layer_view(std::uint32_t layer,
                                              const KVExecutionRowHandle* row) const;
    [[nodiscard]] ops::KvarnPagedLayerView kvarn_layer_view(std::uint32_t layer,
                                                            const KVExecutionRowHandle* row) const;

    DeviceKVPagePool pages_;
    KVExecutionTablePool execution_tables_;
    std::uint32_t layers_      = 0;
    std::uint32_t max_context_ = 0;
    std::int32_t kv_heads_     = 0;
    std::int32_t head_dim_     = 0;
    KvCacheStorage storage_    = KvCacheStorage::BFloat16;
    KvarnBits kvarn_bits_      = KvarnBits::Bits4;
    Tensor kvarn_tail_k_;
    Tensor kvarn_tail_v_;
    Tensor kvarn_tail_logical_pages_;
    const DeviceKVPagePool* exact_tail_ = nullptr;
    std::int32_t tail_retention_        = 0;
    std::uint32_t tail_ring_pages_      = 0;
};

// Exact KV tail: BF16, HeadMajor, two planes (K, V) per full-attention layer, page 64. Device-only,
// so unlike PagedKVCacheLayout it owns no execution tables; its pages are addressed by position.
struct ExactTailCacheLayout {
    DeviceKVPagePoolLayout pages;
    std::int32_t retention = 0;
    std::uint32_t layers   = 0;
    std::uint32_t ring_pages = 0;

    [[nodiscard]] std::size_t payload_bytes() const noexcept { return pages.payload_bytes(); }
};

struct DecoderStateLayout {
    PagedKVCacheLayout text_kv;
    std::optional<PagedKVCacheLayout> mtp_kv;
    std::optional<ExactTailCacheLayout> exact_tail;

    [[nodiscard]] std::size_t kv_payload_bytes() const noexcept;
};

[[nodiscard]] DecoderStateLayout plan_decoder_state(LayoutBuilder& builder,
                                                    const DecoderStateSpec& spec);
// One layout builder per rank: each layer's planes and each rank's block-table copy are laid out in
// that rank's own backing.
[[nodiscard]] DecoderStateLayout plan_decoder_state(std::span<LayoutBuilder* const> builders,
                                                    const DecoderStateSpec& spec);

struct DecoderState {
    PagedKVCache text_kv;
    std::optional<PagedKVCache> mtp_kv;
    // Exact tail pool: no execution tables, addressed by position ring, device-only.
    std::optional<DeviceKVPagePool> exact_tail;

    DecoderState(std::span<const DeviceSpan> backings, const DecoderStateLayout& layout);
    DecoderState(DeviceSpan backing, const DecoderStateLayout& layout);

    [[nodiscard]] PagedKVCache* mtp_cache() noexcept;
    [[nodiscard]] const PagedKVCache* mtp_cache() const noexcept;
    [[nodiscard]] DeviceKVPagePool* exact_tail_pool() noexcept;
    [[nodiscard]] const DeviceKVPagePool* exact_tail_pool() const noexcept;
};

} // namespace ninfer::models::qwen3_5
