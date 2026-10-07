// KVarN continuation tail: the StateImage image that carries each full-attention layer's KVarN
// sink/tail (plus the MTP pool's) across a checkpoint save/restore or a prefix reuse.
//
// The host section is device-free: it checks the image geometry against the paged KV tail geometry
// and runs a capture -> activate round trip byte for byte, token by token. The device section
// exercises the real StateImageDevicePool: its kvarn tail views, `copy_slot`, `zero_slot` and the
// reverse copy back into a paged-shaped buffer.
//
// The whole-model acceptance (a second request reusing a prefix must report cached prompt tokens
// and match the first request's message byte for byte) runs against the 27B artifact through
// `.deps/kvarn-adm/p1_prefix_reuse.sh`; this suite covers the state-image layer it rides on.
#include "core/arena.h"
#include "core/device.h"
#include "core/layout.h"
#include "core/tensor.h"
#include "models/qwen3_5/state/state_image.h"
#include "ninfer/ops/kvarn.h"

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace q36 = ninfer::models::qwen3_5;
namespace ops = ninfer::ops;

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) { return; }
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

bool cuda_unavailable(cudaError_t error) {
    return error == cudaErrorNoDevice || error == cudaErrorInsufficientDriver;
}

constexpr std::uint32_t kTextLayers = 16; // the model's full-attention layers
constexpr std::uint32_t kMtpLayers  = 1;
constexpr std::int32_t kKvHeads     = 4;
constexpr std::int32_t kKvarnSlots  = 2;

q36::KvarnContinuationStateSpec kvarn_spec(std::uint32_t text_layers, std::uint32_t mtp_layers) {
    return q36::KvarnContinuationStateSpec{.text_layers = text_layers,
                                           .mtp_layers  = mtp_layers,
                                           .kv_heads    = kKvHeads,
                                           .head_dim    = ops::kKvarnHeadDim};
}

q36::StateImageSpec make_spec(std::optional<q36::KvarnContinuationStateSpec> kvarn) {
    q36::StateImageSpec spec{
        .linear =
            {
                .layers         = 4,
                .conv_channels  = 8,
                .conv_width     = 3,
                .value_heads    = 2,
                .value_head_dim = 4,
                .key_head_dim   = 3,
                .slot_count     = kKvarnSlots,
                .conv_dtype     = ninfer::DType::BF16,
            },
        .hidden = 17,
    };
    spec.kvarn = kvarn;
    return spec;
}

struct Planned {
    q36::StateImageDeviceLayout layout;
    std::size_t bytes = 0;
};

Planned plan(std::optional<q36::KvarnContinuationStateSpec> kvarn) {
    ninfer::LayoutBuilder builder;
    q36::StateImageDeviceLayout layout =
        q36::plan_state_image_device_pool(builder, make_spec(kvarn));
    return {.layout = std::move(layout), .bytes = builder.finish(256)};
}

// One layer of the paged KV tail, from the public KVarN geometry the paged views declare:
// K/V are `BF16 [D, G, Hkv * tail_slots]` and the markers are `I32 [tail_slots]`.
std::size_t paged_tail_layer_bytes() {
    const ninfer::Tensor tail(nullptr, ninfer::DType::BF16,
                              {ops::kKvarnHeadDim, ops::kKvarnGroup,
                               kKvHeads * ops::kKvarnTailSlots});
    return tail.bytes();
}

std::size_t paged_marker_layer_bytes() {
    const ninfer::Tensor markers(nullptr, ninfer::DType::I32, {ops::kKvarnTailSlots});
    return markers.bytes();
}

std::size_t paged_token_bytes() {
    return static_cast<std::size_t>(kKvHeads) * ops::kKvarnTailSlots * ops::kKvarnHeadDim *
           sizeof(std::uint16_t);
}

