// KVarN per-execution-row marker reset: `PagedKVCache::reset_kvarn_tail_row(table_row, stream)`
// (models/qwen3_5/state/decoder_state.cpp) rewrites ONE execution row's KVarN slot markers back to
// the empty sentinel (`0xff`, i.e. int32 -1) in every layer of a `KvCacheStorage::KvarnGroup128`
// cache, and does nothing at all anywhere else. The constructor writes that sentinel for the whole
// tensor once (`decoder_state.cpp`), so the reset is what returns a single row to that state -- the
// CausalScore path is about to call it, and it had no test.
//
// The test is device-resident. It builds the cache the documented way -- DecoderStateSpec ->
// plan_decoder_state -> `DecoderStateLayout::text_kv` -> one device buffer + the public
// `PagedKVCache(DeviceSpan, const PagedKVCacheLayout&)` constructor -- seeds the markers through the
// public batch view (`kvarn_batch_layer_view(layer).tail_logical_pages`, an `I32 [tail_slots, rows]`
// tensor), resets one row and reads every marker back. Geometry is deliberately tiny: two layers,
// one KV head, D256 (the only head width both the exact-tail D256 profile and KVarN admit), one
// physical page group and three table rows.
#include "core/arena.h"
#include "core/device.h"
#include "core/layout.h"
#include "core/tensor.h"
#include "models/qwen3_5/state/decoder_state.h"
#include "ninfer/ops/kvarn.h"

#include <cuda_runtime.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
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

constexpr std::uint32_t kLayers     = 2;
constexpr std::int32_t kKvHeads     = 1;
constexpr std::int32_t kTableRows   = 3;
constexpr std::uint32_t kCapacity   = 64; // one logical page at 64- and at 128-token pages alike
constexpr std::uint32_t kPageGroups = 1;
// The one row the reset must rewrite; the other two must survive it byte for byte.
constexpr std::int32_t kTargetRow = 1;

constexpr std::size_t kSlotBytes = sizeof(std::int32_t);
// Markers are slot-major, so one row of `kKvarnTailSlots` int32 is contiguous.
constexpr std::size_t kRowBytes  = static_cast<std::size_t>(ops::kKvarnTailSlots) * kSlotBytes;

q36::DecoderStateSpec make_spec(ninfer::KvCacheStorage storage) {
    q36::DecoderStateSpec spec;
    spec.full_attention_layers     = kLayers;
    spec.capacity                  = kCapacity;
    spec.kv_heads                  = kKvHeads;
    spec.attention_head_dim        = ops::kKvarnHeadDim; // 256
    spec.kv_storage                = storage;
    spec.kv_table_rows             = kTableRows;
    spec.text_physical_page_groups = kPageGroups;
    spec.enable_mtp                = false;
    spec.kv_tail_tokens            = 0;
    return spec;
}

std::size_t row_offset(std::int32_t row) {
    return static_cast<std::size_t>(row) * kRowBytes;
}

// Distinct, small, positive values: no byte of any of them is 0xff, so a 0xff byte seen after the
// reset can only be the sentinel the reset wrote.
std::int32_t seed_value(std::uint32_t layer, std::int32_t row, std::int32_t slot) {
    return static_cast<std::int32_t>(layer) * 1000 + row * 100 + slot + 1;
}

// One layer's marker block, host side, laid out exactly as the device view is: marker (slot `s`,
// row `r`) at `r * kRowBytes + s * 4`.
std::vector<std::int32_t> seed_values(std::uint32_t layer) {
    std::vector<std::int32_t> values(static_cast<std::size_t>(kTableRows) *
                                     static_cast<std::size_t>(ops::kKvarnTailSlots));
    for (std::int32_t row = 0; row < kTableRows; ++row) {
        for (std::int32_t slot = 0; slot < ops::kKvarnTailSlots; ++slot) {
            values[static_cast<std::size_t>(row) * static_cast<std::size_t>(ops::kKvarnTailSlots) +
                   static_cast<std::size_t>(slot)] = seed_value(layer, row, slot);
        }
    }
    return values;
}

// The block the test expects on the device: the seed everywhere, except `reset_row`, whose markers
// are the empty sentinel (0xff in every byte of the int32).
std::vector<std::uint8_t> expected_block(std::uint32_t layer,
                                         std::optional<std::int32_t> reset_row) {
    const std::vector<std::int32_t> seed = seed_values(layer);
    std::vector<std::uint8_t> bytes(seed.size() * kSlotBytes);
    std::memcpy(bytes.data(), seed.data(), bytes.size());
    if (reset_row) {
        const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(row_offset(*reset_row));
        std::fill(begin, begin + static_cast<std::ptrdiff_t>(kRowBytes),
                  static_cast<std::uint8_t>(0xffU));
    }
    return bytes;
}

