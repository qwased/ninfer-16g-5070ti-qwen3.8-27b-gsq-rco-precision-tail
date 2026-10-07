#include "models/qwen3_5/state/state_image.h"

#include "core/device.h"

#include <algorithm>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace ninfer::models::qwen3_5 {
namespace {

constexpr std::size_t kStateImageAlignment = 256;

std::size_t checked_mul(std::size_t left, std::size_t right, const char* label) {
    if (right != 0 && left > std::numeric_limits<std::size_t>::max() / right) {
        throw std::overflow_error(label);
    }
    return left * right;
}

std::size_t checked_add(std::size_t left, std::size_t right, const char* label) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw std::overflow_error(label);
    }
    return left + right;
}

std::int32_t checked_i32(std::size_t value, const char* label) {
    if (value > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
        throw std::overflow_error(label);
    }
    return static_cast<std::int32_t>(value);
}

std::uint32_t padded_dflash_capacity(std::uint32_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("StateImage DFlash capacity must be positive");
    }
    constexpr std::uint64_t alignment = 128;
    const std::uint64_t padded =
        (static_cast<std::uint64_t>(capacity) + alignment - 1U) & ~(alignment - 1U);
    if (padded > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
        throw std::overflow_error("StateImage DFlash padded capacity exceeds int32");
    }
    return static_cast<std::uint32_t>(padded);
}

bool same_linear_spec(const LinearAttentionStatePoolSpec& left,
                      const LinearAttentionStatePoolSpec& right) noexcept {
    return left.layers == right.layers && left.conv_channels == right.conv_channels &&
           left.conv_width == right.conv_width && left.value_heads == right.value_heads &&
           left.value_head_dim == right.value_head_dim && left.key_head_dim == right.key_head_dim &&
           left.slot_count == right.slot_count && left.conv_dtype == right.conv_dtype &&
           left.recurrent_dtype == right.recurrent_dtype;
}

bool same_dflash_spec(const std::optional<DFlashLocalStateSpec>& left,
                      const std::optional<DFlashLocalStateSpec>& right) noexcept {
    if (left.has_value() != right.has_value()) { return false; }
    if (!left) { return true; }
    return left->layers == right->layers && left->capacity == right->capacity &&
           left->kv_heads == right->kv_heads && left->head_dim == right->head_dim;
}

bool same_kvarn_spec(const std::optional<KvarnContinuationStateSpec>& left,
                     const std::optional<KvarnContinuationStateSpec>& right) noexcept {
    if (left.has_value() != right.has_value()) { return false; }
    if (!left) { return true; }
    return left->text_layers == right->text_layers && left->mtp_layers == right->mtp_layers &&
           left->kv_heads == right->kv_heads && left->head_dim == right->head_dim;
}

bool same_kvarn_layout(const std::optional<KvarnContinuationImageLayout>& left,
                       const std::optional<KvarnContinuationImageLayout>& right) noexcept {
    if (left.has_value() != right.has_value()) { return false; }
    if (!left) { return true; }
    return same_kvarn_spec(left->spec, right->spec) && left->text_k_offset == right->text_k_offset &&
           left->text_v_offset == right->text_v_offset &&
           left->text_marker_offset == right->text_marker_offset &&
           left->mtp_k_offset == right->mtp_k_offset && left->mtp_v_offset == right->mtp_v_offset &&
           left->mtp_marker_offset == right->mtp_marker_offset &&
           left->tail_layer_bytes == right->tail_layer_bytes &&
           left->marker_layer_bytes == right->marker_layer_bytes &&
           left->image_bytes == right->image_bytes;
}

// The KVarN continuation image: per layer a BF16 sink/tail tile (K and V) plus the retired slot
// markers. Text layers and the MTP pool are separate components so either can be zero.
KvarnContinuationImageLayout plan_kvarn_continuation_image(const KvarnContinuationStateSpec& spec) {
    const Tensor tail(nullptr, DType::BF16,
                      {spec.head_dim, ops::kKvarnGroup, spec.kv_heads * ops::kKvarnTailSlots});
    const Tensor markers(nullptr, DType::I32, {ops::kKvarnTailSlots});
    KvarnContinuationImageLayout out{
        .spec               = spec,
        .tail_layer_bytes   = tail.bytes(),
        .marker_layer_bytes = markers.bytes(),
    };
    LayoutBuilder builder;
    const auto add = [&](std::uint32_t layers, std::size_t& k, std::size_t& v, std::size_t& marker,
                         const char* label) {
        if (layers == 0) { return; }
        const std::size_t tail_bytes =
            checked_mul(out.tail_layer_bytes, layers, "KVarN continuation tail bytes overflow");
        const std::size_t marker_bytes =
            checked_mul(out.marker_layer_bytes, layers, "KVarN continuation marker bytes overflow");
        k      = builder.add(tail_bytes, kStateImageAlignment, label).offset;
        v      = builder.add(tail_bytes, kStateImageAlignment, label).offset;
        marker = builder.add(marker_bytes, kStateImageAlignment, label).offset;
    };
    add(spec.text_layers, out.text_k_offset, out.text_v_offset, out.text_marker_offset,
        "KVarN Text continuation");
    add(spec.mtp_layers, out.mtp_k_offset, out.mtp_v_offset, out.mtp_marker_offset,
        "KVarN MTP continuation");
    out.image_bytes = builder.finish(kStateImageAlignment, "KVarN continuation image");
    return out;
}

bool same_region(const LayoutRegion& left, const LayoutRegion& right) noexcept {
    return left.offset == right.offset && left.bytes == right.bytes &&
           left.alignment == right.alignment;
}