// The KVarN image must be sized for the paged KV tail it carries, or the capture/activate memcpys
// in `program/storage/context.cpp` silently overrun or truncate a layer's tail.
void test_image_geometry() {
    Planned planned = plan(kvarn_spec(kTextLayers, kMtpLayers));
    const q36::StateImageHostLayout& host = planned.layout.host;
    expect(host.kvarn.has_value() && host.kvarn_layout.has_value(), "kvarn host layout is present");
    if (!host.kvarn_layout) { return; }
    const q36::KvarnContinuationImageLayout& image = *host.kvarn_layout;

    expect(image.spec.text_layers == kTextLayers && image.spec.mtp_layers == kMtpLayers &&
               image.spec.kv_heads == kKvHeads && image.spec.head_dim == ops::kKvarnHeadDim,
           "kvarn image records its continuation spec");
    expect(image.tail_layer_bytes == paged_tail_layer_bytes(),
           "kvarn image tail layer stride must equal the paged KV tail layer size");
    expect(image.marker_layer_bytes == paged_marker_layer_bytes(),
           "kvarn image marker layer stride must equal the paged marker layer size");
    expect(paged_token_bytes() * ops::kKvarnGroup == image.tail_layer_bytes,
           "one KVarN page of tokens covers a whole tail layer");

    // The plan's per-sequence KVarN sink/tail footprint: 16 text layers, K and V.
    expect(static_cast<std::uint64_t>(image.tail_layer_bytes) * 2U * kTextLayers ==
               24ULL * 1024 * 1024,
           "16 text layers of K+V KVarN sink/tail must be 24 MiB");

    const std::size_t text_k_end = image.text_k_offset + image.tail_layer_bytes * kTextLayers;
    const std::size_t text_v_end = image.text_v_offset + image.tail_layer_bytes * kTextLayers;
    const std::size_t text_m_end = image.text_marker_offset + image.marker_layer_bytes * kTextLayers;
    const std::size_t mtp_k_end  = image.mtp_k_offset + image.tail_layer_bytes * kMtpLayers;
    const std::size_t mtp_v_end  = image.mtp_v_offset + image.tail_layer_bytes * kMtpLayers;
    const std::size_t mtp_m_end  = image.mtp_marker_offset + image.marker_layer_bytes * kMtpLayers;
    for (const std::size_t end : {text_k_end, text_v_end, text_m_end, mtp_k_end, mtp_v_end,
                                  mtp_m_end}) {
        expect(end <= image.image_bytes, "a kvarn image component must fit inside the image");
    }
    expect(image.text_k_offset % 256 == 0 && image.text_v_offset % 256 == 0 &&
               image.text_marker_offset % 256 == 0 && image.mtp_k_offset % 256 == 0,
           "kvarn image components are 256 B aligned");
    expect(image.text_v_offset >= text_k_end && image.text_marker_offset >= text_v_end &&
               image.mtp_k_offset >= text_m_end && image.mtp_v_offset >= mtp_k_end &&
               image.mtp_marker_offset >= mtp_v_end,
           "kvarn image components must not overlap");

    expect(planned.layout.kvarn.has_value(), "kvarn device region is planned");
    if (planned.layout.kvarn) {
        expect(planned.layout.kvarn->images.dtype == ninfer::DType::U8,
               "kvarn continuation image is a U8 blob");
        expect(planned.layout.kvarn->images.shape[0] ==
                   static_cast<std::int32_t>(image.image_bytes),
               "kvarn image blob spans one image");
        expect(planned.layout.kvarn->images.shape[1] == kKvarnSlots,
               "kvarn image has one entry per StateImage slot");
        expect(planned.layout.kvarn->images.region.bytes >= image.image_bytes * kKvarnSlots,
               "the kvarn device region holds one image per slot");
        // The device region and the packed host image are planned independently -- the pool
        // constructor cross-checks them by rebuilding the host layout from the device components --
        // so the host image covers exactly one slot's worth of continuation bytes.
        expect(host.kvarn->bytes == image.image_bytes,
               "the host image packs one slot's kvarn continuation");
    }

    // Without an MTP pool the image carries the text layers only.
    Planned no_mtp = plan(kvarn_spec(kTextLayers, 0));
    const q36::KvarnContinuationImageLayout& no_mtp_image = *no_mtp.layout.host.kvarn_layout;
    expect(no_mtp_image.mtp_k_offset == 0 && no_mtp_image.mtp_v_offset == 0 &&
               no_mtp_image.mtp_marker_offset == 0,
           "an MTP-less kvarn image has no MTP offsets");
    expect(no_mtp_image.image_bytes + image.tail_layer_bytes * 2U * kMtpLayers <=
               image.image_bytes,
           "the MTP pool adds its own K/V to the kvarn image");
}

