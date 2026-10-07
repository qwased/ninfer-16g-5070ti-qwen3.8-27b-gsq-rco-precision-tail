#pragma once

// Fixed native profile from Huawei KVarN commit 7586257f1c632e63187bfacbbe21ccb51540f7b3,
// vllm/model_executor/layers/quantization/kvarn/config.py: kvarn_k4v2_g128, D256.
// The packed widths are not fixed here: they are template parameters of the kernels and read
// from the cache view, so this file holds only the geometry and schedule constants.

namespace ninfer::ops::kvarn {

inline constexpr int D                 = 256;
inline constexpr int Group             = 128;
inline constexpr int Iterations        = 8;
inline constexpr int PrefillSlabTokens = 16384;
inline constexpr int MtpPackedWindow   = 1024;
inline constexpr int PackedQueryChunk  = 16;
inline constexpr int DecodeMidWindow   = 122880;
inline constexpr int DecodeMidSplits   = 41;
inline constexpr int DecodeLongSplits  = 82;

inline constexpr float StdMin = 1.0e-3F;
inline constexpr float StdMax = 1.0e3F;
inline constexpr float LogMin = -0.3F;
inline constexpr float LogMax = 10.0F;

} // namespace ninfer::ops::kvarn