bool same_optional_region(const std::optional<LayoutRegion>& left,
                          const std::optional<LayoutRegion>& right) noexcept {
    return left.has_value() == right.has_value() && (!left || same_region(*left, *right));
}

bool same_host_layout(const StateImageHostLayout& left,
                      const StateImageHostLayout& right) noexcept {
    return same_linear_spec(left.spec.linear, right.spec.linear) &&
           left.spec.hidden == right.spec.hidden &&
           same_dflash_spec(left.spec.dflash_local, right.spec.dflash_local) &&
           same_kvarn_spec(left.spec.kvarn, right.spec.kvarn) &&
           same_region(left.linear_conv, right.linear_conv) &&
           left.linear_conv_layer_bytes == right.linear_conv_layer_bytes &&
           same_region(left.linear_recurrent, right.linear_recurrent) &&
           left.linear_recurrent_layer_bytes == right.linear_recurrent_layer_bytes &&
           same_region(left.continuation_hidden, right.continuation_hidden) &&
           same_optional_region(left.dflash_local_k, right.dflash_local_k) &&
           same_optional_region(left.dflash_local_v, right.dflash_local_v) &&
           left.dflash_local_layer_bytes == right.dflash_local_layer_bytes &&
           same_optional_region(left.kvarn, right.kvarn) &&
           same_kvarn_layout(left.kvarn_layout, right.kvarn_layout) &&
           left.image_bytes == right.image_bytes;
}

StateImageHostLayout plan_host_state_image(const StateImageSpec& spec) {
    if (spec.linear.layers == 0 || spec.linear.conv_channels <= 0 || spec.linear.conv_width <= 0 ||
        spec.linear.value_heads <= 0 || spec.linear.value_head_dim <= 0 ||
        spec.linear.key_head_dim <= 0 || spec.linear.slot_count <= 0 ||
        (spec.linear.conv_dtype != DType::BF16 && spec.linear.conv_dtype != DType::FP32) ||
        (spec.linear.recurrent_dtype != DType::FP32 &&
         spec.linear.recurrent_dtype != DType::FP16) ||
        spec.hidden <= 0) {
        throw std::invalid_argument("StateImage host geometry is invalid");
    }
    if (spec.dflash_local && (spec.dflash_local->layers == 0 || spec.dflash_local->kv_heads <= 0 ||
                              spec.dflash_local->head_dim <= 0)) {
        throw std::invalid_argument("StateImage host DFlash geometry is invalid");
    }
    if (spec.kvarn && (spec.kvarn->text_layers == 0 || spec.kvarn->kv_heads <= 0 ||
                       spec.kvarn->head_dim != ops::kKvarnHeadDim)) {
        throw std::invalid_argument("StateImage host KVarN geometry is invalid");
    }

    LayoutBuilder builder;
    StateImageHostLayout host;
    host.spec = spec;
    const Tensor conv_slot(nullptr, spec.linear.conv_dtype,
                           {spec.linear.conv_channels, spec.linear.conv_width});
    const Tensor recurrent_slot(
        nullptr, spec.linear.recurrent_dtype,
        {spec.linear.key_head_dim, spec.linear.value_head_dim, spec.linear.value_heads});
    const Tensor hidden_slot(nullptr, DType::BF16, {spec.hidden});
    host.linear_conv_layer_bytes = conv_slot.bytes();
    host.linear_conv = builder.add(checked_mul(host.linear_conv_layer_bytes, spec.linear.layers,
                                               "StateImage host convolution bytes overflow"),
                                   kStateImageAlignment, "StateImage host convolution");
    host.linear_recurrent_layer_bytes = recurrent_slot.bytes();
    host.linear_recurrent =
        builder.add(checked_mul(host.linear_recurrent_layer_bytes, spec.linear.layers,
                                "StateImage host recurrent bytes overflow"),
                    kStateImageAlignment, "StateImage host recurrent");
    host.continuation_hidden = builder.add(hidden_slot.bytes(), kStateImageAlignment,
                                           "StateImage host continuation hidden");
    if (spec.dflash_local) {
        const Tensor local_slot(
            nullptr, DType::BF16,
            {spec.dflash_local->head_dim,
             static_cast<std::int32_t>(padded_dflash_capacity(spec.dflash_local->capacity)),
             spec.dflash_local->kv_heads});
        host.dflash_local_layer_bytes = local_slot.bytes();
        const std::size_t component_bytes =
            checked_mul(host.dflash_local_layer_bytes, spec.dflash_local->layers,
                        "StateImage host DFlash local bytes overflow");
        host.dflash_local_k =
            builder.add(component_bytes, kStateImageAlignment, "StateImage host DFlash local K");
        host.dflash_local_v =
            builder.add(component_bytes, kStateImageAlignment, "StateImage host DFlash local V");
    }
    if (spec.kvarn) {
        host.kvarn_layout = plan_kvarn_continuation_image(*spec.kvarn);
        host.kvarn        = builder.add(host.kvarn_layout->image_bytes, kStateImageAlignment,
                                        "StateImage host KVarN continuation");
    }
    host.image_bytes = builder.finish(kStateImageAlignment, "StateImage host image");
    return host;
}

std::byte* byte_offset(std::byte* base, std::size_t offset) noexcept { return base + offset; }

const std::byte* byte_offset(const std::byte* base, std::size_t offset) noexcept {
    return base + offset;
}

void validate_slot(std::int32_t slot, std::int32_t slot_count, const char* label) {
    if (slot < 0 || slot >= slot_count) { throw std::out_of_range(label); }
}

} // namespace

StateImageDeviceLayout plan_state_image_device_pool(LayoutBuilder& builder,
                                                    const StateImageSpec& spec) {
    LayoutBuilder* const builders[] = {&builder};
    const StateImageShard whole[]   = {{.rank = 0, .first_layer = 0, .layers = spec.linear.layers}};
    return plan_state_image_device_pool(builders, whole, spec);
}