void test_transfer_work() {
    const q36::StateImageHostLayout host =
        plan(kvarn_spec(kTextLayers, kMtpLayers)).layout.host;
    const ninfer::TransferWork work = q36::state_image_transfer_work(host);
    const std::size_t base_payload =
        host.linear_conv.bytes + host.linear_recurrent.bytes + host.continuation_hidden.bytes;
    expect(work.payload_bytes == base_payload + host.kvarn->bytes,
           "the kvarn image joins the StateImage transfer payload");
    expect(work.copy_operations == 2U * host.spec.linear.layers + 1U + 1U,
           "the kvarn image is one host-image copy operation");

    const q36::StateImageHostLayout plain = plan(std::nullopt).layout.host;
    expect(!plain.kvarn.has_value(), "a spec without kvarn carries no image");
    const ninfer::TransferWork plain_work = q36::state_image_transfer_work(plain);
    expect(plain_work.copy_operations + 1U == work.copy_operations &&
               plain_work.payload_bytes < work.payload_bytes,
           "the kvarn image adds exactly one transfer operation and its bytes");
}

// Capture -> activate round trip at the host layout level. The source is a paged KV tail written
// per token; capture lands each layer's K/V/markers at the image offsets the pool views use, then
// the source is zeroed (a fresh or reused sequence) and activate copies the image back.
void test_host_capture_activate_roundtrip() {
    const q36::StateImageHostLayout host =
        plan(kvarn_spec(kTextLayers, kMtpLayers)).layout.host;
    const q36::KvarnContinuationImageLayout& image = *host.kvarn_layout;
    const std::size_t tail                         = image.tail_layer_bytes;
    const std::size_t marker                       = image.marker_layer_bytes;
    const std::size_t layer_bytes                  = 2U * tail + marker;
    const std::uint32_t text_component             = 0;
    const std::uint32_t mtp_component              = kTextLayers;

    std::vector<std::uint8_t> paged(layer_bytes * (kTextLayers + kMtpLayers));
    for (std::size_t index = 0; index < paged.size(); ++index) {
        paged[index] = static_cast<std::uint8_t>((index * 131U + 7U) & 0xffU);
    }

    std::vector<std::uint8_t> blob(host.kvarn->bytes, 0U);
    const auto transfer = [&](std::uint32_t layers, std::uint32_t component, std::size_t k_offset,
                              std::size_t v_offset, std::size_t marker_offset, bool to_image) {
        for (std::uint32_t index = 0; index < layers; ++index) {
            const std::size_t base = static_cast<std::size_t>(component + index) * layer_bytes;
            if (to_image) {
                std::memcpy(blob.data() + k_offset + index * tail, paged.data() + base, tail);
                std::memcpy(blob.data() + v_offset + index * tail, paged.data() + base + tail,
                            tail);
                std::memcpy(blob.data() + marker_offset + index * marker,
                            paged.data() + base + 2U * tail, marker);
            } else {
                std::memcpy(paged.data() + base, blob.data() + k_offset + index * tail, tail);
                std::memcpy(paged.data() + base + tail, blob.data() + v_offset + index * tail,
                            tail);
                std::memcpy(paged.data() + base + 2U * tail,
                            blob.data() + marker_offset + index * marker, marker);
            }
        }
    };
    transfer(kTextLayers, text_component, image.text_k_offset, image.text_v_offset,
             image.text_marker_offset, true);
    transfer(kMtpLayers, mtp_component, image.mtp_k_offset, image.mtp_v_offset,
             image.mtp_marker_offset, true);

    const std::vector<std::uint8_t> snapshot = paged;
    std::memset(paged.data(), 0, paged.size());

    transfer(kTextLayers, text_component, image.text_k_offset, image.text_v_offset,
             image.text_marker_offset, false);
    transfer(kMtpLayers, mtp_component, image.mtp_k_offset, image.mtp_v_offset,
             image.mtp_marker_offset, false);

    expect(paged == snapshot, "capture then activate must restore every paged byte");

    // Token by token: each of the 128 tokens in a page is a contiguous run of Hkv*tail_slots rows.
    const std::size_t token = paged_token_bytes();
    bool tokens_equal       = true;
    for (std::uint32_t index = 0; index < kTextLayers + kMtpLayers; ++index) {
        const std::size_t base = static_cast<std::size_t>(index) * layer_bytes;
        for (int at = 0; at < ops::kKvarnGroup; ++at) {
            const std::size_t offset = base + static_cast<std::size_t>(at) * token;
            tokens_equal = tokens_equal &&
                           std::memcmp(paged.data() + offset, snapshot.data() + offset, token) == 0;
        }
    }
    expect(tokens_equal, "every KVarN tail token must survive the round trip");
}

