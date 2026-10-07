#pragma once

#include "core/tensor.h"

#include <cuda_runtime_api.h>

#include <cstdint>

namespace ninfer::ops {

// Huawei's released kvarn_k4v2_g128 profile at D256.
inline constexpr std::int32_t kKvarnHeadDim    = 256;
inline constexpr std::int32_t kKvarnGroup      = 128;
inline constexpr std::int32_t kKvarnIterations = 8;
inline constexpr std::int32_t kKvarnSinkPages  = 1;
inline constexpr std::int32_t kKvarnTailSlots  = 3;

// The record geometry and the bit-field codec are shared by host oracles and device kernels.
#if defined(__CUDACC__)
#define KVARN_HOST_DEVICE __host__ __device__
#else
#define KVARN_HOST_DEVICE
#endif

// ---------------------------------------------------------------------------
// Record geometry, parameterized by the packed widths (kb = K bits, vb = V
// bits). All metadata is FP16; D = 256, G = 128. `4096` is one packed
// code-plane per bit (D*G/8 codes * 1 bit / 8):
//
//   K packed          4096*kb          one G*kb/8-byte row per dim
//   K scale          + 512   [D]       K row scale
//   K zero           + 512   [D]       K row zero
//   K token scale    + 256   [G]       K token scale
//   V packed          4096*vb          one D*vb/8-byte row per token
//   V channel scale  + 512   [D]       V channel scale
//   V token scale    + 256   [G]       V token scale
//   V token zero     + 256   [G]       V token zero
//
// The release profiles are K == V == b for b in {4,5,6}, giving records of
// 35072 / 43264 / 51456 B. At kb == vb == 4 the K-side offsets and the K
// packing arithmetic are byte-identical to the legacy k4v2 profile below, so
// k4v4 changes the V side only.
// ---------------------------------------------------------------------------
inline constexpr std::int32_t kKvarnPackedBytesPerBit = (kKvarnHeadDim * kKvarnGroup) / 8; // 4096

KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_packed_offset(std::int32_t /*kb*/,
                                                                     std::int32_t /*vb*/) {
    return 0;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_scale_offset(std::int32_t kb,
                                                                     std::int32_t /*vb*/) {
    return kKvarnPackedBytesPerBit * kb;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_zero_offset(std::int32_t kb,
                                                                    std::int32_t vb) {
    return kvarn_k_scale_offset(kb, vb) + kKvarnHeadDim * 2;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_token_scale_offset(std::int32_t kb,
                                                                           std::int32_t vb) {
    return kvarn_k_zero_offset(kb, vb) + kKvarnHeadDim * 2;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_packed_offset(std::int32_t kb,
                                                                      std::int32_t vb) {
    return kvarn_k_token_scale_offset(kb, vb) + kKvarnGroup * 2;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_channel_scale_offset(std::int32_t kb,
                                                                             std::int32_t vb) {
    return kvarn_v_packed_offset(kb, vb) + kKvarnPackedBytesPerBit * vb;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_token_scale_offset(std::int32_t kb,
                                                                           std::int32_t vb) {
    return kvarn_v_channel_scale_offset(kb, vb) + kKvarnHeadDim * 2;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_token_zero_offset(std::int32_t kb,
                                                                          std::int32_t vb) {
    return kvarn_v_token_scale_offset(kb, vb) + kKvarnGroup * 2;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_record_bytes(std::int32_t kb,
                                                                   std::int32_t vb) {
    return kvarn_v_token_zero_offset(kb, vb) + kKvarnGroup * 2;
}

// Code addressing. K codes pack along tokens inside one dim row; V codes pack
// along dims inside one token row. `*_code_bit` is the bit offset inside the row.
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_row_bytes(std::int32_t kb) {
    return kKvarnGroup * kb / 8;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_k_code_bit(std::int32_t kb,
                                                                 std::int32_t token) {
    return token * kb;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_row_bytes(std::int32_t vb) {
    return kKvarnHeadDim * vb / 8;
}
KVARN_HOST_DEVICE inline constexpr std::int32_t kvarn_v_code_bit(std::int32_t vb,
                                                                 std::int32_t dim) {
    return dim * vb;
}

// The K side of the k4v4 profile is byte-identical to the legacy k4v2 (kb = 4) one, so
// `kvarn_k_row_bytes(4) == kKvarnGroup / 2` still pins the historic K packing.
static_assert(kvarn_k_row_bytes(4) == kKvarnGroup / 2);

// Release profiles: K == V == b for b in {4, 5, 6}. Every record is 256 B aligned, which the
// page geometry relies on (`record_bytes / kKvarnGroup` bytes per token per KV head).
static_assert(kvarn_record_bytes(4, 4) == 35072);
static_assert(kvarn_record_bytes(5, 5) == 43264);
static_assert(kvarn_record_bytes(6, 6) == 51456);
static_assert(kvarn_record_bytes(4, 4) % 256 == 0 && kvarn_record_bytes(5, 5) % 256 == 0 &&
              kvarn_record_bytes(6, 6) % 256 == 0);
static_assert(kvarn_k_row_bytes(4) == 64 && kvarn_k_row_bytes(5) == 80 && kvarn_k_row_bytes(6) == 96);
static_assert(kvarn_v_row_bytes(4) == 128 && kvarn_v_row_bytes(5) == 160 && kvarn_v_row_bytes(6) == 192);

// Little-endian bit-field codec, shared by host oracles and device kernels.
// `bit` is the offset of the code inside `row`; the second byte is touched only
// when the code straddles a byte boundary, so a row whose length is a multiple
// of 8 bits (every K/V row here) never reads past its end.
KVARN_HOST_DEVICE inline std::uint32_t kvarn_unpack_code(const std::uint8_t* row, std::int32_t bit,
                                                         std::int32_t bits) {
    const std::uint32_t low = row[bit >> 3];
    const std::uint32_t high = ((bit & 7) + bits > 8) ? row[(bit >> 3) + 1] : 0U;
    return ((low | (high << 8)) >> (bit & 7)) & ((1U << bits) - 1U);
}

// ORs the code in; the target bytes must be zeroed first (the store kernels do).
KVARN_HOST_DEVICE inline void kvarn_pack_code(std::uint8_t* row, std::int32_t bit,
                                              std::int32_t bits, std::uint32_t code) {
    const std::uint32_t masked = code & ((1U << bits) - 1U);
    row[bit >> 3] |= static_cast<std::uint8_t>(masked << (bit & 7));
    if ((bit & 7) + bits > 8) {
        row[(bit >> 3) + 1] |= static_cast<std::uint8_t>(masked >> (8 - (bit & 7)));
    }
}

struct KvarnTileStorage {
    Tensor k_codes;          // U8   [kvarn_k_row_bytes(bits),D,N]
    Tensor k_scales;         // FP16 [D,N]
    Tensor k_zeros;          // FP16 [D,N]
    Tensor k_token_scales;   // FP16 [G,N]
    Tensor v_codes;          // U8   [kvarn_v_row_bytes(bits),G,N]
    Tensor v_channel_scales; // FP16 [D,N]
    Tensor v_token_scales;   // FP16 [G,N]
    Tensor v_token_zeros;    // FP16 [G,N]
    // Packed width of both K and V (K == V). The release profiles are 4, 5 and 6.
    std::int32_t bits = 4;
};

struct KvarnTailStateView {
    Tensor k;             // BF16 [D,G,Hkv*tail_slots]
    Tensor v;             // BF16 [D,G,Hkv*tail_slots]
    Tensor logical_pages; // I32 [tail_slots]
    std::int32_t num_kv_heads = 0;
};

struct KvarnPagedLayerView {
    Tensor records;            // U8 [kvarn_record_bytes(bits,bits) / P,P,Hkv,Nphysical]
    Tensor tail_k;             // BF16 [D,P,Hkv*tail_slots]
    Tensor tail_v;             // BF16 [D,P,Hkv*tail_slots]
    Tensor tail_logical_pages; // I32 [tail_slots]
    Tensor block_table;        // I32 [Nlogical]
    std::int32_t num_kv_heads = 0;
    std::int32_t bits         = 4;
};

struct KvarnPagedBatchLayerView {
    Tensor records;            // U8 [kvarn_record_bytes(bits,bits) / P,P,Hkv,Nphysical]
    Tensor tail_k;             // BF16 [D,P,Hkv*tail_slots,C]
    Tensor tail_v;             // BF16 [D,P,Hkv*tail_slots,C]
    Tensor tail_logical_pages; // I32 [tail_slots,C]
    Tensor block_tables;       // I32 [Nlogical,C]
    std::int32_t num_kv_heads = 0;
    std::int32_t bits         = 4;
};

// Inputs are Hadamard-rotated contiguous BF16 [D,G,N] tiles. The represented decode is:
// K[d,t] = (code[d,t] * k_scales[d] + k_zeros[d]) * k_token_scales[t]
// V[d,t] = (code[d,t] * v_token_scales[t] + v_token_zeros[t]) * v_channel_scales[d]
void kvarn_store(const Tensor& rotated_k, const Tensor& rotated_v, KvarnTileStorage storage,
                 cudaStream_t stream);
void kvarn_dequant(const KvarnTileStorage& storage, Tensor& rotated_k, Tensor& rotated_v,
                   cudaStream_t stream);

// Orthonormal Sylvester-Hadamard transform over contiguous BF16 D256 vectors.
void kvarn_hadamard(const Tensor& source, Tensor& destination, cudaStream_t stream);

} // namespace ninfer::ops