StateImageDeviceLayout plan_state_image_device_pool(std::span<LayoutBuilder* const> builders,
                                                    std::span<const StateImageShard> shards,
                                                    const StateImageSpec& spec) {
    if (spec.hidden <= 0) {
        throw std::invalid_argument("StateImage hidden width must be positive");
    }
    if (shards.empty()) { throw std::invalid_argument("StateImage needs at least one shard"); }
    std::uint32_t next_layer = 0;
    for (const StateImageShard& shard : shards) {
        if (shard.first_layer != next_layer || shard.layers == 0 || shard.rank >= builders.size() ||
            builders[shard.rank] == nullptr) {
            throw std::invalid_argument("StateImage shards must cover the layers in order");
        }
        next_layer += shard.layers;
    }
    if (next_layer != spec.linear.layers) {
        throw std::invalid_argument("StateImage shards do not cover every Linear Attention layer");
    }

    StateImageDeviceLayout out;
    out.shards.assign(shards.begin(), shards.end());
    for (const StateImageShard& shard : shards) {
        LinearAttentionStatePoolSpec shard_spec = spec.linear;
        shard_spec.layers                       = shard.layers;
        out.linear.push_back(plan_linear_attention_state_pool(*builders[shard.rank], shard_spec));
    }
    // The continuation hidden and DFlash local state are read by the head and the draft, which run
    // on rank 0.
    LayoutBuilder& builder = *builders[0];
    out.continuation_hidden =
        builder.add_tensor(DType::BF16, {spec.hidden, spec.linear.slot_count}, kStateImageAlignment,
                           "StateImage continuation hidden");
    if (spec.dflash_local) {
        const DFlashLocalStateSpec& dflash = *spec.dflash_local;
        out.dflash_local =
            plan_cyclic_kv_cache(builder, dflash.layers, dflash.capacity, dflash.kv_heads,
                                 dflash.head_dim, spec.linear.slot_count);
    }
    if (spec.kvarn) {
        KvarnContinuationStateLayout tails;
        tails.image  = plan_kvarn_continuation_image(*spec.kvarn);
        tails.images = builder.add_tensor(
            DType::U8,
            {checked_i32(tails.image.image_bytes, "KVarN continuation image exceeds int32"),
             spec.linear.slot_count},
            kStateImageAlignment, "StateImage KVarN continuation images");
        out.kvarn = std::move(tails);
    }

    out.host = plan_host_state_image(spec);
    return out;
}

TransferWork state_image_transfer_work(const StateImageHostLayout& layout) {
    std::size_t payload = checked_add(layout.linear_conv.bytes, layout.linear_recurrent.bytes,
                                      "StateImage transfer payload overflow");
    payload             = checked_add(payload, layout.continuation_hidden.bytes,
                                      "StateImage transfer payload overflow");
    if (layout.dflash_local_k) {
        if (!layout.spec.dflash_local || !layout.dflash_local_v) {
            throw std::invalid_argument("StateImage DFlash transfer layout is incomplete");
        }
        const std::size_t component_bytes =
            checked_mul(layout.dflash_local_layer_bytes, layout.spec.dflash_local->layers,
                        "StateImage transfer payload overflow");
        payload = checked_add(
            payload, checked_mul(component_bytes, 2U, "StateImage transfer payload overflow"),
            "StateImage transfer payload overflow");
    }
    if (layout.spec.kvarn) {
        payload = checked_add(payload, layout.kvarn_layout->image_bytes,
                              "StateImage transfer payload overflow");
    }
    const std::uint64_t operations =
        2ULL * layout.spec.linear.layers + 1ULL +
        (layout.spec.dflash_local ? 2ULL * layout.spec.dflash_local->layers : 0ULL) +
        (layout.spec.kvarn ? 1ULL : 0ULL);
    if (operations > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("StateImage transfer operation count exceeds uint32");
    }
    return TransferWork{.payload_bytes   = static_cast<std::uint64_t>(payload),
                        .copy_operations = static_cast<std::uint32_t>(operations)};
}

TransferWork dflash_local_transfer_work(const StateImageHostLayout& layout) {
    if (!layout.spec.dflash_local || !layout.dflash_local_k || !layout.dflash_local_v) {
        throw std::invalid_argument("StateImage has no DFlash local component");
    }
    const std::size_t component_bytes =
        checked_mul(layout.dflash_local_layer_bytes, layout.spec.dflash_local->layers,
                    "StateImage DFlash transfer payload overflow");
    const std::uint64_t operations = 2ULL * layout.spec.dflash_local->layers;
    if (component_bytes > std::numeric_limits<std::uint64_t>::max() / 2U ||
        operations > std::numeric_limits<std::uint32_t>::max()) {
        throw std::overflow_error("StateImage DFlash transfer work exceeds its representation");
    }
    return TransferWork{.payload_bytes   = static_cast<std::uint64_t>(2U * component_bytes),
                        .copy_operations = static_cast<std::uint32_t>(operations)};
}

HostStatePool::HostStatePool(StateImageHostLayout layout, std::uint32_t capacity)
    : layout_(std::move(layout)), slots_(capacity), free_slots_(capacity), free_count_(capacity) {
    if (!same_host_layout(layout_, plan_host_state_image(layout_.spec))) {
        throw std::invalid_argument("HostStatePool image layout is invalid");
    }
    const std::size_t bytes =
        checked_mul(layout_.image_bytes, capacity, "HostStatePool backing size overflow");
    if (bytes != 0) { backing_.emplace(bytes); }
    for (std::uint32_t index = 0; index < capacity; ++index) {
        free_slots_[index] = capacity - 1U - index;
    }
}

