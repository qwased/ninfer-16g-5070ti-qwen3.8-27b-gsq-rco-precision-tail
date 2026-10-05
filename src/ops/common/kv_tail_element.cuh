#pragma once

// ninfer::ops - element trait of the exact KV tail ring (WP3, F16 tail).
//
// The tail write and read kernels run the same code for a BF16 and an F16 ring; only the scalar
// conversions and the tensor-core op differ. The BF16 specialization is deliberately an exact
// no-op -- the query/K/V source rows are BF16 and their bits are copied verbatim, and it dispatches
// to mma_bf16 -- so its generated code stays byte-identical to the pre-F16 kernel. F16 converts the
// BF16 source to float and back to half on the way in (never reinterpreting BF16 bits as F16) and
// accumulates QK^T / PV with mma_f16.

#include "ops/common/mma.cuh"

#include "core/dtype.h"

#include <cuda_bf16.h>
#include <cuda_fp16.h>

#include <type_traits>

namespace ninfer::ops {

template <typename Elem>
struct KvTailElement;

template <>
struct KvTailElement<__nv_bfloat16> {
    // BF16 keeps the source bits verbatim.
    __device__ static __nv_bfloat16 from_source(__nv_bfloat16 value) { return value; }
    __device__ static __nv_bfloat16 from_float(float value) { return __float2bfloat16(value); }
    __device__ static __nv_bfloat16 zero() { return __float2bfloat16(0.0f); }
    __device__ static void mma(float& c0, float& c1, float& c2, float& c3, unsigned a0, unsigned a1,
                               unsigned a2, unsigned a3, unsigned b0, unsigned b1) {
        mma_bf16(c0, c1, c2, c3, a0, a1, a2, a3, b0, b1);
    }
};

template <>
struct KvTailElement<__half> {
    __device__ static __half from_source(__nv_bfloat16 value) {
        return __float2half(__bfloat162float(value));
    }
    __device__ static __half from_float(float value) { return __float2half(value); }
    __device__ static __half zero() { return __float2half(0.0f); }
    __device__ static void mma(float& c0, float& c1, float& c2, float& c3, unsigned a0, unsigned a1,
                               unsigned a2, unsigned a3, unsigned b0, unsigned b1) {
        mma_f16(c0, c1, c2, c3, a0, a1, a2, a3, b0, b1);
    }
};

// Copies eight BF16 source elements into the tail ring as `Dst`. BF16 is a raw 16-byte vector copy;
// F16 converts each element BF16 -> float -> half.
template <typename Dst>
__device__ __forceinline__ void store_tail_vec8(Dst* dst, const __nv_bfloat16* src) {
    if constexpr (std::is_same_v<Dst, __nv_bfloat16>) {
        *reinterpret_cast<int4*>(dst) = *reinterpret_cast<const int4*>(src);
    } else {
        alignas(16) __half converted[8];
#pragma unroll
        for (int i = 0; i < 8; ++i) { converted[i] = __float2half(__bfloat162float(src[i])); }
        *reinterpret_cast<int4*>(dst) = *reinterpret_cast<const int4*>(converted);
    }
}

// Host-side dispatch on a tail tensor's element dtype: F16 instantiates the __half kernels, every
// other (BF16) value the __nv_bfloat16 ones.
template <typename Fn>
void with_kv_tail_element(DType dtype, Fn&& fn) {
    if (dtype == DType::FP16) {
        fn.template operator()<__half>();
    } else {
        fn.template operator()<__nv_bfloat16>();
    }
}

} // namespace ninfer::ops