void test_spec_validation() {
    const auto rejects = [](const q36::KvarnContinuationStateSpec& kvarn) {
        ninfer::LayoutBuilder builder;
        try {
            (void)q36::plan_state_image_device_pool(builder, make_spec(kvarn));
        } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    q36::KvarnContinuationStateSpec bad_head = kvarn_spec(kTextLayers, kMtpLayers);
    bad_head.head_dim                        = 128;
    expect(rejects(bad_head), "a non-D256 kvarn continuation head dim must be rejected");
    expect(rejects(kvarn_spec(0, kMtpLayers)),
           "a kvarn continuation with no text layers must be rejected");
    q36::KvarnContinuationStateSpec no_heads = kvarn_spec(kTextLayers, kMtpLayers);
    no_heads.kv_heads                        = 0;
    expect(rejects(no_heads), "a kvarn continuation with no KV heads must be rejected");
}

void fill_tensor(const ninfer::Tensor& tensor, std::uint8_t value) {
    CUDA_CHECK(cudaMemset(tensor.data, value, tensor.bytes()));
}

void copy_to_tensor(const ninfer::Tensor& tensor, const std::uint8_t* source) {
    CUDA_CHECK(cudaMemcpy(tensor.data, source, tensor.bytes(), cudaMemcpyHostToDevice));
}

std::vector<std::uint8_t> read_tensor(const ninfer::Tensor& tensor) {
    std::vector<std::uint8_t> host(tensor.bytes());
    CUDA_CHECK(cudaMemcpy(host.data(), tensor.data, host.size(), cudaMemcpyDeviceToHost));
    return host;
}

bool all_equal(const std::vector<std::uint8_t>& bytes, std::uint8_t value) {
    for (const std::uint8_t byte : bytes) {
        if (byte != value) { return false; }
    }
    return true;
}

// The real StateImageDevicePool carries the KVarN tail. Capture is a whole-layer D2D copy from the
// paged tail view; activate is the reverse. Both are byte copies, so the pool must expose the paged
// geometry and preserve every byte, including the retired-page markers.
void test_device_roundtrip(ninfer::DeviceContext& device) {
    const Planned planned = plan(kvarn_spec(kTextLayers, kMtpLayers));
    ninfer::DeviceArena arena(planned.bytes);
    q36::StateImageDevicePool pool({arena.base(), arena.capacity()}, planned.layout);
    expect(pool.has_kvarn(), "the pool exposes its KVarN continuation image");
    expect(pool.kvarn_text_layers() == kTextLayers && pool.kvarn_mtp_layers() == kMtpLayers,
           "the pool reports its KVarN text and MTP layer counts");

    const std::size_t tail   = paged_tail_layer_bytes();
    const std::size_t marker = paged_marker_layer_bytes();

    pool.zero_all(device.stream);
    device.synchronize();
    expect(all_equal(read_tensor(pool.kvarn_text_tail(0, 0).k), 0) &&
               all_equal(read_tensor(pool.kvarn_text_tail(0, 0).v), 0),
           "zero_all clears the KVarN tail");
    expect(all_equal(read_tensor(pool.kvarn_text_tail(0, 0).logical_pages), 0xff),
           "zero_all retires the KVarN markers");
    expect(all_equal(read_tensor(pool.kvarn_mtp_tail(0, 1).k), 0),
           "zero_all clears the MTP KVarN tail");

    std::vector<std::uint8_t> k_pattern(tail);
    std::vector<std::uint8_t> v_pattern(tail);
    for (std::size_t index = 0; index < tail; ++index) {
        k_pattern[index] = static_cast<std::uint8_t>((index * 29U + 3U) & 0xffU);
        v_pattern[index] = static_cast<std::uint8_t>((index * 17U + 91U) & 0xffU);
    }
    std::vector<std::uint8_t> marker_pattern(marker);
    for (std::size_t index = 0; index < marker; ++index) {
        marker_pattern[index] = static_cast<std::uint8_t>((index * 37U + 11U) & 0xffU);
    }

    const ops::KvarnTailStateView text = pool.kvarn_text_tail(0, 0);
    expect(text.num_kv_heads == kKvHeads, "the pool tail view reports the KV head count");
    expect(text.k.dtype == ninfer::DType::BF16 && text.k.bytes() == tail &&
               text.v.bytes() == tail && text.logical_pages.bytes() == marker,
           "the pool tail view matches the paged KV tail geometry");
    copy_to_tensor(text.k, k_pattern.data());
    copy_to_tensor(text.v, v_pattern.data());
    copy_to_tensor(text.logical_pages, marker_pattern.data());
    device.synchronize();

    // A D2D slot copy carries the whole image; the source slot is then retired.
    pool.copy_slot(0, 1, device.stream);
    pool.zero_slot(0, device.stream);
    device.synchronize();
    const ops::KvarnTailStateView copied = pool.kvarn_text_tail(0, 1);
    expect(read_tensor(copied.k) == k_pattern && read_tensor(copied.v) == v_pattern &&
               read_tensor(copied.logical_pages) == marker_pattern,
           "the captured KVarN tail survives the slot copy");
    const ops::KvarnTailStateView cleared = pool.kvarn_text_tail(0, 0);
    expect(all_equal(read_tensor(cleared.k), 0) && all_equal(read_tensor(cleared.v), 0),
           "zero_slot clears the KVarN tail");
    expect(all_equal(read_tensor(cleared.logical_pages), 0xff),
           "zero_slot retires the KVarN markers");

    // Activate: copy the slot back into paged-shaped buffers and compare every byte.
    ninfer::DeviceBuffer restored_k(tail);
    ninfer::DeviceBuffer restored_v(tail);
    ninfer::DeviceBuffer restored_m(marker);
    CUDA_CHECK(cudaMemcpyAsync(restored_k.p, copied.k.data, tail, cudaMemcpyDeviceToDevice,
                               device.stream));
    CUDA_CHECK(cudaMemcpyAsync(restored_v.p, copied.v.data, tail, cudaMemcpyDeviceToDevice,
                               device.stream));
    CUDA_CHECK(cudaMemcpyAsync(restored_m.p, copied.logical_pages.data, marker,
                               cudaMemcpyDeviceToDevice, device.stream));
    device.synchronize();
    const auto as_bytes = [](const void* data, std::size_t bytes) {
        return ninfer::Tensor(const_cast<void*>(data), ninfer::DType::U8,
                              {static_cast<std::int32_t>(bytes)});
    };
    expect(read_tensor(as_bytes(restored_k.p, tail)) == k_pattern,
           "the activated KVarN tail token bytes match the captured ones");
    expect(read_tensor(as_bytes(restored_v.p, tail)) == v_pattern,
           "the activated KVarN tail value bytes match the captured ones");
    expect(read_tensor(as_bytes(restored_m.p, marker)) == marker_pattern,
           "the activated KVarN markers match the captured ones");

    // The MTP pool is a separate component: writing it must not touch the text pool.
    const ops::KvarnTailStateView mtp = pool.kvarn_mtp_tail(0, 0);
    fill_tensor(mtp.k, 0x5aU);
    device.synchronize();
    expect(all_equal(read_tensor(mtp.k), 0x5aU), "the MTP KVarN tail view writes its own slot");
    expect(all_equal(read_tensor(pool.kvarn_text_tail(0, 0).k), 0),
           "the MTP pool must not alias the text pool");

    const auto throws_out_of_range = [&](bool mtp_layer) {
        try {
            const ops::KvarnTailStateView view =
                mtp_layer ? pool.kvarn_mtp_tail(kMtpLayers, 0) : pool.kvarn_text_tail(kTextLayers, 0);
            (void)view;
        } catch (const std::out_of_range&) { return true; }
        return false;
    };
    expect(throws_out_of_range(false), "an out-of-range kvarn text layer must be rejected");
    expect(throws_out_of_range(true), "an out-of-range kvarn MTP layer must be rejected");
    bool slot_rejected = false;
    try {
        (void)pool.kvarn_text_tail(0, kKvarnSlots);
    } catch (const std::out_of_range&) { slot_rejected = true; }
    expect(slot_rejected, "an out-of-range StateImage slot must be rejected");
}

} // namespace

int main() {
    test_image_geometry();
    test_transfer_work();
    test_host_capture_activate_roundtrip();
    test_spec_validation();

    int count                   = 0;
    const cudaError_t count_err = cudaGetDeviceCount(&count);
    if (cuda_unavailable(count_err) || count == 0) {
        std::cout << "SKIP: no usable CUDA device (host KVarN continuation image checks ran)\n";
        return failures == 0 ? 0 : 1;
    }
    CUDA_CHECK(count_err);
    ninfer::DeviceContext device(0);
    test_device_roundtrip(device);

    std::cout << (failures == 0 ? "OK" : "FAIL") << " kvarn continuation image\n";
    return failures == 0 ? 0 : 1;
}