std::optional<HostStateSlotHandle> HostStatePool::allocate() noexcept {
    if (free_count_ == 0) { return std::nullopt; }
    const std::uint32_t index = free_slots_[--free_count_];
    Slot& slot                = slots_[index];
    slot.occupied             = true;
    ++occupied_;
    return HostStateSlotHandle{.index = index, .generation = slot.generation};
}

bool HostStatePool::release(HostStateSlotHandle handle) noexcept {
    if (!valid(handle)) { return false; }
    Slot& slot    = slots_[handle.index];
    slot.occupied = false;
    if (++slot.generation == 0) { ++slot.generation; }
    free_slots_[free_count_++] = handle.index;
    --occupied_;
    return true;
}

void HostStatePool::release_all() noexcept {
    for (std::uint32_t index = 0; index < slots_.size(); ++index) {
        Slot& slot = slots_[index];
        if (!slot.occupied) { continue; }
        slot.occupied = false;
        if (++slot.generation == 0) { ++slot.generation; }
        free_slots_[free_count_++] = index;
    }
    occupied_ = 0;
}

HostStateImageView HostStatePool::writable_view(HostStateSlotHandle handle) {
    if (!valid(handle)) { throw std::invalid_argument("HostStatePool handle is stale"); }
    return {.data = slot_data(handle.index), .layout = &layout_};
}

HostStateImageConstView HostStatePool::view(HostStateSlotHandle handle) const {
    if (!valid(handle)) { throw std::invalid_argument("HostStatePool handle is stale"); }
    return {.data = slot_data(handle.index), .layout = &layout_};
}

std::uint32_t HostStatePool::capacity() const noexcept {
    return static_cast<std::uint32_t>(slots_.size());
}

bool HostStatePool::valid(HostStateSlotHandle handle) const noexcept {
    return handle.index < slots_.size() && slots_[handle.index].occupied &&
           slots_[handle.index].generation == handle.generation;
}

std::byte* HostStatePool::slot_data(std::uint32_t index) const noexcept {
    return static_cast<std::byte*>(backing_->data()) +
           static_cast<std::size_t>(index) * layout_.image_bytes;
}

StateImageDevicePool::StateImageDevicePool(DeviceSpan backing, const StateImageDeviceLayout& layout)
    : StateImageDevicePool(std::span<const DeviceSpan>(&backing, 1), layout) {}

StateImageDevicePool::StateImageDevicePool(std::span<const DeviceSpan> backings,
                                           const StateImageDeviceLayout& layout)
    : shards_(layout.shards),
      continuation_hidden_(layout.continuation_hidden.bind(backings.front())),
      host_layout_(layout.host) {
    if (layout.shards.empty() || layout.shards.size() != layout.linear.size()) {
        throw std::invalid_argument("StateImage shard inventory is inconsistent");
    }
    linear_.reserve(layout.shards.size());
    for (std::size_t index = 0; index < layout.shards.size(); ++index) {
        if (layout.shards[index].rank >= backings.size()) {
            throw std::invalid_argument("StateImage shard names a rank with no backing");
        }
        linear_.push_back(std::make_unique<LinearAttentionStatePool>(
            backings[layout.shards[index].rank], layout.linear[index]));
    }
    if (continuation_hidden_.dtype != DType::BF16 || !continuation_hidden_.is_contiguous() ||
        continuation_hidden_.ne[0] != host_layout_.spec.hidden ||
        continuation_hidden_.ne[1] != linear_.front()->slot_count()) {
        throw std::invalid_argument("StateImage continuation hidden layout is inconsistent");
    }
    if (layout.dflash_local.has_value() != host_layout_.spec.dflash_local.has_value()) {
        throw std::invalid_argument("StateImage DFlash layout is inconsistent");
    }
    // The device components must add up to the host image's geometry: rebuild the whole-model spec
    // from the shards and compare.
    LinearAttentionStatePoolSpec whole = layout.linear.front().spec;
    whole.layers                       = 0;
    for (const auto& shard_layout : layout.linear) { whole.layers += shard_layout.spec.layers; }
    StateImageSpec device_spec{.linear = whole, .hidden = continuation_hidden_.ne[0]};
    if (layout.dflash_local) {
        device_spec.dflash_local = DFlashLocalStateSpec{
            .layers   = static_cast<std::uint32_t>(layout.dflash_local->k.size()),
            .capacity = layout.dflash_local->capacity,
            .kv_heads = layout.dflash_local->num_kv_heads,
            .head_dim = layout.dflash_local->head_dim,
        };
    }
    if (layout.kvarn.has_value() != host_layout_.spec.kvarn.has_value()) {
        throw std::invalid_argument("StateImage KVarN host layout is inconsistent");
    }
    if (layout.kvarn) {
        kvarn_images_ = layout.kvarn->images.bind(backings.front());
        kvarn_layout_ = layout.kvarn->image;
        const KvarnContinuationStateSpec& kvarn = kvarn_layout_->spec;
        if (kvarn_images_.dtype != DType::U8 ||
            kvarn_images_.ne[0] !=
                checked_i32(kvarn_layout_->image_bytes, "KVarN continuation image exceeds int32") ||
            kvarn_images_.ne[1] != linear_.front()->slot_count() ||
            !same_kvarn_layout(kvarn_layout_, std::optional<KvarnContinuationImageLayout>(
                                                  plan_kvarn_continuation_image(kvarn)))) {
            throw std::invalid_argument("StateImage KVarN layout is inconsistent");
        }
        kvarn_text_layers_ = kvarn.text_layers;
        kvarn_mtp_layers_  = kvarn.mtp_layers;
        device_spec.kvarn  = kvarn;
    }
    if (!same_host_layout(host_layout_, plan_host_state_image(device_spec))) {
        throw std::invalid_argument("StateImage host layout does not match its device components");
    }
    if (layout.dflash_local) {
        if (layout.dflash_local->lane_capacity != linear_.front()->slot_count()) {
            throw std::invalid_argument("StateImage components do not share one slot geometry");
        }
        dflash_local_.emplace(backings.front(), *layout.dflash_local);
    }
}