std::vector<std::uint8_t> row_bytes(const std::vector<std::uint8_t>& block, std::int32_t row) {
    const std::size_t begin = row_offset(row);
    return std::vector<std::uint8_t>(block.begin() + static_cast<std::ptrdiff_t>(begin),
                                     block.begin() + static_cast<std::ptrdiff_t>(begin + kRowBytes));
}

std::vector<std::uint8_t> all_ff_row() { return std::vector<std::uint8_t>(kRowBytes, 0xffU); }

std::vector<std::uint8_t> read_bytes(const void* device_data, std::size_t bytes,
                                     cudaStream_t stream) {
    std::vector<std::uint8_t> host(bytes);
    CUDA_CHECK(cudaMemcpyAsync(host.data(), device_data, bytes, cudaMemcpyDeviceToHost, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));
    return host;
}

bool any_ff(const std::vector<std::uint8_t>& bytes) {
    return std::any_of(bytes.begin(), bytes.end(), [](std::uint8_t byte) { return byte == 0xffU; });
}

// The batch marker view of one layer: its geometry is what every index in this test rests on, so it
// is pinned before any marker is trusted.
ninfer::Tensor layer_markers(const q36::PagedKVCache& cache, std::uint32_t layer) {
    const ninfer::Tensor markers = cache.kvarn_batch_layer_view(layer).tail_logical_pages;
    expect(markers.ne[0] == ops::kKvarnTailSlots && markers.ne[1] == kTableRows,
           "the batch marker view is I32 [kKvarnTailSlots, table_rows]");
    expect(markers.nb[0] == static_cast<std::int64_t>(kSlotBytes) &&
               markers.nb[1] == static_cast<std::int64_t>(kRowBytes),
           "the batch marker view has one contiguous marker block per row");
    expect(markers.bytes() == kRowBytes * static_cast<std::size_t>(kTableRows),
           "the batch marker view spans exactly one layer's markers");
    return markers;
}

void test_kvarn_tail_row_reset(ninfer::DeviceContext& device) {
    ninfer::LayoutBuilder builder;
    const q36::DecoderStateLayout layout =
        q36::plan_decoder_state(builder, make_spec(ninfer::KvCacheStorage::KvarnGroup128));
    const std::size_t bytes = builder.finish(256);
    ninfer::DeviceArena arena(bytes);
    const q36::PagedKVCache cache({arena.base(), arena.capacity()}, layout.text_kv);
    // The KVarN constructor retires the whole marker tensor with `cudaMemset` (the legacy default
    // stream), while `device.stream` is a non-blocking stream, so the two have no implicit order.
    // The seed below overwrites every marker anyway; this makes the seed order explicit rather than
    // assumed, so a reordered constructor memset cannot turn into a spurious failure.
    CUDA_CHECK(cudaDeviceSynchronize());

    expect(cache.storage() == ninfer::KvCacheStorage::KvarnGroup128,
           "the planned text cache is a KVarN cache");
    expect(cache.layers() == kLayers, "the text cache covers every full-attention layer");

    // Seed every layer's markers through the public batch view.
    for (std::uint32_t layer = 0; layer < kLayers; ++layer) {
        const ninfer::Tensor markers     = layer_markers(cache, layer);
        const std::vector<std::int32_t> seed = seed_values(layer);
        expect(markers.bytes() == seed.size() * kSlotBytes,
               "the seeded host image is exactly one layer's marker block");
        CUDA_CHECK(cudaMemcpyAsync(markers.data, seed.data(), markers.bytes(),
                                   cudaMemcpyHostToDevice, device.stream));
        CUDA_CHECK(cudaStreamSynchronize(device.stream));
    }

    // Pre-reset state. Capturing it also proves the seed carries no sentinel byte, so every 0xff
    // observed below is the reset's.
    std::vector<std::vector<std::uint8_t>> before(kLayers);
    for (std::uint32_t layer = 0; layer < kLayers; ++layer) {
        const ninfer::Tensor markers = layer_markers(cache, layer);
        before[layer]                = read_bytes(markers.data, markers.bytes(), device.stream);
        expect(!any_ff(before[layer]),
               "layer " + std::to_string(layer) + ": the seeded markers carry no sentinel byte");
        expect(before[layer] == expected_block(layer, std::nullopt),
               "layer " + std::to_string(layer) +
                   ": every seeded marker is readable through the public batch view");
    }

    cache.reset_kvarn_tail_row(kTargetRow, device.stream);
    CUDA_CHECK(cudaStreamSynchronize(device.stream));

    for (std::uint32_t layer = 0; layer < kLayers; ++layer) {
        const ninfer::Tensor markers         = layer_markers(cache, layer);
        const std::vector<std::uint8_t> after = read_bytes(markers.data, markers.bytes(),
                                                           device.stream);
        expect(row_bytes(after, kTargetRow) == all_ff_row(),
               "layer " + std::to_string(layer) +
                   ": the reset row's markers are the 0xff empty-slot sentinel");
        for (std::int32_t row = 0; row < kTableRows; ++row) {
            if (row == kTargetRow) { continue; }
            expect(row_bytes(after, row) == row_bytes(before[layer], row),
                   "layer " + std::to_string(layer) + ", row " + std::to_string(row) +
                       ": another row's markers are unchanged by the reset");
        }
        expect(after == expected_block(layer, kTargetRow),
               "layer " + std::to_string(layer) +
                   ": the reset rewrites exactly one row and disturbs no other marker");
    }

    // Out-of-range rows are refused: the cache's row count is the batch view's second extent, so
    // `-1` and `kTableRows` leave every marker exactly as it was.
    cache.reset_kvarn_tail_row(-1, device.stream);
    cache.reset_kvarn_tail_row(kTableRows, device.stream);
    CUDA_CHECK(cudaStreamSynchronize(device.stream));
    for (std::uint32_t layer = 0; layer < kLayers; ++layer) {
        const ninfer::Tensor markers = layer_markers(cache, layer);
        expect(read_bytes(markers.data, markers.bytes(), device.stream) ==
                   expected_block(layer, kTargetRow),
               "layer " + std::to_string(layer) +
                   ": an out-of-range row leaves every marker untouched");
    }
}