LinearAttentionStatePool* StateImageDevicePool::single_shard() const {
    if (linear_.size() != 1) {
        throw std::logic_error("StateImage pool is sharded across ranks: address a shard");
    }
    return linear_.front().get();
}

StateImageDeviceSlotView StateImageDevicePool::slot_view(std::int32_t slot) const {
    StateImageDeviceSlotView view{
        .continuation_hidden = continuation_hidden_slot(slot),
    };
    for (const auto& pool : linear_) { view.linear.push_back(pool->slot_view(slot)); }
    if (dflash_local_) { view.dflash_local = dflash_local_->slot_view(slot); }
    return view;
}

Tensor StateImageDevicePool::continuation_hidden_slot(std::int32_t slot) const {
    validate_slot(slot, slot_count(), "StateImage slot is out of range");
    return continuation_hidden_.slice(1, slot, 1).view({host_layout_.spec.hidden});
}

CyclicKVCache* StateImageDevicePool::dflash_local() noexcept {
    return dflash_local_ ? &*dflash_local_ : nullptr;
}

const CyclicKVCache* StateImageDevicePool::dflash_local() const noexcept {
    return dflash_local_ ? &*dflash_local_ : nullptr;
}

namespace {

ops::KvarnTailStateView kvarn_tail_view(const Tensor& image,
                                        const KvarnContinuationImageLayout& layout, bool mtp,
                                        std::uint32_t layer) {
    const std::uint32_t layers = mtp ? layout.spec.mtp_layers : layout.spec.text_layers;
    if (layer >= layers) { throw std::out_of_range("StateImage KVarN tail slot is out of range"); }
    const std::size_t k_offset =
        (mtp ? layout.mtp_k_offset : layout.text_k_offset) + layer * layout.tail_layer_bytes;
    const std::size_t v_offset =
        (mtp ? layout.mtp_v_offset : layout.text_v_offset) + layer * layout.tail_layer_bytes;
    const std::size_t marker_offset = (mtp ? layout.mtp_marker_offset : layout.text_marker_offset) +
                                      layer * layout.marker_layer_bytes;
    auto* base                   = static_cast<std::byte*>(image.data);
    const std::int32_t row_heads = layout.spec.kv_heads * ops::kKvarnTailSlots;
    return {
        .k             = Tensor(base + k_offset, DType::BF16,
                                {layout.spec.head_dim, ops::kKvarnGroup, row_heads}),
        .v             = Tensor(base + v_offset, DType::BF16,
                                {layout.spec.head_dim, ops::kKvarnGroup, row_heads}),
        .logical_pages = Tensor(base + marker_offset, DType::I32, {ops::kKvarnTailSlots}),
        .num_kv_heads  = layout.spec.kv_heads,
    };
}

} // namespace

ops::KvarnTailStateView StateImageDevicePool::kvarn_text_tail(std::uint32_t layer,
                                                              std::int32_t slot) const {
    if (!has_kvarn()) { throw std::logic_error("StateImage has no KVarN continuation state"); }
    validate_slot(slot, slot_count(), "StateImage KVarN slot is out of range");
    const Tensor image = kvarn_images_.slice(1, slot, 1).view({kvarn_images_.ne[0]});
    return kvarn_tail_view(image, *kvarn_layout_, false, layer);
}

ops::KvarnTailStateView StateImageDevicePool::kvarn_mtp_tail(std::uint32_t layer,
                                                             std::int32_t slot) const {
    if (!has_kvarn() || kvarn_mtp_layers_ == 0) {
        throw std::logic_error("StateImage has no KVarN MTP continuation state");
    }
    validate_slot(slot, slot_count(), "StateImage KVarN slot is out of range");
    const Tensor image = kvarn_images_.slice(1, slot, 1).view({kvarn_images_.ne[0]});
    return kvarn_tail_view(image, *kvarn_layout_, true, layer);
}

void StateImageDevicePool::zero_slot(std::int32_t slot, RankStreams streams) {
    validate_slot(slot, slot_count(), "StateImage zero slot is out of range");
    for (std::size_t index = 0; index < linear_.size(); ++index) {
        linear_[index]->zero_slot(slot, streams[shards_[index].rank]);
    }
    const cudaStream_t stream = streams[0];
    const Tensor hidden       = continuation_hidden_slot(slot);
    CUDA_CHECK(cudaMemsetAsync(hidden.data, 0, hidden.bytes(), stream));
    if (dflash_local_) {
        for (std::uint32_t layer = 0; layer < dflash_local_->layer_count(); ++layer) {
            const CyclicKVCacheLayerView view = dflash_local_->layer_view(layer);
            const Tensor k                    = view.k.slice(3, slot, 1);
            const Tensor v                    = view.v.slice(3, slot, 1);
            CUDA_CHECK(cudaMemsetAsync(k.data, 0, k.bytes(), stream));
            CUDA_CHECK(cudaMemsetAsync(v.data, 0, v.bytes(), stream));
        }
    }
    if (has_kvarn()) {
        const Tensor image = kvarn_images_.slice(1, slot, 1).view({kvarn_images_.ne[0]});
        CUDA_CHECK(cudaMemsetAsync(image.data, 0, image.bytes(), stream));
        const auto reset_markers = [&](const ops::KvarnTailStateView& tail) {
            CUDA_CHECK(
                cudaMemsetAsync(tail.logical_pages.data, 0xff, tail.logical_pages.bytes(), stream));
        };
        for (std::uint32_t layer = 0; layer < kvarn_text_layers_; ++layer) {
            reset_markers(kvarn_text_tail(layer, slot));
        }
        for (std::uint32_t layer = 0; layer < kvarn_mtp_layers_; ++layer) {
            reset_markers(kvarn_mtp_tail(layer, slot));
        }
    }
}

void StateImageDevicePool::zero_all(RankStreams streams) {
    for (std::size_t index = 0; index < linear_.size(); ++index) {
        linear_[index]->zero_all(streams[shards_[index].rank]);
    }
    const cudaStream_t stream = streams[0];
    CUDA_CHECK(cudaMemsetAsync(continuation_hidden_.data, 0, continuation_hidden_.bytes(), stream));
    if (dflash_local_) {
        for (std::uint32_t layer = 0; layer < dflash_local_->layer_count(); ++layer) {
            const CyclicKVCacheLayerView view = dflash_local_->layer_view(layer);
            CUDA_CHECK(cudaMemsetAsync(view.k.data, 0, view.k.bytes(), stream));
            CUDA_CHECK(cudaMemsetAsync(view.v.data, 0, view.v.bytes(), stream));
        }
    }
    if (has_kvarn()) {
        CUDA_CHECK(cudaMemsetAsync(kvarn_images_.data, 0, kvarn_images_.bytes(), stream));
        for (std::int32_t slot = 0; slot < slot_count(); ++slot) {
            for (std::uint32_t layer = 0; layer < kvarn_text_layers_; ++layer) {
                const Tensor markers = kvarn_text_tail(layer, slot).logical_pages;
                CUDA_CHECK(cudaMemsetAsync(markers.data, 0xff, markers.bytes(), stream));
            }
            for (std::uint32_t layer = 0; layer < kvarn_mtp_layers_; ++layer) {
                const Tensor markers = kvarn_mtp_tail(layer, slot).logical_pages;
                CUDA_CHECK(cudaMemsetAsync(markers.data, 0xff, markers.bytes(), stream));
            }
        }
    }
}

void StateImageDevicePool::copy_slot(std::int32_t source, std::int32_t destination,
                                     RankStreams streams) {
    validate_slot(source, slot_count(), "StateImage copy source is out of range");
    validate_slot(destination, slot_count(), "StateImage copy destination is out of range");
    if (source == destination) { return; }
    for (std::size_t index = 0; index < linear_.size(); ++index) {
        linear_[index]->copy_slot(source, destination, streams[shards_[index].rank]);
    }
    const cudaStream_t stream       = streams[0];
    const Tensor source_hidden      = continuation_hidden_slot(source);
    const Tensor destination_hidden = continuation_hidden_slot(destination);
    CUDA_CHECK(cudaMemcpyAsync(destination_hidden.data, source_hidden.data,
                               destination_hidden.bytes(), cudaMemcpyDeviceToDevice, stream));
    if (dflash_local_) {
        dflash_local_->copy_slot_from(*dflash_local_, source, destination, stream);
    }
    if (has_kvarn()) {
        const Tensor source_image = kvarn_images_.slice(1, source, 1).view({kvarn_images_.ne[0]});
        const Tensor destination_image =
            kvarn_images_.slice(1, destination, 1).view({kvarn_images_.ne[0]});
        CUDA_CHECK(cudaMemcpyAsync(destination_image.data, source_image.data, source_image.bytes(),
                                   cudaMemcpyDeviceToDevice, stream));
    }
}

void StateImageDevicePool::copy_dflash_local(std::int32_t source, std::int32_t destination,
                                             RankStreams streams) {
    const cudaStream_t stream = streams[0];
    validate_slot(source, slot_count(), "StateImage DFlash copy source is out of range");
    validate_slot(destination, slot_count(), "StateImage DFlash copy destination is out of range");
    if (!dflash_local_) { throw std::logic_error("StateImage has no DFlash local component"); }
    if (source != destination) {
        dflash_local_->copy_slot_from(*dflash_local_, source, destination, stream);
    }
}

void StateImageDevicePool::validate_host_layout(const StateImageHostLayout* layout,
                                                const std::byte* data) const {
    if (layout == nullptr || data == nullptr || !same_host_layout(*layout, host_layout_)) {
        throw std::invalid_argument("Host and device StateImage layouts do not match");
    }
}

// Visits the Device components of one slot as (rank, device pointer, packed host offset, bytes):
// every Linear Attention layer from the shard that holds it, then the rest on rank 0.
template <class Visit>
void StateImageDevicePool::for_each_host_component(std::int32_t slot, Visit&& visit) const {
    std::uint32_t layers = 0;
    for (const auto& shard : linear_) { layers += shard->layer_count(); }
    for (std::uint32_t layer = 0; layer < layers; ++layer) {
        for_each_host_component(
            slot, StateImagePart{.kind = StateImagePart::Kind::LinearLayer, .layer = layer}, visit);
    }
    for_each_host_component(slot, StateImagePart{.kind = StateImagePart::Kind::Rest}, visit);
}