// A second cache over a second buffer with a non-KVarN storage: there is no marker block at all, so
// the call must neither throw nor write a byte of its device memory.
void test_non_kvarn_reset_is_a_no_op(ninfer::DeviceContext& device) {
    ninfer::LayoutBuilder builder;
    const q36::DecoderStateLayout layout =
        q36::plan_decoder_state(builder, make_spec(ninfer::KvCacheStorage::BFloat16));
    const std::size_t bytes = builder.finish(256);
    ninfer::DeviceArena arena(bytes);
    const q36::PagedKVCache cache({arena.base(), arena.capacity()}, layout.text_kv);
    // Constructor writes (if any go to the legacy default stream) must land before the snapshot;
    // `device.stream` is non-blocking, so it does not order itself against them.
    CUDA_CHECK(cudaDeviceSynchronize());

    expect(cache.storage() == ninfer::KvCacheStorage::BFloat16,
           "the second cache is a plain BF16 cache");

    const std::vector<std::uint8_t> before =
        read_bytes(arena.base(), arena.capacity(), device.stream);

    // In range for the row count a KVarN cache of this shape would have, negative, and one past the
    // end.
    bool no_op_failed = false;
    try {
        cache.reset_kvarn_tail_row(0, device.stream);
        cache.reset_kvarn_tail_row(kTargetRow, device.stream);
        cache.reset_kvarn_tail_row(kTableRows - 1, device.stream);
        cache.reset_kvarn_tail_row(-1, device.stream);
        cache.reset_kvarn_tail_row(kTableRows, device.stream);
    } catch (const std::exception& error) {
        no_op_failed = true;
        ++failures;
        std::cerr << "FAIL: a non-KVarN cache's reset must be a no-op, but it threw: "
                  << error.what() << '\n';
    }
    if (!no_op_failed) {
        CUDA_CHECK(cudaStreamSynchronize(device.stream));
        expect(read_bytes(arena.base(), arena.capacity(), device.stream) == before,
               "a non-KVarN cache's device memory is untouched by the reset");
    }
}

} // namespace

int main() {
    int count                   = 0;
    const cudaError_t count_err = cudaGetDeviceCount(&count);
    if (cuda_unavailable(count_err) || count == 0) {
        std::cout << "SKIP: no usable CUDA device (the reset contract needs one)\n";
        return failures == 0 ? 0 : 1;
    }
    CUDA_CHECK(count_err);
    ninfer::DeviceContext device(0);
    test_kvarn_tail_row_reset(device);
    test_non_kvarn_reset_is_a_no_op(device);

    std::cout << (failures == 0 ? "OK" : "FAIL") << " kvarn tail row reset\n";
    return failures == 0 ? 0 : 1;
}