template <class Visit>
void StateImageDevicePool::for_each_host_component(std::int32_t slot, StateImagePart part,
                                                   Visit&& visit) const {
    if (part.kind == StateImagePart::Kind::LinearLayer) {
        for (std::size_t index = 0; index < linear_.size(); ++index) {
            const std::uint32_t first = shards_[index].first_layer;
            if (part.layer < first || part.layer >= first + linear_[index]->layer_count()) {
                continue;
            }
            const std::size_t rank    = shards_[index].rank;
            const std::uint32_t local = part.layer - first;
            const Tensor conv         = linear_[index]->conv_slot(local, slot);
            visit(rank, conv.data,
                  host_layout_.linear_conv.offset +
                      part.layer * host_layout_.linear_conv_layer_bytes,
                  conv.bytes());
            const Tensor recurrent = linear_[index]->recurrent_slot(local, slot);
            visit(rank, recurrent.data,
                  host_layout_.linear_recurrent.offset +
                      part.layer * host_layout_.linear_recurrent_layer_bytes,
                  recurrent.bytes());
            return;
        }
        throw std::out_of_range("StateImage linear layer is out of range");
    }
    const Tensor hidden = continuation_hidden_slot(slot);
    visit(std::size_t{0}, hidden.data, host_layout_.continuation_hidden.offset, hidden.bytes());
    if (dflash_local_) {
        for (std::uint32_t layer = 0; layer < dflash_local_->layer_count(); ++layer) {
            const CyclicKVCacheLayerView view = dflash_local_->layer_view(layer);
            const Tensor k                    = view.k.slice(3, slot, 1);
            const Tensor v                    = view.v.slice(3, slot, 1);
            visit(std::size_t{0}, k.data,
                  host_layout_.dflash_local_k->offset +
                      layer * host_layout_.dflash_local_layer_bytes,
                  k.bytes());
            visit(std::size_t{0}, v.data,
                  host_layout_.dflash_local_v->offset +
                      layer * host_layout_.dflash_local_layer_bytes,
                  v.bytes());
        }
    }
    if (has_kvarn()) {
        const Tensor image = kvarn_images_.slice(1, slot, 1).view({kvarn_images_.ne[0]});
        visit(std::size_t{0}, image.data, host_layout_.kvarn->offset, image.bytes());
    }
}

void StateImageDevicePool::copy_to_host(std::int32_t source, HostStateImageView destination,
                                        RankStreams streams) const {
    validate_slot(source, slot_count(), "StateImage copy-to-host source is out of range");
    validate_host_layout(destination.layout, destination.data);
    // Each shard's layers land at their global offsets in the one host image.
    for (std::size_t index = 0; index < linear_.size(); ++index) {
        const cudaStream_t shard_stream = streams[shards_[index].rank];
        for (std::uint32_t local = 0; local < linear_[index]->layer_count(); ++local) {
            const std::size_t layer = shards_[index].first_layer + local;
            const Tensor conv       = linear_[index]->conv_slot(local, source);
            CUDA_CHECK(cudaMemcpyAsync(
                byte_offset(destination.data, host_layout_.linear_conv.offset +
                                                  layer * host_layout_.linear_conv_layer_bytes),
                conv.data, conv.bytes(), cudaMemcpyDeviceToHost, shard_stream));
            const Tensor recurrent = linear_[index]->recurrent_slot(local, source);
            CUDA_CHECK(cudaMemcpyAsync(
                byte_offset(destination.data,
                            host_layout_.linear_recurrent.offset +
                                layer * host_layout_.linear_recurrent_layer_bytes),
                recurrent.data, recurrent.bytes(), cudaMemcpyDeviceToHost, shard_stream));
        }
    }
    const cudaStream_t stream = streams[0];
    const Tensor hidden       = continuation_hidden_slot(source);
    CUDA_CHECK(
        cudaMemcpyAsync(byte_offset(destination.data, host_layout_.continuation_hidden.offset),
                        hidden.data, hidden.bytes(), cudaMemcpyDeviceToHost, stream));
    if (dflash_local_) {
        for (std::uint32_t layer = 0; layer < dflash_local_->layer_count(); ++layer) {
            const CyclicKVCacheLayerView view = dflash_local_->layer_view(layer);
            const Tensor k                    = view.k.slice(3, source, 1);
            const Tensor v                    = view.v.slice(3, source, 1);
            CUDA_CHECK(cudaMemcpyAsync(
                byte_offset(destination.data, host_layout_.dflash_local_k->offset +
                                                  layer * host_layout_.dflash_local_layer_bytes),
                k.data, k.bytes(), cudaMemcpyDeviceToHost, stream));
            CUDA_CHECK(cudaMemcpyAsync(
                byte_offset(destination.data, host_layout_.dflash_local_v->offset +
                                                  layer * host_layout_.dflash_local_layer_bytes),
                v.data, v.bytes(), cudaMemcpyDeviceToHost, stream));
        }
    }
    if (has_kvarn()) {
        const Tensor image = kvarn_images_.slice(1, source, 1).view({kvarn_images_.ne[0]});
        CUDA_CHECK(cudaMemcpyAsync(byte_offset(destination.data, host_layout_.kvarn->offset),
                                   image.data, image.bytes(), cudaMemcpyDeviceToHost, stream));
    }
}

void StateImageDevicePool::copy_from_host(HostStateImageConstView source, std::int32_t destination,
                                          RankStreams streams) {
    validate_slot(destination, slot_count(),
                  "StateImage copy-from-host destination is out of range");
    validate_host_layout(source.layout, source.data);
    for (std::size_t index = 0; index < linear_.size(); ++index) {
        const cudaStream_t shard_stream = streams[shards_[index].rank];
        for (std::uint32_t local = 0; local < linear_[index]->layer_count(); ++local) {
            const std::size_t layer = shards_[index].first_layer + local;
            const Tensor conv       = linear_[index]->conv_slot(local, destination);
            CUDA_CHECK(cudaMemcpyAsync(
                conv.data,
                byte_offset(source.data, host_layout_.linear_conv.offset +
                                             layer * host_layout_.linear_conv_layer_bytes),
                conv.bytes(), cudaMemcpyHostToDevice, shard_stream));
            const Tensor recurrent = linear_[index]->recurrent_slot(local, destination);
            CUDA_CHECK(cudaMemcpyAsync(
                recurrent.data,
                byte_offset(source.data, host_layout_.linear_recurrent.offset +
                                             layer * host_layout_.linear_recurrent_layer_bytes),
                recurrent.bytes(), cudaMemcpyHostToDevice, shard_stream));
        }
    }
    const cudaStream_t stream = streams[0];
    const Tensor hidden       = continuation_hidden_slot(destination);
    CUDA_CHECK(cudaMemcpyAsync(hidden.data,
                               byte_offset(source.data, host_layout_.continuation_hidden.offset),
                               hidden.bytes(), cudaMemcpyHostToDevice, stream));
    if (dflash_local_) {
        for (std::uint32_t layer = 0; layer < dflash_local_->layer_count(); ++layer) {
            const CyclicKVCacheLayerView view = dflash_local_->layer_view(layer);
            const Tensor k                    = view.k.slice(3, destination, 1);
            const Tensor v                    = view.v.slice(3, destination, 1);
            CUDA_CHECK(cudaMemcpyAsync(
                k.data,
                byte_offset(source.data, host_layout_.dflash_local_k->offset +
                                             layer * host_layout_.dflash_local_layer_bytes),
                k.bytes(), cudaMemcpyHostToDevice, stream));
            CUDA_CHECK(cudaMemcpyAsync(
                v.data,
                byte_offset(source.data, host_layout_.dflash_local_v->offset +
                                             layer * host_layout_.dflash_local_layer_bytes),
                v.bytes(), cudaMemcpyHostToDevice, stream));
        }
    }
    if (has_kvarn()) {
        const Tensor image = kvarn_images_.slice(1, destination, 1).view({kvarn_images_.ne[0]});
        CUDA_CHECK(cudaMemcpyAsync(image.data, byte_offset(source.data, host_layout_.kvarn->offset),
                                   image.bytes(), cudaMemcpyHostToDevice, stream));
    }
}

namespace {

// Visits the pieces of the host byte range [offset, offset + bytes) that fall in each fixed-size
// segment of a segmented host image.
template <class Visit>
void split_segments(std::size_t offset, std::size_t bytes, std::size_t segment_bytes,
                    Visit&& visit) {
    std::size_t done = 0;
    while (done < bytes) {
        const std::size_t position = offset + done;
        const std::size_t segment  = position / segment_bytes;
        const std::size_t within   = position % segment_bytes;
        const std::size_t count    = std::min(bytes - done, segment_bytes - within);
        visit(segment, within, done, count);
        done += count;
    }
}

void validate_segments(std::size_t segment_count, std::size_t segment_bytes,
                       std::size_t image_bytes) {
    if (segment_bytes == 0 || segment_count == 0 ||
        segment_count < 1U + (image_bytes - 1U) / segment_bytes) {
        throw std::invalid_argument("segmented StateImage host copy does not cover the image");
    }
}

} // namespace

void StateImageDevicePool::copy_to_host_segments(std::int32_t source,
                                                 std::span<std::byte* const> segments,
                                                 std::size_t segment_bytes,
                                                 RankStreams streams) const {
    validate_slot(source, slot_count(), "StateImage segmented D2H source is out of range");
    validate_segments(segments.size(), segment_bytes, host_layout_.image_bytes);
    for_each_host_component(
        source, [&](std::size_t rank, void* device, std::size_t offset, std::size_t bytes) {
            split_segments(
                offset, bytes, segment_bytes,
                [&](std::size_t segment, std::size_t within, std::size_t done, std::size_t count) {
                    CUDA_CHECK(cudaMemcpyAsync(segments[segment] + within,
                                               static_cast<const std::byte*>(device) + done, count,
                                               cudaMemcpyDeviceToHost, streams[rank]));
                });
        });
}

void StateImageDevicePool::copy_from_host_segments(std::span<const std::byte* const> segments,
                                                   std::size_t segment_bytes,
                                                   std::int32_t destination, RankStreams streams) {
    std::uint32_t layers = 0;
    for (const auto& shard : linear_) { layers += shard->layer_count(); }
    for (std::uint32_t layer = 0; layer < layers; ++layer) {
        copy_from_host_segments(
            segments, segment_bytes, destination,
            StateImagePart{.kind = StateImagePart::Kind::LinearLayer, .layer = layer}, streams);
    }
    copy_from_host_segments(segments, segment_bytes, destination,
                            StateImagePart{.kind = StateImagePart::Kind::Rest}, streams);
}

void StateImageDevicePool::copy_from_host_segments(std::span<const std::byte* const> segments,
                                                   std::size_t segment_bytes,
                                                   std::int32_t destination, StateImagePart part,
                                                   RankStreams streams) {
    validate_slot(destination, slot_count(),
                  "StateImage segmented H2D destination is out of range");
    validate_segments(segments.size(), segment_bytes, host_layout_.image_bytes);
    for_each_host_component(
        destination, part,
        [&](std::size_t rank, void* device, std::size_t offset, std::size_t bytes) {
            split_segments(
                offset, bytes, segment_bytes,
                [&](std::size_t segment, std::size_t within, std::size_t done, std::size_t count) {
                    CUDA_CHECK(cudaMemcpyAsync(static_cast<std::byte*>(device) + done,
                                               segments[segment] + within, count,
                                               cudaMemcpyHostToDevice, streams[rank]));
                });
        });
}

} // namespace ninfer::models::qwen3_5
