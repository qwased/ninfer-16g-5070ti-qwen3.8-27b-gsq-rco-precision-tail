#include "ninfer/ops/kvarn.h"
#include "ninfer/ops/kvarn_attention.h"
#include "ops/op_tester.h"
#include "ops/kvarn/config.cuh"
#include "ops/kvarn/decode.cuh"
#include "ops/kvarn/tail_partial.h"
#include "ops/softmax_attention/dense/causal_cache/geometry.cuh"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace ninfer;
using namespace ninfer::test;

namespace {

constexpr int kD            = ops::kKvarnHeadDim;
constexpr int kGroup        = ops::kKvarnGroup;
constexpr int kTiles        = 2;
constexpr int kTileElements = kD * kGroup;
constexpr int kBits        = 4;              // the suite runs on the k4v4 release profile
constexpr int kRecordBytes = ops::kvarn_record_bytes(kBits, kBits);

std::uint16_t f32_to_f16(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    const std::uint32_t sign = (bits >> 16) & 0x8000U;
    std::uint32_t mantissa   = bits & 0x007fffffU;
    int exponent             = static_cast<int>((bits >> 23) & 0xffU) - 127;
    if (((bits >> 23) & 0xffU) == 0xffU) {
        return static_cast<std::uint16_t>(sign | (mantissa == 0 ? 0x7c00U : 0x7e00U));
    }
    if (exponent > 15) return static_cast<std::uint16_t>(sign | 0x7c00U);
    if (exponent >= -14) {
        std::uint32_t half_exponent = static_cast<std::uint32_t>(exponent + 15);
        std::uint32_t rounded       = mantissa + 0x00000fffU + ((mantissa >> 13) & 1U);
        if ((rounded & 0x00800000U) != 0) {
            rounded = 0;
            if (++half_exponent >= 31) return static_cast<std::uint16_t>(sign | 0x7c00U);
        }
        return static_cast<std::uint16_t>(sign | (half_exponent << 10) | (rounded >> 13));
    }
    if (exponent < -24) return static_cast<std::uint16_t>(sign);
    mantissa |= 0x00800000U;
    const int shift             = -exponent - 14;
    std::uint32_t half_mantissa = mantissa >> (shift + 13);
    const std::uint32_t remain  = mantissa & ((1U << (shift + 13)) - 1U);
    const std::uint32_t halfway = 1U << (shift + 12);
    if (remain > halfway || (remain == halfway && (half_mantissa & 1U) != 0)) { ++half_mantissa; }
    return static_cast<std::uint16_t>(sign | half_mantissa);
}

float f16_to_f32(std::uint16_t value) {
    const std::uint32_t sign = static_cast<std::uint32_t>(value & 0x8000U) << 16;
    const std::uint32_t exp  = (value >> 10) & 0x1fU;
    std::uint32_t mantissa   = value & 0x03ffU;
    std::uint32_t bits;
    if (exp == 0) {
        if (mantissa == 0) {
            bits = sign;
        } else {
            int unbiased = -14;
            while ((mantissa & 0x0400U) == 0) {
                mantissa <<= 1;
                --unbiased;
            }
            bits = sign | (static_cast<std::uint32_t>(unbiased + 127) << 23) |
                   ((mantissa & 0x03ffU) << 13);
        }
    } else if (exp == 31) {
        bits = sign | 0x7f800000U | (mantissa << 13);
    } else {
        bits = sign | ((exp + 112U) << 23) | (mantissa << 13);
    }
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

double sample_std(const std::vector<double>& matrix, int rows, int cols, bool column, int index) {
    const int count    = column ? rows : cols;
    double sum         = 0.0;
    double sum_squared = 0.0;
    for (int item = 0; item < count; ++item) {
        const int row      = column ? item : index;
        const int col      = column ? index : item;
        const double value = matrix[static_cast<std::size_t>(row) * cols + col];
        sum += value;
        sum_squared += value * value;
    }
    const double variance = (sum_squared - sum * sum / count) / (count - 1);
    return std::sqrt(std::max(variance, 0.0));
}

double imbalance(const std::vector<double>& matrix, int rows, int cols) {
    double row_min = std::numeric_limits<double>::infinity();
    double row_max = 0.0;
    double col_min = std::numeric_limits<double>::infinity();
    double col_max = 0.0;
    for (int row = 0; row < rows; ++row) {
        const double deviation = sample_std(matrix, rows, cols, false, row);
        row_min                = std::min(row_min, deviation);
        row_max                = std::max(row_max, deviation);
    }
    for (int col = 0; col < cols; ++col) {
        const double deviation = sample_std(matrix, rows, cols, true, col);
        col_min                = std::min(col_min, deviation);
        col_max                = std::max(col_max, deviation);
    }
    return row_max / std::max(row_min, 1.0e-8) + col_max / std::max(col_min, 1.0e-8);
}

struct Balanced {
    std::vector<double> values;
    std::vector<double> column_scale;
    std::vector<double> row_scale;
};

Balanced balance(const std::vector<double>& input, int rows, int cols) {
    std::vector<double> log_column(cols, 0.0);
    std::vector<double> log_row(rows, 0.0);
    std::vector<double> best_column(log_column);
    std::vector<double> best_row(log_row);
    std::vector<double> current(input);
    double best = imbalance(current, rows, cols);

    const auto rebuild = [&] {
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                current[static_cast<std::size_t>(row) * cols + col] =
                    input[static_cast<std::size_t>(row) * cols + col] /
                    std::exp(log_row[row] + log_column[col]);
            }
        }
    };
    for (int iteration = 0; iteration < ops::kKvarnIterations; ++iteration) {
        for (int col = 0; col < cols; ++col) {
            const double deviation =
                std::clamp(sample_std(current, rows, cols, true, col), 1.0e-3, 1.0e3);
            log_column[col] = std::clamp(log_column[col] + std::log(deviation), -0.3, 10.0);
        }
        rebuild();
        for (int row = 0; row < rows; ++row) {
            const double deviation =
                std::clamp(sample_std(current, rows, cols, false, row), 1.0e-3, 1.0e3);
            log_row[row] = std::clamp(log_row[row] + std::log(deviation), -0.3, 10.0);
        }
        rebuild();
        const double candidate = imbalance(current, rows, cols);
        if (candidate <= best) {
            best        = candidate;
            best_column = log_column;
            best_row    = log_row;
        }
    }

    Balanced result{input, std::vector<double>(cols), std::vector<double>(rows)};
    for (int col = 0; col < cols; ++col) result.column_scale[col] = std::exp(best_column[col]);
    for (int row = 0; row < rows; ++row) result.row_scale[row] = std::exp(best_row[row]);
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            result.values[static_cast<std::size_t>(row) * cols + col] /=
                result.row_scale[row] * result.column_scale[col];
        }
    }
    return result;
}

std::vector<float> make_input(std::uint32_t seed, bool value) {
    std::mt19937 generator(seed);
    std::normal_distribution<float> distribution(0.0F, 1.0F);
    std::vector<float> result(static_cast<std::size_t>(kTileElements) * kTiles);
    for (int tile = 0; tile < kTiles; ++tile) {
        for (int token = 0; token < kGroup; ++token) {
            const float token_scale =
                std::exp((static_cast<float>((token * 17 + 5 * tile) % 23) - 11.0F) * 0.055F);
            for (int d = 0; d < kD; ++d) {
                const float channel_scale =
                    std::exp((static_cast<float>((d * 13 + 7 * tile) % 31) - 15.0F) * 0.045F);
                float x = distribution(generator) * token_scale * channel_scale;
                if ((d + 19 * token + 11 * tile) % 509 == 0) { x *= value ? 4.0F : 6.0F; }
                const std::size_t index = static_cast<std::size_t>(tile) * kTileElements +
                                          static_cast<std::size_t>(token) * kD + d;
                result[index] = bf16_to_f32(f32_to_bf16(x));
            }
        }
    }
    return result;
}

// The independent FP64 oracle for one KVarN tile. `decoded` is the represented value
// `(code * scale + zero) * factor`; `step` is the theoretical quantization step
// `(max - min) / qmax * row_scale * column_scale` at `qmax = (1 << bits) - 1`, i.e. the
// value distance between two adjacent codes; `code` is the integer the oracle selected in
// the balanced domain.
struct CodecOracle {
    std::vector<double> decoded;
    std::vector<double> step;
    std::vector<std::uint32_t> code;
};

CodecOracle codec_oracle(const std::vector<float>& input, bool key, int bits) {
    CodecOracle result{std::vector<double>(input.size()), std::vector<double>(input.size()),
                       std::vector<std::uint32_t>(input.size(), 0)};
    for (int tile = 0; tile < kTiles; ++tile) {
        const std::size_t base = static_cast<std::size_t>(tile) * kTileElements;
        const int rows         = key ? kD : kGroup;
        const int cols         = key ? kGroup : kD;
        std::vector<double> matrix(kTileElements);
        for (int token = 0; token < kGroup; ++token) {
            for (int d = 0; d < kD; ++d) {
                const int row                                      = key ? d : token;
                const int col                                      = key ? token : d;
                matrix[static_cast<std::size_t>(row) * cols + col] = input[base + token * kD + d];
            }
        }
        const Balanced balanced = balance(matrix, rows, cols);
        const int qmax          = (1 << bits) - 1;
        for (int row = 0; row < rows; ++row) {
            const auto first = balanced.values.begin() + static_cast<std::ptrdiff_t>(row) * cols;
            const auto last  = first + cols;
            const auto [minimum_it, maximum_it] = std::minmax_element(first, last);
            const double minimum                = *minimum_it;
            const double scale                  = std::max((*maximum_it - minimum) / qmax, 1.0e-10);
            const float absorbed_scale =
                f16_to_f32(f32_to_f16(static_cast<float>(balanced.row_scale[row] * scale)));
            const float absorbed_zero =
                f16_to_f32(f32_to_f16(static_cast<float>(balanced.row_scale[row] * minimum)));
            for (int col = 0; col < cols; ++col) {
                const int code = std::clamp(
                    static_cast<int>(std::nearbyint(
                        (balanced.values[static_cast<std::size_t>(row) * cols + col] - minimum) /
                        scale)),
                    0, qmax);
                const float other_scale =
                    f16_to_f32(f32_to_f16(static_cast<float>(balanced.column_scale[col])));
                const float decoded =
                    std::fma(static_cast<float>(code), absorbed_scale, absorbed_zero) * other_scale;
                const int token                 = key ? col : row;
                const int d                     = key ? row : col;
                const std::size_t index         = base + static_cast<std::size_t>(token) * kD + d;
                result.decoded[index]           = decoded;
                result.step[index]              = static_cast<double>(absorbed_scale) * other_scale;
                result.code[index]              = static_cast<std::uint32_t>(code);
            }
        }
    }
    return result;
}

template <int Bits>
struct DeviceStorage {
    DeviceBuffer k_codes{static_cast<std::size_t>(ops::kvarn_k_row_bytes(Bits)) * kD * kTiles};
    DeviceBuffer k_scales{static_cast<std::size_t>(kD) * kTiles * sizeof(std::uint16_t)};
    DeviceBuffer k_zeros{static_cast<std::size_t>(kD) * kTiles * sizeof(std::uint16_t)};
    DeviceBuffer k_token_scales{static_cast<std::size_t>(kGroup) * kTiles * sizeof(std::uint16_t)};
    DeviceBuffer v_codes{static_cast<std::size_t>(ops::kvarn_v_row_bytes(Bits)) * kGroup * kTiles};
    DeviceBuffer v_channel_scales{static_cast<std::size_t>(kD) * kTiles * sizeof(std::uint16_t)};
    DeviceBuffer v_token_scales{static_cast<std::size_t>(kGroup) * kTiles * sizeof(std::uint16_t)};
    DeviceBuffer v_token_zeros{static_cast<std::size_t>(kGroup) * kTiles * sizeof(std::uint16_t)};

    ops::KvarnTileStorage view() {
        return {
            Tensor(k_codes.p, DType::U8, {ops::kvarn_k_row_bytes(Bits), kD, kTiles}),
            Tensor(k_scales.p, DType::FP16, {kD, kTiles}),
            Tensor(k_zeros.p, DType::FP16, {kD, kTiles}),
            Tensor(k_token_scales.p, DType::FP16, {kGroup, kTiles}),
            Tensor(v_codes.p, DType::U8, {ops::kvarn_v_row_bytes(Bits), kGroup, kTiles}),
            Tensor(v_channel_scales.p, DType::FP16, {kD, kTiles}),
            Tensor(v_token_scales.p, DType::FP16, {kGroup, kTiles}),
            Tensor(v_token_zeros.p, DType::FP16, {kGroup, kTiles}),
            Bits,
        };
    }
};

int compare_profile(const char* label, const std::vector<double>& actual,
                    const std::vector<double>& expected, double limit, bool report = false) {
    double error_squared     = 0.0;
    double reference_squared = 0.0;
    double maximum           = 0.0;
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (!std::isfinite(actual[index]) || !std::isfinite(expected[index])) {
            std::cerr << label << ": non-finite value at " << index << '\n';
            return 1;
        }
        const double error = std::abs(actual[index] - expected[index]);
        error_squared += error * error;
        reference_squared += expected[index] * expected[index];
        maximum = std::max(maximum, error);
    }
    const double relative_l2 = std::sqrt(error_squared / std::max(reference_squared, 1.0e-30));
    if (report) {
        std::cout << label << ": relative_l2=" << relative_l2 << " max_abs=" << maximum << '\n';
    }
    if (relative_l2 > limit) {
        std::cerr << label << ": relative_l2=" << relative_l2 << " limit=" << limit
                  << " max_abs=" << maximum << '\n';
        return 1;
    }
    return 0;
}

// WP5: the KVarN codec tolerance is a quantization-step criterion, not a fitted relative-L2 bound.
// `qmax = (1 << bits) - 1` fixes the theoretical step q = (max - min) / qmax * row_scale *
// column_scale, the value distance between two adjacent codes. The device kernels and this FP64
// oracle run independent Sinkhorn passes, so an element sitting on a rounding boundary can resolve
// to the adjacent code and move by exactly one q. The criterion therefore has the two parts that
// op-development.md 6.3 requires of a floating-point criterion -- a pointwise bound plus a finite
// gross pointwise-error cap:
//   1. pointwise: |actual - expected| <= q * (1 + kStepSlack) for every element. The slack absorbs
//      the small difference between the two independently computed row/column scale factors; it is
//      far below the size of a real codec defect, which moves elements by whole steps.
//   2. gross cap: the number of elements whose code differs from the oracle's stays a tiny fraction
//      of the tile. Wrong field width, wrong bit order or biased rounding moves essentially every
//      element, so a systematic defect cannot hide behind the pointwise bound.
constexpr double kStepSlack       = 5.0e-2;
constexpr double kMaxFlipFraction = 1.0e-3;

struct StepCriterion {
    std::size_t total      = 0;
    std::size_t flips      = 0;  // elements whose code differs from the oracle's
    std::size_t wide_flips = 0;  // code differences beyond one neighbouring code
    std::size_t over_step  = 0;  // pointwise deviations beyond q * (1 + kStepSlack)
    double worst_ratio     = 0.0; // max |actual - expected| / q over the tile
};

int check_step_criterion(const char* label, const std::vector<double>& actual,
                         const std::vector<double>& expected, const std::vector<double>& step,
                         const std::vector<std::uint32_t>& actual_code,
                         const std::vector<std::uint32_t>& expected_code) {
    StepCriterion stats;
    stats.total = actual.size();
    for (std::size_t index = 0; index < actual.size(); ++index) {
        if (!std::isfinite(actual[index]) || !std::isfinite(expected[index])) {
            std::cerr << label << ": non-finite value at " << index << '\n';
            return 1;
        }
        const double q    = step[index];
        const double ratio = std::abs(actual[index] - expected[index]) / std::max(q, 1.0e-30);
        stats.worst_ratio  = std::max(stats.worst_ratio, ratio);
        if (ratio > 1.0 + kStepSlack) { ++stats.over_step; }
        if (actual_code[index] != expected_code[index]) {
            ++stats.flips;
            const std::uint32_t gap = actual_code[index] > expected_code[index]
                                          ? actual_code[index] - expected_code[index]
                                          : expected_code[index] - actual_code[index];
            if (gap > 1) { ++stats.wide_flips; }
        }
    }
    const std::size_t flip_cap = static_cast<std::size_t>(kMaxFlipFraction * stats.total);
    std::cout << label << ": step_ratio_max=" << stats.worst_ratio << " flips=" << stats.flips
              << "/" << flip_cap << " over_step=" << stats.over_step
              << " wide_flips=" << stats.wide_flips << " of " << stats.total << '\n';
    int failures = 0;
    if (stats.over_step != 0) {
        std::cerr << label << ": " << stats.over_step << " elements exceed one quantization step (max "
                  << stats.worst_ratio << " steps)\n";
        ++failures;
    }
    if (stats.wide_flips != 0) {
        std::cerr << label << ": " << stats.wide_flips << " codes differ by more than one step\n";
        ++failures;
    }
    if (stats.flips > flip_cap) {
        std::cerr << label << ": " << stats.flips << " boundary flips exceed the cap " << flip_cap
                  << '\n';
        ++failures;
    }
    return failures;
}

template <int Bits>
int run_codec_case() {
    const std::vector<float> key             = make_input(0x4b41524eU, false);
    const std::vector<float> value           = make_input(0x56324736U, true);
    const CodecOracle expected_key           = codec_oracle(key, true, Bits);
    const CodecOracle expected_value         = codec_oracle(value, false, Bits);
    DeviceBuffer device_key                  = to_device_bf16(key);
    DeviceBuffer device_value                = to_device_bf16(value);
    DeviceStorage<Bits> storage;
    DeviceBuffer decoded_key(key.size() * sizeof(float));
    DeviceBuffer decoded_value(value.size() * sizeof(float));

    Tensor key_tensor(device_key.p, DType::BF16, {kD, kGroup, kTiles});
    Tensor value_tensor(device_value.p, DType::BF16, {kD, kGroup, kTiles});
    ops::KvarnTileStorage storage_view = storage.view();
    ops::kvarn_store(key_tensor, value_tensor, storage_view, nullptr);
    Tensor decoded_key_tensor(decoded_key.p, DType::FP32, {kD, kGroup, kTiles});
    Tensor decoded_value_tensor(decoded_value.p, DType::FP32, {kD, kGroup, kTiles});
    ops::kvarn_dequant(storage_view, decoded_key_tensor, decoded_value_tensor, nullptr);
    cuda_synchronize();

    const std::vector<double> actual_key   = from_device_f32(decoded_key, key.size());
    const std::vector<double> actual_value = from_device_f32(decoded_value, value.size());
    int failures                           = 0;

    const auto k_codes    = from_device<std::uint8_t>(storage.k_codes, storage.k_codes.bytes);
    const auto k_scales   = from_device<std::uint16_t>(storage.k_scales, kD * kTiles);
    const auto k_zeros    = from_device<std::uint16_t>(storage.k_zeros, kD * kTiles);
    const auto k_tokens   = from_device<std::uint16_t>(storage.k_token_scales, kGroup * kTiles);
    const auto v_codes    = from_device<std::uint8_t>(storage.v_codes, storage.v_codes.bytes);
    const auto v_channels = from_device<std::uint16_t>(storage.v_channel_scales, kD * kTiles);
    const auto v_scales   = from_device<std::uint16_t>(storage.v_token_scales, kGroup * kTiles);
    const auto v_zeros    = from_device<std::uint16_t>(storage.v_token_zeros, kGroup * kTiles);
    std::vector<double> represented_key(key.size());
    std::vector<double> represented_value(value.size());
    std::vector<std::uint32_t> device_key_codes(key.size(), 0);
    std::vector<std::uint32_t> device_value_codes(value.size(), 0);
    for (int tile = 0; tile < kTiles; ++tile) {
        for (int token = 0; token < kGroup; ++token) {
            for (int d = 0; d < kD; ++d) {
                const std::size_t output = static_cast<std::size_t>(tile) * kTileElements +
                                           static_cast<std::size_t>(token) * kD + d;
                const std::uint32_t kc = ops::kvarn_unpack_code(
                    &k_codes[(static_cast<std::size_t>(tile) * kD + d) *
                             ops::kvarn_k_row_bytes(Bits)],
                    token * Bits, Bits);
                device_key_codes[output] = kc;
                represented_key[output] =
                    std::fma(static_cast<float>(kc), f16_to_f32(k_scales[tile * kD + d]),
                             f16_to_f32(k_zeros[tile * kD + d])) *
                    f16_to_f32(k_tokens[tile * kGroup + token]);
                const std::uint32_t vc = ops::kvarn_unpack_code(
                    &v_codes[(static_cast<std::size_t>(tile) * kGroup + token) *
                             ops::kvarn_v_row_bytes(Bits)],
                    d * Bits, Bits);
                device_value_codes[output] = vc;
                represented_value[output] =
                    std::fma(static_cast<float>(vc), f16_to_f32(v_scales[tile * kGroup + token]),
                             f16_to_f32(v_zeros[tile * kGroup + token])) *
                    f16_to_f32(v_channels[tile * kD + d]);
            }
        }
    }
    failures += compare_profile("KVarN K stored-bit decode", actual_key, represented_key, 2.0e-7);
    failures +=
        compare_profile("KVarN V stored-bit decode", actual_value, represented_value, 2.0e-7);
    failures += check_step_criterion("KVarN K official oracle", actual_key, expected_key.decoded,
                                     expected_key.step, device_key_codes, expected_key.code);
    failures += check_step_criterion("KVarN V official oracle", actual_value, expected_value.decoded,
                                     expected_value.step, device_value_codes, expected_value.code);

    const auto check_roundtrip = [&](const char* label, const std::vector<std::uint8_t>& stored,
                                     std::vector<std::uint8_t>& destination) {
        for (std::size_t index = 0; index < stored.size(); ++index) {
            if (destination[index] != stored[index]) {
                std::cerr << label << ": mismatch at index " << index
                          << " stored=" << static_cast<int>(stored[index])
                          << " roundtrip=" << static_cast<int>(destination[index]) << '\n';
                return 1;
            }
        }
        return 0;
    };

    const std::size_t k_row_bytes = static_cast<std::size_t>(ops::kvarn_k_row_bytes(Bits));
    const std::size_t v_row_bytes = static_cast<std::size_t>(ops::kvarn_v_row_bytes(Bits));
    std::vector<std::uint8_t> k_roundtrip(k_codes.size(), 0);
    for (int tile = 0; tile < kTiles; ++tile) {
        for (int d = 0; d < kD; ++d) {
            const std::size_t row = (static_cast<std::size_t>(tile) * kD + d) * k_row_bytes;
            for (int token = 0; token < kGroup; ++token) {
                const std::uint32_t code =
                    ops::kvarn_unpack_code(&k_codes[row], token * Bits, Bits);
                ops::kvarn_pack_code(&k_roundtrip[row], token * Bits, Bits, code);
            }
        }
    }
    std::vector<std::uint8_t> v_roundtrip(v_codes.size(), 0);
    for (int tile = 0; tile < kTiles; ++tile) {
        for (int token = 0; token < kGroup; ++token) {
            const std::size_t row = (static_cast<std::size_t>(tile) * kGroup + token) * v_row_bytes;
            for (int d = 0; d < kD; ++d) {
                const std::uint32_t code =
                    ops::kvarn_unpack_code(&v_codes[row], d * Bits, Bits);
                ops::kvarn_pack_code(&v_roundtrip[row], d * Bits, Bits, code);
            }
        }
    }
    failures += check_roundtrip("KVarN K pack/unpack roundtrip", k_codes, k_roundtrip);
    failures += check_roundtrip("KVarN V pack/unpack roundtrip", v_codes, v_roundtrip);
    return failures;
}

// Exhaustive per-code bit-order roundtrip for every packed width. Each representable code, placed
// at each K/V in-row field offset, must read back identical, and a code must never touch or read a
// byte outside its own row. The device roundtrip above only re-packs the codes the kernels happened
// to produce; this pins the little-endian LSB-first field order itself, including the codes that
// straddle a byte boundary (bits 5 and 6 at most offsets), on the host with no device involved.
int run_codec_bit_order_case() {
    constexpr int kCodecRowBytes = 256; // >= the widest row (V, 6 bits -> 192 B)
    int failures                 = 0;
    for (int bits : {4, 5, 6}) {
        const std::uint32_t qmax = (1U << bits) - 1U;
        for (int key = 0; key < 2; ++key) {
            const bool is_key    = key == 1;
            const int fields     = is_key ? ops::kKvarnGroup : ops::kKvarnHeadDim;
            const int row_bytes  = is_key ? ops::kvarn_k_row_bytes(bits) : ops::kvarn_v_row_bytes(bits);
            const char* side     = is_key ? "K" : "V";
            for (int field = 0; field < fields; ++field) {
                const std::int32_t bit = field * bits;
                const int low_byte     = bit >> 3;
                const int high_byte    = ((bit & 7) + bits > 8) ? low_byte + 1 : low_byte;
                for (std::uint32_t code = 0; code <= qmax; ++code) {
                    std::uint8_t row[kCodecRowBytes];
                    std::memset(row, 0, sizeof(row));
                    // A sentinel beyond the row end proves the codec never reads out of its row.
                    std::memset(row + row_bytes, 0xff, sizeof(row) - static_cast<std::size_t>(row_bytes));
                    ops::kvarn_pack_code(row, bit, bits, code);
                    const std::uint32_t readback = ops::kvarn_unpack_code(row, bit, bits);
                    if (readback != code) {
                        std::cerr << "KVarN " << side << " bit-order: bits=" << bits
                                  << " field=" << field << " code=" << code
                                  << " readback=" << readback << '\n';
                        ++failures;
                    }
                    for (int byte = 0; byte < row_bytes; ++byte) {
                        if (byte != low_byte && byte != high_byte && row[byte] != 0) {
                            std::cerr << "KVarN " << side << " bit-order: bits=" << bits
                                      << " field=" << field << " code=" << code << " clobbered byte "
                                      << byte << '\n';
                            ++failures;
                        }
                    }
                }
            }
        }
    }
    if (failures == 0) { std::cout << "KVarN codec bit-order roundtrip: 4/5/6-bit exhaustive OK\n"; }
    return failures;
}

int run_hadamard_case() {
    constexpr int kVectors = 11;
    std::vector<float> input(static_cast<std::size_t>(kD) * kVectors);
    fill_uniform(input, 0x48414441U, -3.0F, 3.0F);
    round_to_bf16(input);
    std::vector<double> expected(input.size());
    for (int vector = 0; vector < kVectors; ++vector) {
        for (int row = 0; row < kD; ++row) {
            double sum = 0.0;
            for (int col = 0; col < kD; ++col) {
                const bool negative =
                    (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                sum += (negative ? -1.0 : 1.0) * input[static_cast<std::size_t>(vector) * kD + col];
            }
            expected[static_cast<std::size_t>(vector) * kD + row] = sum / 16.0;
        }
    }
    DeviceBuffer source = to_device_bf16(input);
    DeviceBuffer destination(input.size() * sizeof(std::uint16_t));
    Tensor source_tensor(source.p, DType::BF16, {kD, kVectors});
    Tensor destination_tensor(destination.p, DType::BF16, {kD, kVectors});
    ops::kvarn_hadamard(source_tensor, destination_tensor, nullptr);
    cuda_synchronize();
    return compare_profile("KVarN Hadamard", from_device_bf16(destination, input.size()), expected,
                           2.0e-3);
}

std::vector<float> make_cache_values(int tokens, std::uint32_t seed, int heads = 1) {
    std::vector<float> values(static_cast<std::size_t>(kD) * heads * tokens);
    fill_uniform(values, seed, -2.0F, 2.0F);
    round_to_bf16(values);
    return values;
}

template <int Heads, int Pages = 4, int Bits = kBits>
struct CacheFixture {
    static constexpr int kHeads = Heads;
    static constexpr int kPages = Pages;
    static constexpr int kBits  = Bits;
    static constexpr int kRecordBytes = ops::kvarn_record_bytes(Bits, Bits);
    DeviceBuffer records{static_cast<std::size_t>(kRecordBytes) * kHeads * kPages};
    DeviceBuffer tail_k{static_cast<std::size_t>(kD) * kGroup * kHeads * ops::kKvarnTailSlots *
                        sizeof(std::uint16_t)};
    DeviceBuffer tail_v{static_cast<std::size_t>(kD) * kGroup * kHeads * ops::kKvarnTailSlots *
                        sizeof(std::uint16_t)};
    DeviceBuffer markers{ops::kKvarnTailSlots * sizeof(std::int32_t)};
    DeviceBuffer block_tables{kPages * sizeof(std::int32_t)};

    CacheFixture() {
        records.fill();
        tail_k.fill();
        tail_v.fill();
        markers.copy_from_host(std::vector<std::int32_t>(ops::kKvarnTailSlots, -1).data(),
                               markers.bytes);
        std::vector<std::int32_t> table(kPages);
        std::iota(table.begin(), table.end(), 0);
        block_tables.copy_from_host(table.data(), block_tables.bytes);
    }

    ops::KvarnPagedBatchLayerView view() {
        return {
            .records = Tensor(records.p, DType::U8,
                              {kRecordBytes / kGroup, kGroup, kHeads, kPages}),
            .tail_k = Tensor(tail_k.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots, 1}),
            .tail_v = Tensor(tail_v.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots, 1}),
            .tail_logical_pages = Tensor(markers.p, DType::I32, {ops::kKvarnTailSlots, 1}),
            .block_tables       = Tensor(block_tables.p, DType::I32, {kPages, 1}),
            .num_kv_heads       = kHeads,
            .bits               = Bits,
        };
    }

    ops::KvarnPagedLayerView layer_view() {
        return {
            .records = Tensor(records.p, DType::U8,
                              {kRecordBytes / kGroup, kGroup, kHeads, kPages}),
            .tail_k  = Tensor(tail_k.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots}),
            .tail_v  = Tensor(tail_v.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots}),
            .tail_logical_pages = Tensor(markers.p, DType::I32, {ops::kKvarnTailSlots}),
            .block_table        = Tensor(block_tables.p, DType::I32, {kPages}),
            .num_kv_heads       = kHeads,
            .bits               = Bits,
        };
    }
};

template <int Heads>
struct BatchCacheFixture {
    static constexpr int kHeads         = Heads;
    static constexpr int kRows          = 2;
    static constexpr int kLogicalPages  = 6;
    static constexpr int kPhysicalPages = kRows * kLogicalPages;
    DeviceBuffer records{static_cast<std::size_t>(kRecordBytes) * kHeads *
                         kPhysicalPages};
    DeviceBuffer tail_k{static_cast<std::size_t>(kD) * kGroup * kHeads * ops::kKvarnTailSlots *
                        kRows * sizeof(std::uint16_t)};
    DeviceBuffer tail_v{static_cast<std::size_t>(kD) * kGroup * kHeads * ops::kKvarnTailSlots *
                        kRows * sizeof(std::uint16_t)};
    DeviceBuffer markers{ops::kKvarnTailSlots * kRows * sizeof(std::int32_t)};
    DeviceBuffer block_tables{kLogicalPages * kRows * sizeof(std::int32_t)};
    std::vector<std::int32_t> host_block_tables;

    BatchCacheFixture() : host_block_tables(kPhysicalPages) {
        records.fill();
        tail_k.fill();
        tail_v.fill();
        markers.copy_from_host(std::vector<std::int32_t>(ops::kKvarnTailSlots * kRows, -1).data(),
                               markers.bytes);
        for (int physical = 0; physical < kPhysicalPages; ++physical) {
            host_block_tables[physical] = physical;
        }
        block_tables.copy_from_host(host_block_tables.data(), block_tables.bytes);
    }

    ops::KvarnPagedBatchLayerView view() {
        return {
            .records = Tensor(records.p, DType::U8,
                              {kRecordBytes / kGroup, kGroup, kHeads, kPhysicalPages}),
            .tail_k =
                Tensor(tail_k.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots, kRows}),
            .tail_v =
                Tensor(tail_v.p, DType::BF16, {kD, kGroup, kHeads * ops::kKvarnTailSlots, kRows}),
            .tail_logical_pages = Tensor(markers.p, DType::I32, {ops::kKvarnTailSlots, kRows}),
            .block_tables       = Tensor(block_tables.p, DType::I32, {kLogicalPages, kRows}),
            .num_kv_heads       = kHeads,
        };
    }
};

float decode_cache_value(const std::vector<std::uint8_t>& records,
                         const std::vector<std::uint16_t>& tail,
                         const std::vector<std::int32_t>& markers, bool key, int position, int head,
                         int heads, int d, int table_row = 0, int logical_pages = 4,
                         const std::vector<std::int32_t>* block_tables = nullptr,
                         int bits = kBits) {
    const int page  = position / kGroup;
    const int token = position % kGroup;
    for (int slot = 0; slot < ops::kKvarnTailSlots; ++slot) {
        if (markers[slot + ops::kKvarnTailSlots * table_row] == page) {
            const std::size_t tail_index =
                static_cast<std::size_t>(d) +
                static_cast<std::size_t>(kD) *
                    (token + kGroup * (head + heads * (slot + ops::kKvarnTailSlots * table_row)));
            return bf16_to_f32(tail[tail_index]);
        }
    }
    const int physical =
        block_tables == nullptr ? page : (*block_tables)[page + logical_pages * table_row];
    const std::uint8_t* record =
        records.data() +
        (static_cast<std::size_t>(physical) * heads + head) * ops::kvarn_record_bytes(bits, bits);
    if (key) {
        const int code = static_cast<int>(ops::kvarn_unpack_code(
            record + ops::kvarn_k_packed_offset(bits, bits) + d * ops::kvarn_k_row_bytes(bits),
            token * bits, bits));
        std::uint16_t scale_bits;
        std::uint16_t zero_bits;
        std::uint16_t token_bits;
        std::memcpy(&scale_bits, record + ops::kvarn_k_scale_offset(bits, bits) + 2 * d, 2);
        std::memcpy(&zero_bits, record + ops::kvarn_k_zero_offset(bits, bits) + 2 * d, 2);
        std::memcpy(&token_bits, record + ops::kvarn_k_token_scale_offset(bits, bits) + 2 * token,
                    2);
        return std::fma(static_cast<float>(code), f16_to_f32(scale_bits), f16_to_f32(zero_bits)) *
               f16_to_f32(token_bits);
    }
    const int code = static_cast<int>(ops::kvarn_unpack_code(
        record + ops::kvarn_v_packed_offset(bits, bits) + token * ops::kvarn_v_row_bytes(bits),
        d * bits, bits));
    std::uint16_t channel_bits;
    std::uint16_t scale_bits;
    std::uint16_t zero_bits;
    std::memcpy(&channel_bits, record + ops::kvarn_v_channel_scale_offset(bits, bits) + 2 * d, 2);
    std::memcpy(&scale_bits, record + ops::kvarn_v_token_scale_offset(bits, bits) + 2 * token, 2);
    std::memcpy(&zero_bits, record + ops::kvarn_v_token_zero_offset(bits, bits) + 2 * token, 2);
    return std::fma(static_cast<float>(code), f16_to_f32(scale_bits), f16_to_f32(zero_bits)) *
           f16_to_f32(channel_bits);
}

template <int Heads, int Pages, int Bits>
void append_cache(CacheFixture<Heads, Pages, Bits>& cache, const std::vector<float>& key,
                  const std::vector<float>& value, int first_position, bool provisional) {
    const int width = static_cast<int>(key.size() / (kD * Heads));
    std::vector<std::int32_t> positions(width);
    for (int index = 0; index < width; ++index) positions[index] = first_position + index;
    DeviceBuffer device_key       = to_device_bf16(key);
    DeviceBuffer device_value     = to_device_bf16(value);
    DeviceBuffer device_positions = to_device(positions);
    DeviceBuffer rows             = to_device(std::vector<std::int32_t>{0});
    Tensor key_tensor(device_key.p, DType::BF16, {kD, Heads, width, 1});
    Tensor value_tensor(device_value.p, DType::BF16, {kD, Heads, width, 1});
    Tensor position_tensor(device_positions.p, DType::I32, {width, 1});
    Tensor row_tensor(rows.p, DType::I32, {1});
    ops::kvarn_kv_append(key_tensor, value_tensor, position_tensor, Tensor{}, row_tensor,
                         cache.view(), provisional, nullptr);
    cuda_synchronize();
}

template <int Heads>
void append_cache_row(BatchCacheFixture<Heads>& cache, const std::vector<float>& key,
                      const std::vector<float>& value, int first_position, int table_row) {
    const int width = static_cast<int>(key.size() / (kD * Heads));
    std::vector<std::int32_t> host_positions(width);
    for (int index = 0; index < width; ++index) host_positions[index] = first_position + index;
    DeviceBuffer device_key   = to_device_bf16(key);
    DeviceBuffer device_value = to_device_bf16(value);
    DeviceBuffer positions    = to_device(host_positions);
    DeviceBuffer rows         = to_device(std::vector<std::int32_t>{table_row});
    Tensor key_tensor(device_key.p, DType::BF16, {kD, Heads, width, 1});
    Tensor value_tensor(device_value.p, DType::BF16, {kD, Heads, width, 1});
    Tensor position_tensor(positions.p, DType::I32, {width, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});
    ops::kvarn_kv_append(key_tensor, value_tensor, position_tensor, Tensor{}, rows_tensor,
                         cache.view(), false, nullptr);
    cuda_synchronize();
}

template <int Heads, int Pages, int Bits>
int run_cached_attention_case(CacheFixture<Heads, Pages, Bits>& cache, int query_heads,
                              int first_position, int width, const char* label) {
    std::vector<float> query  = make_cache_values(query_heads * width, 0x4001U + query_heads);
    DeviceBuffer device_query = to_device_bf16(query);
    DeviceBuffer output(query.size() * sizeof(std::uint16_t));
    std::vector<std::int32_t> query_positions(width);
    for (int column = 0; column < width; ++column) {
        query_positions[column] = first_position + column;
    }
    DeviceBuffer positions = to_device(query_positions);
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{0});
    Tensor query_tensor(device_query.p, DType::BF16, {kD, query_heads, width, 1});
    Tensor output_tensor(output.p, DType::BF16, {kD, query_heads, width, 1});
    Tensor position_tensor(positions.p, DType::I32, {width, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});
    const ops::CausalAttentionExecutionEnvelope envelope{
        1, static_cast<std::uint32_t>(first_position + width)};
    WorkspaceArena workspace(std::max<std::size_t>(
        1, ops::kvarn_attention_workspace_capacity_bytes(query_heads, envelope, 1, width, width)));
    ops::kvarn_attention_cached(query_tensor, position_tensor, rows_tensor, 0.0625F, cache.view(),
                                envelope, workspace, output_tensor, nullptr);
    cuda_synchronize();

    std::vector<double> rotated_query(query.size());
    for (int column = 0; column < width; ++column) {
        for (int head = 0; head < query_heads; ++head) {
            for (int row = 0; row < kD; ++row) {
                double sum = 0.0;
                for (int col = 0; col < kD; ++col) {
                    const bool negative =
                        (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                    const std::size_t input_index =
                        static_cast<std::size_t>(col) +
                        static_cast<std::size_t>(kD) * (head + query_heads * column);
                    sum += (negative ? -1.0 : 1.0) * query[input_index];
                }
                const std::size_t output_index =
                    static_cast<std::size_t>(row) +
                    static_cast<std::size_t>(kD) * (head + query_heads * column);
                rotated_query[output_index] = sum / 16.0;
            }
        }
    }
    const auto record_values = from_device<std::uint8_t>(cache.records, cache.records.bytes);
    const auto tail_key      = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto tail_value    = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    const auto marker_values = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
    std::vector<double> expected(query.size());
    for (int column = 0; column < width; ++column) {
        const int visible = query_positions[column] + 1;
        for (int head = 0; head < query_heads; ++head) {
            std::vector<double> scores(visible);
            double maximum = -std::numeric_limits<double>::infinity();
            for (int position = 0; position < visible; ++position) {
                double dot = 0.0;
                for (int d = 0; d < kD; ++d) {
                    const std::size_t query_index =
                        static_cast<std::size_t>(d) +
                        static_cast<std::size_t>(kD) * (head + query_heads * column);
                    dot += rotated_query[query_index] *
                           decode_cache_value(record_values, tail_key, marker_values, true,
                                              position, head / (query_heads / Heads), Heads, d, 0,
                                              4, nullptr, Bits);
                }
                scores[position] = dot * 0.0625;
                maximum          = std::max(maximum, scores[position]);
            }
            double denominator = 0.0;
            std::vector<double> rotated(kD, 0.0);
            for (int position = 0; position < visible; ++position) {
                const double probability = std::exp(scores[position] - maximum);
                denominator += probability;
                for (int d = 0; d < kD; ++d) {
                    rotated[d] +=
                        probability * decode_cache_value(record_values, tail_value, marker_values,
                                                         false, position,
                                                         head / (query_heads / Heads), Heads, d, 0,
                                                         4, nullptr, Bits);
                }
            }
            for (double& value : rotated) { value /= denominator; }
            for (int row = 0; row < kD; ++row) {
                double sum = 0.0;
                for (int col = 0; col < kD; ++col) {
                    const bool negative =
                        (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                    sum += (negative ? -1.0 : 1.0) * rotated[col];
                }
                const std::size_t output_index =
                    static_cast<std::size_t>(row) +
                    static_cast<std::size_t>(kD) * (head + query_heads * column);
                expected[output_index] = sum / 16.0;
            }
        }
    }
    int failures = compare_profile(label, from_device_bf16(output, query.size()), expected, 8.0e-3);
    if (width == 1 || (width == 2 && Heads == 4) || (width == 6 && Heads == 2) || width == 64) {
        DeviceBuffer original_query = to_device_bf16(query);
        cudaStream_t stream         = nullptr;
        cudaGraph_t graph           = nullptr;
        cudaGraphExec_t executable  = nullptr;
        cuda_check(cudaStreamCreate(&stream), "create KVarN graph stream");
        cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
                   "begin KVarN capture");
        ops::kvarn_attention_cached(query_tensor, position_tensor, rows_tensor, 0.0625F,
                                    cache.view(), envelope, workspace, output_tensor, stream);
        cuda_check(cudaStreamEndCapture(stream, &graph), "end KVarN capture");
        cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
                   "instantiate KVarN graph");
        cuda_check(cudaMemcpyAsync(device_query.p, original_query.p, original_query.bytes,
                                   cudaMemcpyDeviceToDevice, stream),
                   "restore KVarN graph query");
        cuda_check(cudaGraphLaunch(executable, stream), "launch KVarN graph");
        cuda_synchronize(stream);
        failures += compare_profile("KVarN CUDA Graph attention",
                                    from_device_bf16(output, query.size()), expected, 8.0e-3);
        cudaGraphExecDestroy(executable);
        cudaGraphDestroy(graph);
        cudaStreamDestroy(stream);
    }
    return failures;
}

int run_prefill_slab_boundary_case() {
    constexpr int Heads         = 4;
    constexpr int QueryHeads    = 24;
    constexpr int Width         = 64;
    constexpr int FirstPosition = ops::kvarn::PrefillSlabTokens;
    constexpr int Context       = FirstPosition + Width;
    constexpr int Pages         = (Context + kGroup - 1) / kGroup;
    constexpr int SampleHeads[] = {0, QueryHeads - 1};

    CacheFixture<Heads, Pages> cache;
    append_cache(cache, make_cache_values(Context, 0x7101U, Heads),
                 make_cache_values(Context, 0x7102U, Heads), 0, false);

    const std::vector<float> query_vectors = make_cache_values(QueryHeads, 0x7201U);
    std::vector<float> query(static_cast<std::size_t>(kD) * QueryHeads * Width);
    for (int column = 0; column < Width; ++column) {
        std::copy(query_vectors.begin(), query_vectors.end(),
                  query.begin() + static_cast<std::size_t>(column) * kD * QueryHeads);
    }
    std::vector<std::int32_t> host_positions(Width);
    std::iota(host_positions.begin(), host_positions.end(), FirstPosition);
    DeviceBuffer device_query   = to_device_bf16(query);
    DeviceBuffer original_query = to_device_bf16(query);
    DeviceBuffer output(query.size() * sizeof(std::uint16_t));
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{0});
    Tensor query_tensor(device_query.p, DType::BF16, {kD, QueryHeads, Width, 1});
    Tensor output_tensor(output.p, DType::BF16, {kD, QueryHeads, Width, 1});
    Tensor positions_tensor(positions.p, DType::I32, {Width, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});
    const ops::CausalAttentionExecutionEnvelope envelope{1, Context};
    WorkspaceArena workspace(
        ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, Width, Width));
    ops::kvarn_attention_cached(query_tensor, positions_tensor, rows_tensor, 0.0625F, cache.view(),
                                envelope, workspace, output_tensor, nullptr);
    cuda_synchronize();

    const auto record_values = from_device<std::uint8_t>(cache.records, cache.records.bytes);
    const auto tail_key      = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto tail_value    = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    const auto marker_values = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
    std::vector<double> expected(static_cast<std::size_t>(std::size(SampleHeads)) * Width * kD);
    for (int sample = 0; sample < static_cast<int>(std::size(SampleHeads)); ++sample) {
        const int head    = SampleHeads[sample];
        const int kv_head = head / (QueryHeads / Heads);
        std::vector<double> rotated_query(kD);
        for (int row = 0; row < kD; ++row) {
            double sum = 0.0;
            for (int col = 0; col < kD; ++col) {
                const bool negative =
                    (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                sum += (negative ? -1.0 : 1.0) * query_vectors[static_cast<std::size_t>(col) +
                                                               static_cast<std::size_t>(kD) * head];
            }
            rotated_query[row] = sum / 16.0;
        }

        double maximum     = -std::numeric_limits<double>::infinity();
        double denominator = 0.0;
        std::vector<double> numerator(kD, 0.0);
        for (int position = 0; position < Context; ++position) {
            double dot = 0.0;
            for (int d = 0; d < kD; ++d) {
                dot += rotated_query[d] * decode_cache_value(record_values, tail_key, marker_values,
                                                             true, position, kv_head, Heads, d, 0,
                                                             Pages);
            }
            const double score           = dot * 0.0625;
            const double previous_weight = score > maximum ? std::exp(maximum - score) : 1.0;
            const double value_weight    = score > maximum ? 1.0 : std::exp(score - maximum);
            maximum                      = std::max(maximum, score);
            denominator                  = denominator * previous_weight + value_weight;
            for (int d = 0; d < kD; ++d) {
                numerator[d] =
                    numerator[d] * previous_weight +
                    value_weight * decode_cache_value(record_values, tail_value, marker_values,
                                                      false, position, kv_head, Heads, d, 0, Pages);
            }
            if (position < FirstPosition) { continue; }
            std::vector<double> normalized(kD);
            for (int d = 0; d < kD; ++d) { normalized[d] = numerator[d] / denominator; }
            const int column = position - FirstPosition;
            for (int row = 0; row < kD; ++row) {
                double sum = 0.0;
                for (int col = 0; col < kD; ++col) {
                    const bool negative =
                        (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                    sum += (negative ? -1.0 : 1.0) * normalized[col];
                }
                const std::size_t index =
                    static_cast<std::size_t>(row) +
                    static_cast<std::size_t>(kD) * (sample + std::size(SampleHeads) * column);
                expected[index] = sum / 16.0;
            }
        }
    }

    const auto compare = [&](const char* label) {
        const auto actual_full = from_device_bf16(output, query.size());
        std::vector<double> actual(expected.size());
        for (int column = 0; column < Width; ++column) {
            for (int sample = 0; sample < static_cast<int>(std::size(SampleHeads)); ++sample) {
                const int head = SampleHeads[sample];
                for (int d = 0; d < kD; ++d) {
                    const std::size_t selected =
                        static_cast<std::size_t>(d) +
                        static_cast<std::size_t>(kD) * (sample + std::size(SampleHeads) * column);
                    const std::size_t full =
                        static_cast<std::size_t>(d) +
                        static_cast<std::size_t>(kD) * (head + QueryHeads * column);
                    actual[selected] = actual_full[full];
                }
            }
        }
        return compare_profile(label, actual, expected, 8.0e-3);
    };
    int failures = compare("KVarN slab-boundary attention");

    cudaStream_t stream        = nullptr;
    cudaGraph_t graph          = nullptr;
    cudaGraphExec_t executable = nullptr;
    cuda_check(cudaStreamCreate(&stream), "create slab-boundary graph stream");
    cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
               "begin slab-boundary graph capture");
    ops::kvarn_attention_cached(query_tensor, positions_tensor, rows_tensor, 0.0625F, cache.view(),
                                envelope, workspace, output_tensor, stream);
    cuda_check(cudaStreamEndCapture(stream, &graph), "end slab-boundary graph capture");
    cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
               "instantiate slab-boundary graph");
    cuda_check(cudaMemcpyAsync(device_query.p, original_query.p, original_query.bytes,
                               cudaMemcpyDeviceToDevice, stream),
               "restore slab-boundary graph query");
    cuda_check(cudaGraphLaunch(executable, stream), "launch slab-boundary graph");
    cuda_synchronize(stream);
    failures += compare("KVarN slab-boundary CUDA Graph attention");
    cudaGraphExecDestroy(executable);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);
    return failures;
}

template <int Heads>
int run_batched_attention_case(int query_heads, const char* label) {
    constexpr int Batch                  = 2;
    constexpr int Width                  = 6;
    constexpr int GuardBytes             = 4096;
    const std::int32_t row_first[Batch]  = {253, 70};
    const std::int32_t table_rows[Batch] = {1, 0};

    BatchCacheFixture<Heads> cache;
    append_cache_row(cache, make_cache_values(row_first[0] + Width, 0x8101U + Heads, Heads),
                     make_cache_values(row_first[0] + Width, 0x8201U + Heads, Heads), 0, 0);
    append_cache_row(cache, make_cache_values(row_first[1] + Width, 0x9101U + Heads, Heads),
                     make_cache_values(row_first[1] + Width, 0x9201U + Heads, Heads), 0, 1);
    BatchCacheFixture<Heads> graph_cache;
    append_cache_row(graph_cache, make_cache_values(row_first[0] + Width, 0x8101U + Heads, Heads),
                     make_cache_values(row_first[0] + Width, 0x8201U + Heads, Heads), 0, 0);
    append_cache_row(graph_cache, make_cache_values(row_first[1] + Width, 0x9101U + Heads, Heads),
                     make_cache_values(row_first[1] + Width, 0x9201U + Heads, Heads), 0, 1);

    std::vector<float> query =
        make_cache_values(query_heads * Width * Batch, 0xa001U + query_heads);
    DeviceBuffer device_query   = to_device_bf16(query);
    DeviceBuffer original_query = to_device_bf16(query);
    DeviceBuffer output(query.size() * sizeof(std::uint16_t));
    std::vector<std::int32_t> host_positions(Width * Batch);
    for (int batch = 0; batch < Batch; ++batch) {
        for (int column = 0; column < Width; ++column) {
            host_positions[column + Width * batch] = row_first[table_rows[batch]] + column;
        }
    }
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{table_rows[0], table_rows[1]});
    Tensor query_tensor(device_query.p, DType::BF16, {kD, query_heads, Width, Batch});
    Tensor output_tensor(output.p, DType::BF16, {kD, query_heads, Width, Batch});
    Tensor positions_tensor(positions.p, DType::I32, {Width, Batch});
    Tensor rows_tensor(rows.p, DType::I32, {Batch});
    const ops::CausalAttentionExecutionEnvelope envelope{1, 512};
    const std::size_t workspace_bytes =
        ops::kvarn_attention_workspace_capacity_bytes(query_heads, envelope, Batch, Width, Width);
    DeviceBuffer workspace_storage(workspace_bytes + 2 * GuardBytes);
    workspace_storage.fill(0xa5);
    auto* workspace_base = static_cast<std::uint8_t*>(workspace_storage.p) + GuardBytes;
    cuda_check(cudaMemset(workspace_base, 0x41, workspace_bytes), "poison B=2 KVarN workspace");
    WorkspaceArena workspace(DeviceSpan{workspace_base, workspace_bytes});

    ops::kvarn_attention_cached(query_tensor, positions_tensor, rows_tensor, 0.0625F, cache.view(),
                                envelope, workspace, output_tensor, nullptr);
    cuda_synchronize();

    std::vector<double> rotated_query(query.size());
    for (int batch = 0; batch < Batch; ++batch) {
        for (int column = 0; column < Width; ++column) {
            for (int head = 0; head < query_heads; ++head) {
                for (int row = 0; row < kD; ++row) {
                    double sum = 0.0;
                    for (int col = 0; col < kD; ++col) {
                        const bool negative =
                            (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                        const std::size_t index =
                            static_cast<std::size_t>(col) +
                            static_cast<std::size_t>(kD) *
                                (head + query_heads * (column + Width * batch));
                        sum += (negative ? -1.0 : 1.0) * query[index];
                    }
                    const std::size_t index = static_cast<std::size_t>(row) +
                                              static_cast<std::size_t>(kD) *
                                                  (head + query_heads * (column + Width * batch));
                    rotated_query[index] = sum / 16.0;
                }
            }
        }
    }

    const auto record_values = from_device<std::uint8_t>(cache.records, cache.records.bytes);
    const auto tail_key      = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto tail_value    = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    const auto marker_values =
        from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots * Batch);
    std::vector<double> expected(query.size());
    for (int batch = 0; batch < Batch; ++batch) {
        for (int column = 0; column < Width; ++column) {
            const int visible = host_positions[column + Width * batch] + 1;
            for (int head = 0; head < query_heads; ++head) {
                std::vector<double> scores(visible);
                double maximum = -std::numeric_limits<double>::infinity();
                for (int position = 0; position < visible; ++position) {
                    double dot = 0.0;
                    for (int d = 0; d < kD; ++d) {
                        const std::size_t query_index =
                            static_cast<std::size_t>(d) +
                            static_cast<std::size_t>(kD) *
                                (head + query_heads * (column + Width * batch));
                        dot += rotated_query[query_index] *
                               decode_cache_value(record_values, tail_key, marker_values, true,
                                                  position, head / (query_heads / Heads), Heads, d,
                                                  table_rows[batch],
                                                  BatchCacheFixture<Heads>::kLogicalPages,
                                                  &cache.host_block_tables);
                    }
                    scores[position] = dot * 0.0625;
                    maximum          = std::max(maximum, scores[position]);
                }
                double denominator = 0.0;
                std::vector<double> rotated(kD, 0.0);
                for (int position = 0; position < visible; ++position) {
                    const double probability = std::exp(scores[position] - maximum);
                    denominator += probability;
                    for (int d = 0; d < kD; ++d) {
                        rotated[d] +=
                            probability *
                            decode_cache_value(
                                record_values, tail_value, marker_values, false, position,
                                head / (query_heads / Heads), Heads, d, table_rows[batch],
                                BatchCacheFixture<Heads>::kLogicalPages, &cache.host_block_tables);
                    }
                }
                for (double& item : rotated) { item /= denominator; }
                for (int row = 0; row < kD; ++row) {
                    double sum = 0.0;
                    for (int col = 0; col < kD; ++col) {
                        const bool negative =
                            (std::popcount(static_cast<unsigned>(row & col)) & 1) != 0;
                        sum += (negative ? -1.0 : 1.0) * rotated[col];
                    }
                    const std::size_t output_index =
                        static_cast<std::size_t>(row) +
                        static_cast<std::size_t>(kD) *
                            (head + query_heads * (column + Width * batch));
                    expected[output_index] = sum / 16.0;
                }
            }
        }
    }

    const auto check_canaries = [&]() {
        std::vector<std::uint8_t> guards(2 * GuardBytes);
        workspace_storage.copy_to_host(guards.data(), GuardBytes);
        workspace_storage.copy_to_host(guards.data() + GuardBytes, GuardBytes,
                                       GuardBytes + workspace_bytes);
        if (std::all_of(guards.begin(), guards.end(),
                        [](std::uint8_t value) { return value == 0xa5; })) {
            return 0;
        }
        std::cerr << label << " workspace canary overwritten\n";
        return 1;
    };

    int failures = compare_profile(label, from_device_bf16(output, query.size()), expected, 8.0e-3);
    failures += check_canaries();

    cudaStream_t stream        = nullptr;
    cudaGraph_t graph          = nullptr;
    cudaGraphExec_t executable = nullptr;
    cuda_check(cudaStreamCreate(&stream), "create B=2 KVarN graph stream");
    cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
               "begin B=2 KVarN capture");
    ops::kvarn_attention_cached(query_tensor, positions_tensor, rows_tensor, 0.0625F,
                                graph_cache.view(), envelope, workspace, output_tensor, stream);
    cuda_check(cudaStreamEndCapture(stream, &graph), "end B=2 KVarN capture");
    cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
               "instantiate B=2 KVarN graph");
    cuda_check(cudaMemcpyAsync(device_query.p, original_query.p, original_query.bytes,
                               cudaMemcpyDeviceToDevice, stream),
               "restore B=2 KVarN graph query");
    cuda_check(cudaGraphLaunch(executable, stream), "launch B=2 KVarN graph");
    cuda_synchronize(stream);
    failures += compare_profile((std::string(label) + " CUDA Graph").c_str(),
                                from_device_bf16(output, query.size()), expected, 8.0e-3);
    failures += check_canaries();
    cudaGraphExecDestroy(executable);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);
    return failures;
}

int run_cache_lifecycle_case() {
    CacheFixture<2> cache;
    constexpr int first = 2 * kGroup - 8;
    append_cache(cache, make_cache_values(first, 0x1001U, 2), make_cache_values(first, 0x1002U, 2),
                 0, false);
    append_cache(cache, make_cache_values(16, 0x2001U, 2), make_cache_values(16, 0x2002U, 2), first,
                 true);

    std::vector<std::int32_t> provisional_positions(16);
    for (int index = 0; index < 16; ++index) provisional_positions[index] = first + index;
    DeviceBuffer positions = to_device(provisional_positions);
    DeviceBuffer accepted  = to_device(std::vector<std::int32_t>{8});
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{0});
    Tensor positions_tensor(positions.p, DType::I32, {16, 1});
    Tensor accepted_tensor(accepted.p, DType::I32, {1});
    Tensor rows_tensor(rows.p, DType::I32, {1});
    ops::kvarn_commit_pages(positions_tensor, accepted_tensor, rows_tensor, cache.view(), nullptr);
    cuda_synchronize();

    append_cache(cache, make_cache_values(8, 0x3001U, 2), make_cache_values(8, 0x3002U, 2),
                 2 * kGroup, false);
    const auto marker_values = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
    int failures             = 0;
    const std::vector<std::int32_t> expected_markers{0, 2, -1};
    failures += verify_exact("KVarN sink/tail lifecycle", marker_values, expected_markers);

    const auto record_values = from_device<std::uint8_t>(cache.records, cache.records.bytes);
    const auto tail_key      = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto tail_value    = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    failures +=
        run_cached_attention_case(cache, 16, 2 * kGroup + 2, 6, "KVarN H16/KV2 width-6 attention");

    ops::kvarn_restore_tail(150, std::array{cache.layer_view()}, nullptr);
    cuda_synchronize();
    const auto restored_markers = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
    failures += verify_exact("KVarN restored tail markers", restored_markers,
                             std::vector<std::int32_t>{0, 1, -1});
    const auto restored_key   = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto restored_value = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    for (int token = 0; token < kGroup; ++token) {
        for (int d = 0; d < kD; ++d) {
            const std::size_t index =
                static_cast<std::size_t>(d) + static_cast<std::size_t>(kD) * (token + kGroup * 2);
            const std::uint16_t expected_key   = f32_to_bf16(decode_cache_value(
                record_values, tail_key, marker_values, true, 128 + token, 0, 2, d));
            const std::uint16_t expected_value = f32_to_bf16(decode_cache_value(
                record_values, tail_value, marker_values, false, 128 + token, 0, 2, d));
            if (restored_key[index] != expected_key || restored_value[index] != expected_value) {
                std::cerr << "KVarN restored tail mismatch at token=" << token << " d=" << d
                          << '\n';
                ++failures;
                token = kGroup;
                break;
            }
        }
    }
    return failures;
}

int run_27b_attention_case() {
    CacheFixture<4> cache;
    append_cache(cache, make_cache_values(200, 0x5001U, 4), make_cache_values(200, 0x5002U, 4), 0,
                 false);
    int failures = run_cached_attention_case(cache, 24, 199, 1, "KVarN H24/KV4 width-1 attention");
    failures += run_cached_attention_case(cache, 24, 136, 64, "KVarN H24/KV4 tiled attention");
    return failures;
}

template <int Width, int Valid = Width, int Context = 122943>
int run_27b_grouped_decode_case() {
    static_assert(Width >= 2 && Width <= 16);
    static_assert(Valid > 0 && Valid <= Width);
    constexpr int Heads        = 4;
    constexpr int QueryHeads   = 24;
    constexpr int LogicalPages = (Context + kGroup - 1) / kGroup;

    std::vector<std::uint8_t> host_records(static_cast<std::size_t>(kRecordBytes) * Heads,
                                           0);
    const std::uint16_t one = f32_to_f16(1.0F);
    for (int head = 0; head < Heads; ++head) {
        std::uint8_t* record =
            host_records.data() + static_cast<std::size_t>(head) * kRecordBytes;
        for (int d = 0; d < kD; ++d) {
            std::memcpy(record + ops::kvarn_v_channel_scale_offset(kBits, kBits) + 2 * d, &one,
                        sizeof(one));
        }
        for (int token = 0; token < kGroup; ++token) {
            std::memcpy(record + ops::kvarn_v_token_zero_offset(kBits, kBits) + 2 * token, &one,
                        sizeof(one));
        }
    }

    DeviceBuffer records = to_device(host_records);
    const std::size_t tail_values =
        static_cast<std::size_t>(kD) * kGroup * Heads * ops::kKvarnTailSlots;
    std::vector<float> host_tail_k(tail_values, 0.0F);
    std::vector<float> host_tail_v(tail_values, 0.0F);
    for (int head = 0; head < Heads; ++head) {
        for (int column = 0; column < Width; ++column) {
            const int token         = (Context - Width + column) % kGroup;
            const std::size_t index = static_cast<std::size_t>(kD) * (token + kGroup * head);
            host_tail_k[index]      = 64.0F;
            host_tail_v[index]      = static_cast<float>((column + 1) * (head + 1));
        }
    }
    DeviceBuffer tail_k = to_device_bf16(host_tail_k);
    DeviceBuffer tail_v = to_device_bf16(host_tail_v);
    std::vector<std::int32_t> host_markers(ops::kKvarnTailSlots, -1);
    host_markers[0]      = LogicalPages - 1;
    DeviceBuffer markers = to_device(host_markers);
    DeviceBuffer tables  = to_device(std::vector<std::int32_t>(LogicalPages, 0));
    const ops::KvarnPagedBatchLayerView cache{
        .records =
            Tensor(records.p, DType::U8, {kRecordBytes / kGroup, kGroup, Heads, 1}),
        .tail_k = Tensor(tail_k.p, DType::BF16, {kD, kGroup, Heads * ops::kKvarnTailSlots, 1}),
        .tail_v = Tensor(tail_v.p, DType::BF16, {kD, kGroup, Heads * ops::kKvarnTailSlots, 1}),
        .tail_logical_pages = Tensor(markers.p, DType::I32, {ops::kKvarnTailSlots, 1}),
        .block_tables       = Tensor(tables.p, DType::I32, {LogicalPages, 1}),
        .num_kv_heads       = Heads,
    };

    const std::vector<float> query(static_cast<std::size_t>(kD) * QueryHeads * Width, 1.0F);
    DeviceBuffer device_query   = to_device_bf16(query);
    DeviceBuffer original_query = to_device_bf16(query);
    DeviceBuffer output(query.size() * sizeof(std::uint16_t));
    std::vector<std::int32_t> host_positions(Width);
    std::iota(host_positions.begin(), host_positions.end(), Context - Width);
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer valid     = to_device(std::vector<std::int32_t>{Valid});
    Tensor query_tensor(device_query.p, DType::BF16, {kD, QueryHeads, Width, 1});
    Tensor output_tensor(output.p, DType::BF16, {kD, QueryHeads, Width, 1});
    Tensor position_tensor(positions.p, DType::I32, {Width, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});
    Tensor valid_tensor(valid.p, DType::I32, {1});
    const ops::CausalAttentionExecutionEnvelope envelope{Context - Width + 1, Context};
    WorkspaceArena workspace(
        ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, Width, Width));
    ops::kvarn::decode_attention(query_tensor, position_tensor, valid_tensor, rows_tensor, 0.0625F,
                                 cache, envelope, workspace, output_tensor, nullptr);
    cuda_synchronize();

    std::vector<double> expected(query.size(), 0.0);
    for (int column = 0; column < Valid; ++column) {
        for (int head = 0; head < QueryHeads; ++head) {
            const float average =
                static_cast<float>(column + 2) * 0.5F * static_cast<float>(head / 6 + 1);
            const double value = bf16_to_f32(f32_to_bf16(average * 0.0625F));
            for (int d = 0; d < kD; ++d) {
                expected[static_cast<std::size_t>(d) +
                         static_cast<std::size_t>(kD) * (head + QueryHeads * column)] = value;
            }
        }
    }
    const std::string label = "KVarN H24/KV4 grouped decode width=" + std::to_string(Width) +
                              " valid=" + std::to_string(Valid);
    const auto compare = [&] {
        // Wider prefixes exceed exact BF16 partial-numerator representation.
        return compare_profile(label.c_str(), from_device_bf16(output, query.size()), expected,
                               Width > 8 ? 8.0e-3 : 1.0e-7);
    };
    int failures = compare();

    cudaStream_t stream        = nullptr;
    cudaGraph_t graph          = nullptr;
    cudaGraphExec_t executable = nullptr;
    cuda_check(cudaStreamCreate(&stream), "create grouped KVarN graph stream");
    cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
               "begin grouped KVarN graph capture");
    ops::kvarn::decode_attention(query_tensor, position_tensor, valid_tensor, rows_tensor, 0.0625F,
                                 cache, envelope, workspace, output_tensor, stream);
    cuda_check(cudaStreamEndCapture(stream, &graph), "end grouped KVarN graph capture");
    cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
               "instantiate grouped KVarN graph");
    cuda_check(cudaMemcpyAsync(device_query.p, original_query.p, original_query.bytes,
                               cudaMemcpyDeviceToDevice, stream),
               "restore grouped KVarN graph query");
    cuda_check(cudaGraphLaunch(executable, stream), "launch grouped KVarN graph");
    cuda_synchronize(stream);
    failures += compare();
    cudaGraphExecDestroy(executable);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);
    return failures;
}

int run_35b_attention_case() {
    CacheFixture<2> cache;
    append_cache(cache, make_cache_values(200, 0x6001U, 2), make_cache_values(200, 0x6002U, 2), 0,
                 false);
    return run_cached_attention_case(cache, 16, 136, 64, "KVarN H16/KV2 tiled attention");
}

int run_tail_staging_case(int width) {
    constexpr int Heads  = 4;
    constexpr int Prefix = 2 * kGroup - 2;
    const int Total      = Prefix + width;
    const auto key       = make_cache_values(Total, 0xc001U, Heads);
    const auto value     = make_cache_values(Total, 0xc002U, Heads);
    const auto rotate    = [](const std::vector<float>& input) {
        std::vector<std::uint16_t> result(input.size());
        for (std::size_t base = 0; base < input.size(); base += kD) {
            for (int d = 0; d < kD; ++d) {
                double sum = 0;
                for (int col = 0; col < kD; ++col) {
                    const int sign =
                        (std::popcount(static_cast<unsigned>(d & col)) & 1) ? -1 : 1;
                    sum += sign * input[base + col];
                }
                result[base + d] = f32_to_bf16(static_cast<float>(sum / 16.0));
            }
        }
        return result;
    };
    const auto expected_k  = rotate(key);
    const auto expected_v  = rotate(value);
    const auto prefix_size = static_cast<std::size_t>(Prefix) * Heads * kD;
    // Multiple independent allocations exercise the inter-CTA page-claim race rather than
    // comparing attention against an already-corrupted production cache.
    for (int repeat = 0; repeat < 8; ++repeat) {
        CacheFixture<Heads> cache;
        append_cache(cache, {key.begin(), key.begin() + prefix_size},
                     {value.begin(), value.begin() + prefix_size}, 0, false);
        append_cache(cache, {key.begin() + prefix_size, key.end()},
                     {value.begin() + prefix_size, value.end()}, Prefix, true);
        const auto markers  = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
        const auto actual_k = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
        const auto actual_v = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
        for (int page = 0; page < 3; ++page) {
            if (std::count(markers.begin(), markers.end(), page) != 1) {
                std::cerr << "KVarN staging did not give page " << page << " one tail slot\n";
                return 1;
            }
            const int slot =
                static_cast<int>(std::find(markers.begin(), markers.end(), page) - markers.begin());
            for (int position = page * kGroup; position < std::min(Total, (page + 1) * kGroup);
                 ++position) {
                for (int head = 0; head < Heads; ++head) {
                    for (int d = 0; d < kD; ++d) {
                        const std::size_t source = d + kD * (head + Heads * position);
                        const std::size_t destination =
                            d + kD * (position % kGroup + kGroup * (head + Heads * slot));
                        if (actual_k[destination] != expected_k[source] ||
                            actual_v[destination] != expected_v[source]) {
                            std::cerr << "KVarN staging changed represented K/V at " << position
                                      << '\n';
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

template <int Heads, int QueryHeads, int Pages = 132>
int run_speculative_boundary_case(int width, int valid, int accepted, int first) {
    CacheFixture<Heads, Pages> speculative;
    const auto prefix_k = make_cache_values(first, 0xd001U, Heads);
    const auto prefix_v = make_cache_values(first, 0xd002U, Heads);
    append_cache(speculative, prefix_k, prefix_v, 0, false);
    const auto key   = make_cache_values(width, 0xd003U, Heads);
    const auto value = make_cache_values(width, 0xd004U, Heads);
    const auto query = make_cache_values(width * QueryHeads, 0xd005U);
    std::vector<std::int32_t> host_positions(width);
    std::iota(host_positions.begin(), host_positions.end(), first);
    DeviceBuffer q = to_device_bf16(query), k = to_device_bf16(key), v = to_device_bf16(value);
    DeviceBuffer original_q = to_device_bf16(query), original_k = to_device_bf16(key),
                 original_v      = to_device_bf16(value);
    DeviceBuffer positions       = to_device(host_positions);
    DeviceBuffer rows            = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer counts          = to_device(std::vector<std::int32_t>{valid});
    DeviceBuffer accepted_counts = to_device(std::vector<std::int32_t>{accepted});
    DeviceBuffer output(query.size() * 2);
    Tensor qt(q.p, DType::BF16, {kD, QueryHeads, width, 1});
    Tensor kt(k.p, DType::BF16, {kD, Heads, width, 1});
    Tensor vt(v.p, DType::BF16, {kD, Heads, width, 1});
    Tensor pt(positions.p, DType::I32, {width, 1});
    Tensor rt(rows.p, DType::I32, {1});
    Tensor ct(counts.p, DType::I32, {1});
    Tensor at(accepted_counts.p, DType::I32, {1});
    Tensor ot(output.p, DType::BF16, {kD, QueryHeads, width, 1});
    const ops::CausalAttentionExecutionEnvelope envelope{1,
                                                         static_cast<std::uint32_t>(first + width)};
    WorkspaceArena workspace(
        ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, 1, width));
    const auto body = [&](cudaStream_t stream) {
        cuda_check(cudaMemcpyAsync(q.p, original_q.p, q.bytes, cudaMemcpyDeviceToDevice, stream),
                   "restore query");
        cuda_check(cudaMemcpyAsync(k.p, original_k.p, k.bytes, cudaMemcpyDeviceToDevice, stream),
                   "restore key");
        cuda_check(cudaMemcpyAsync(v.p, original_v.p, v.bytes, cudaMemcpyDeviceToDevice, stream),
                   "restore value");
        ops::kvarn_attention(qt, kt, vt, pt, ct, rt, 0.0625F, speculative.view(), true, envelope,
                             workspace, ot, stream);
    };
    body(nullptr);
    cuda_synchronize();
    const auto first_output = from_device_bf16(output, query.size());
    cuda_check(cudaMemcpy(q.p, original_q.p, q.bytes, cudaMemcpyDeviceToDevice),
               "restore cached query");
    ops::kvarn::decode_attention(qt, pt, ct, rt, 0.0625F, speculative.view(), envelope, workspace,
                                 ot, nullptr);
    cuda_synchronize();
    const auto expected     = from_device_bf16(output, query.size());
    const std::string label = "KVarN speculative boundary first=" + std::to_string(first) +
                              " width=" + std::to_string(width) + " valid=" + std::to_string(valid);
    // Supplementary parity checks append/cached reads of the same unquantized current group.
    // The reference flush policy does not require identity to token-at-a-time compression.
    int failures               = compare_profile(label.c_str(), first_output, expected, 0.0);
    cudaStream_t stream        = nullptr;
    cudaGraph_t graph          = nullptr;
    cudaGraphExec_t executable = nullptr;
    cuda_check(cudaStreamCreate(&stream), "boundary stream");
    cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal), "boundary capture");
    body(stream);
    cuda_check(cudaStreamEndCapture(stream, &graph), "end boundary capture");
    cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
               "boundary instantiate");
    cuda_check(cudaGraphLaunch(executable, stream), "boundary replay");
    cuda_synchronize(stream);
    failures +=
        compare_profile(label.c_str(), from_device_bf16(output, query.size()), expected, 0.0);
    cudaGraphExecDestroy(executable);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);

    ops::kvarn_commit_pages(pt, at, rt, speculative.view(), nullptr);
    cuda_synchronize();
    // Rejection may leave a tentative record, but completing the page with replacement tokens
    // must encode the accepted BF16 prefix, not reuse that rejected record.
    CacheFixture<Heads, Pages> replay;
    append_cache(replay, prefix_k, prefix_v, 0, false);
    const std::size_t kept = static_cast<std::size_t>(accepted) * Heads * kD;
    if (accepted > 0) {
        append_cache(replay, {key.begin(), key.begin() + kept},
                     {value.begin(), value.begin() + kept}, first, false);
    }
    const int frontier       = first + accepted;
    const int replacements   = kGroup - frontier % kGroup;
    const auto replacement_k = make_cache_values(replacements, 0xd006U, Heads);
    const auto replacement_v = make_cache_values(replacements, 0xd007U, Heads);
    append_cache(replay, replacement_k, replacement_v, frontier, false);
    append_cache(speculative, replacement_k, replacement_v, frontier, false);
    const auto expected_records = from_device<std::uint8_t>(replay.records, replay.records.bytes);
    const auto actual_records =
        from_device<std::uint8_t>(speculative.records, speculative.records.bytes);
    const std::size_t committed_bytes =
        static_cast<std::size_t>(frontier + replacements) / kGroup * Heads * kRecordBytes;
    failures += verify_exact(
        "KVarN rejected-page re-encode",
        std::vector<std::uint8_t>(actual_records.begin(), actual_records.begin() + committed_bytes),
        std::vector<std::uint8_t>(expected_records.begin(),
                                  expected_records.begin() + committed_bytes));
    return failures;
}

template <int Heads, int Layers>
int run_publication_settlement_case() {
    constexpr int First             = 2 * kGroup - 2;
    const std::size_t prefix_values = static_cast<std::size_t>(First) * Heads * kD;
    int failures                    = 0;
    for (const int committed : {1, 2, 4}) {
        std::array<CacheFixture<Heads>, Layers> caches;
        std::array<ops::KvarnPagedLayerView, Layers> views;
        std::array<std::vector<float>, Layers> keys, values;
        for (int layer = 0; layer < Layers; ++layer) {
            auto& cache = caches[layer];
            auto& key = keys[layer] = make_cache_values(First + 6, 0xface01 + layer * 2, Heads);
            auto& value = values[layer] = make_cache_values(First + 6, 0xface02 + layer * 2, Heads);
            // Exact FP32 Hadamard sums let the FP64 oracle check retained BF16 bits directly.
            for (float& x : key) { x = std::round(x * 128.0F) / 128.0F; }
            for (float& x : value) { x = std::round(x * 128.0F) / 128.0F; }
            append_cache(cache, {key.begin(), key.begin() + prefix_values},
                         {value.begin(), value.begin() + prefix_values}, 0, false);
            append_cache(cache, {key.begin() + prefix_values, key.end()},
                         {value.begin() + prefix_values, value.end()}, First, true);
            views[layer] = cache.layer_view();
        }
        const int frontier = First + committed;
        if constexpr (Layers > 1) {
            cudaStream_t stream        = nullptr;
            cudaGraph_t graph          = nullptr;
            cudaGraphExec_t executable = nullptr;
            cuda_check(cudaStreamCreate(&stream), "settlement stream");
            cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
                       "settlement capture");
            ops::kvarn_restore_tail(frontier, views, stream);
            cuda_check(cudaStreamEndCapture(stream, &graph), "end settlement capture");
            cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
                       "settlement instantiate");
            cuda_check(cudaGraphLaunch(executable, stream), "settlement replay");
            cuda_synchronize(stream);
            cudaGraphExecDestroy(executable);
            cudaGraphDestroy(graph);
            cudaStreamDestroy(stream);
        } else {
            ops::kvarn_restore_tail(frontier, views, nullptr);
        }
        cuda_synchronize();
        for (int layer = 0; layer < Layers; ++layer) {
            auto& cache         = caches[layer];
            const auto& key     = keys[layer];
            const auto& value   = values[layer];
            const auto markers  = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
            const auto actual_k = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
            const auto actual_v = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
            const int page      = frontier / kGroup;
            const auto marker   = std::find(markers.begin(), markers.end(), page);
            if (frontier % kGroup != 0 && marker == markers.end()) {
                std::cerr << "KVarN publication lost the partial tail\n";
                return 1;
            }
            const int slot = static_cast<int>(marker - markers.begin());
            for (int position = page * kGroup; position < frontier; ++position) {
                for (int head = 0; head < Heads; ++head) {
                    for (int d = 0; d < kD; ++d) {
                        double k = 0, v = 0;
                        for (int col = 0; col < kD; ++col) {
                            const int sign =
                                (std::popcount(static_cast<unsigned>(d & col)) & 1) ? -1 : 1;
                            const std::size_t source = col + kD * (head + Heads * position);
                            k += sign * key[source];
                            v += sign * value[source];
                        }
                        const std::size_t destination =
                            d + kD * (position % kGroup + kGroup * (head + Heads * slot));
                        if (actual_k[destination] != f32_to_bf16(static_cast<float>(k / 16.0)) ||
                            actual_v[destination] != f32_to_bf16(static_cast<float>(v / 16.0))) {
                            std::cerr << "KVarN publication tail mismatch layer=" << layer
                                      << " committed=" << committed << " position=" << position
                                      << " head=" << head << " d=" << d << '\n';
                            return 1;
                        }
                    }
                }
            }
            CacheFixture<Heads> expected;
            const std::size_t kept = static_cast<std::size_t>(frontier) * Heads * kD;
            append_cache(expected, {key.begin(), key.begin() + kept},
                         {value.begin(), value.begin() + kept}, 0, false);
            failures +=
                verify_exact("KVarN final-publication records",
                             from_device<std::uint8_t>(cache.records, cache.records.bytes),
                             from_device<std::uint8_t>(expected.records, expected.records.bytes));
        }
        if (committed >= 2) {
            const std::vector<std::int32_t> packed_markers(ops::kKvarnTailSlots, -1);
            ops::kvarn_restore_tail(150, views, nullptr);
            cuda_synchronize();
            for (auto& cache : caches) {
                const auto records = from_device<std::uint8_t>(cache.records, cache.records.bytes);
                const auto k = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
                const auto v = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
                failures +=
                    verify_exact("KVarN batched historical markers",
                                 from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots),
                                 std::vector<std::int32_t>{0, 1, -1});
                for (int head = 0; head < Heads; ++head) {
                    for (int token = 0; token < kGroup; ++token) {
                        for (int d = 0; d < kD; ++d) {
                            const auto index = d + kD * (token + kGroup * (head + Heads));
                            const auto ek    = f32_to_bf16(decode_cache_value(
                                records, {}, packed_markers, true, kGroup + token, head, Heads, d));
                            const auto ev =
                                f32_to_bf16(decode_cache_value(records, {}, packed_markers, false,
                                                               kGroup + token, head, Heads, d));
                            if (k[index] != ek || v[index] != ev) {
                                std::cerr << "KVarN batched historical tail mismatch\n";
                                return failures + 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return failures;
}

template <int Heads, int QueryHeads>
int run_append_attention_oracle(int first, int width, bool final_query) {
    CacheFixture<Heads, 8> cache;
    if (first != 0) {
        append_cache(cache, make_cache_values(first, 0xf001, Heads),
                     make_cache_values(first, 0xf002, Heads), 0, false);
    }
    const auto records = from_device<std::uint8_t>(cache.records, cache.records.bytes);
    const auto tk      = from_device<std::uint16_t>(cache.tail_k, cache.tail_k.bytes / 2);
    const auto tv      = from_device<std::uint16_t>(cache.tail_v, cache.tail_v.bytes / 2);
    const auto markers = from_device<std::int32_t>(cache.markers, ops::kKvarnTailSlots);
    const auto key     = make_cache_values(width, 0xf003, Heads);
    const auto value   = make_cache_values(width, 0xf004, Heads);
    const int queries  = final_query ? 1 : width;
    const auto query   = make_cache_values(queries * QueryHeads, 0xf005);
    const int context  = first + width;
    const auto rotate  = [](const std::vector<float>& x) {
        std::vector<double> y(x.size());
        for (std::size_t base = 0; base < x.size(); base += kD) {
            for (int d = 0; d < kD; ++d) {
                double sum = 0;
                for (int col = 0; col < kD; ++col) {
                    const int sign =
                        (std::popcount(static_cast<unsigned>(d & col)) & 1) ? -1 : 1;
                    sum += sign * x[base + col];
                }
                y[base + d] = sum / 16.0;
            }
        }
        return y;
    };
    const auto current_k = rotate(key), current_v = rotate(value), rotated_q = rotate(query);
    std::vector<double> represented_k(static_cast<std::size_t>(context) * Heads * kD);
    std::vector<double> represented_v(represented_k.size());
    for (int position = 0; position < first; ++position) {
        for (int head = 0; head < Heads; ++head) {
            for (int d = 0; d < kD; ++d) {
                const auto index = static_cast<std::size_t>(d) + kD * (head + Heads * position);
                represented_k[index] =
                    decode_cache_value(records, tk, markers, true, position, head, Heads, d);
                represented_v[index] =
                    decode_cache_value(records, tv, markers, false, position, head, Heads, d);
            }
        }
    }
    std::copy(current_k.begin(), current_k.end(),
              represented_k.begin() + static_cast<std::size_t>(first) * Heads * kD);
    std::copy(current_v.begin(), current_v.end(),
              represented_v.begin() + static_cast<std::size_t>(first) * Heads * kD);
    std::vector<int> selected{0};
    if (!final_query) { selected = {0, 1, width / 2, width - 1}; }
    std::vector<double> expected;
    for (const int column : selected) {
        const int visible = final_query ? context : first + column + 1;
        for (int head = 0; head < QueryHeads; ++head) {
            const int kvhead = head / (QueryHeads / Heads);
            std::vector<double> score(visible);
            for (int p = 0; p < visible; ++p) {
                for (int d = 0; d < kD; ++d) {
                    score[p] += rotated_q[d + kD * (head + QueryHeads * column)] *
                                represented_k[d + kD * (kvhead + Heads * p)] / 16.0;
                }
            }
            const double maximum = *std::max_element(score.begin(), score.end());
            double denominator   = 0;
            std::vector<double> numerator(kD, 0);
            for (int p = 0; p < visible; ++p) {
                const double probability = std::exp(score[p] - maximum);
                denominator += probability;
                for (int d = 0; d < kD; ++d) {
                    numerator[d] += probability * represented_v[d + kD * (kvhead + Heads * p)];
                }
            }
            for (int d = 0; d < kD; ++d) {
                double sum = 0;
                for (int col = 0; col < kD; ++col) {
                    const int sign =
                        (std::popcount(static_cast<unsigned>(d & col)) & 1) ? -1 : 1;
                    sum += sign * numerator[col];
                }
                expected.push_back(sum / (16.0 * denominator));
            }
        }
    }
    auto q = to_device_bf16(query), k = to_device_bf16(key), v = to_device_bf16(value);
    auto oq = to_device_bf16(query), ok = to_device_bf16(key), ov = to_device_bf16(value);
    auto old_records = to_device(records), old_tk = to_device(tk), old_tv = to_device(tv),
         old_markers = to_device(markers);
    std::vector<std::int32_t> positions(width);
    std::iota(positions.begin(), positions.end(), first);
    auto dp = to_device(positions), rows = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer output(query.size() * 2);
    Tensor qt(q.p, DType::BF16, {kD, QueryHeads, queries, 1});
    Tensor kt(k.p, DType::BF16, {kD, Heads, width, 1}), vt(v.p, DType::BF16, {kD, Heads, width, 1});
    Tensor pt(dp.p, DType::I32, {width, 1}), rt(rows.p, DType::I32, {1});
    Tensor ot(output.p, DType::BF16, {kD, QueryHeads, queries, 1});
    const ops::CausalAttentionExecutionEnvelope envelope{1, static_cast<std::uint32_t>(context)};
    WorkspaceArena workspace(
        ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, queries, queries));
    const auto body = [&](cudaStream_t stream) {
        for (auto pair : {std::pair{&q, &oq},
                          {&k, &ok},
                          {&v, &ov},
                          {&cache.records, &old_records},
                          {&cache.tail_k, &old_tk},
                          {&cache.tail_v, &old_tv},
                          {&cache.markers, &old_markers}}) {
            cuda_check(cudaMemcpyAsync(pair.first->p, pair.second->p, pair.first->bytes,
                                       cudaMemcpyDeviceToDevice, stream),
                       "restore append oracle inputs");
        }
        ops::kvarn_attention(qt, kt, vt, pt, Tensor{}, rt, 0.0625F, cache.view(), false, envelope,
                             workspace, ot, stream);
    };
    const auto compare = [&] {
        auto full = from_device_bf16(output, query.size());
        if (!std::all_of(full.begin(), full.end(), [](double x) { return std::isfinite(x); })) {
            std::cerr << "KVarN append produced non-finite output\n";
            return 1;
        }
        std::vector<double> actual;
        for (int column : selected) {
            actual.insert(actual.end(),
                          full.begin() + static_cast<std::size_t>(column) * QueryHeads * kD,
                          full.begin() + static_cast<std::size_t>(column + 1) * QueryHeads * kD);
        }
        return compare_profile("KVarN unquantized-current-chunk oracle", actual, expected, 8.0e-3);
    };
    body(nullptr);
    cuda_synchronize();
    int failures               = compare();
    cudaStream_t stream        = nullptr;
    cudaGraph_t graph          = nullptr;
    cudaGraphExec_t executable = nullptr;
    cuda_check(cudaStreamCreate(&stream), "append oracle stream");
    cuda_check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal),
               "append oracle capture");
    body(stream);
    cuda_check(cudaStreamEndCapture(stream, &graph), "append oracle end capture");
    cuda_check(cudaGraphInstantiate(&executable, graph, nullptr, nullptr, 0),
               "append oracle instantiate");
    cuda_check(cudaGraphLaunch(executable, stream), "append oracle replay");
    cuda_synchronize(stream);
    failures += compare();
    cudaGraphExecDestroy(executable);
    cudaGraphDestroy(graph);
    cudaStreamDestroy(stream);
    return failures;
}

// ---------------------------------------------------------------------------
// WP6.1 exact tail partial (route (a)): the KVarN body's rotated-domain tail partial reads the
// shared exact ring and produces (acc, m, l) at the split indices the quantized body leaves free,
// applying W to its FP32 accumulator exactly once. Qualified against an independent FP64 oracle
// over the same BF16 rows.
// ---------------------------------------------------------------------------

// Host mirror of small_t.cuh's default split tier, which is what the KVarN body's
// `kvarn_decode_active_splits` falls back to below window 8198. Independent of the device code.
int host_small_t_default_splits(int window, int split_scale) {
    int target = 480 / split_scale;
    if (window <= 4096) {
        target = 64 / split_scale;
    } else if (window <= 8198) {
        target = 128 / split_scale;
    } else if (window <= 16390) {
        target = 256 / split_scale;
    }
    const int minimum = 4 * split_scale;
    int splits        = (window + target - 1) / target;
    if (splits < minimum) { splits = minimum; }
    const int maximum = 85 * split_scale;
    return splits < maximum ? splits : maximum;
}

struct HostTailPartition {
    int body_window;
    int body_active;
    int tail_active;
};

HostTailPartition host_tail_partition(int window, int tail_tokens, int launch_capacity,
                                      int split_scale) {
    const auto active = [&](int w) {
        if (w <= 0) { return launch_capacity; }
        const int splits = host_small_t_default_splits(w, split_scale);
        return splits < launch_capacity ? splits : launch_capacity;
    };
    const int total      = active(window);
    const int tail_keys  = tail_tokens > 0 ? std::min(tail_tokens, window) : 0;
    const int body_window = window - tail_keys;
    int body_active       = body_window > 0 ? active(body_window) : 0;
    if (body_window > 0 || tail_keys > 0) {
        const int limit = tail_keys > 0 ? total - 1 : total;
        if (body_active > limit) { body_active = limit; }
        if (body_active < 1) { body_active = 1; }
    }
    return HostTailPartition{body_window, body_active, total - body_active};
}

// WP6.4: the KVarN body's own split policy is not the small-T default tier above 8198 keys. Mirror of
// `detail::kvarn_decode_active_splits` (decode_kernel.cuh) at 24 query heads, which is the geometry
// this suite runs: above 8198 it takes div_up(window, 192) splits, capped at 41 up to 122880 keys and
// at 82 beyond it; at or below 8198 it is `causal_small_t_default_splits` at split scale 1, which
// `host_small_t_default_splits` already models. The host mirror of the partition below used to cover
// only the small-T tier, so no case here could exercise the long branch at all.
int host_kvarn_active_splits(int window, int launch_capacity) {
    if (window > 8198) {
        int splits    = (window + 191) / 192;
        const int cap = window <= 122880 ? 41 : 82;
        splits        = std::min(splits, cap);
        return std::min(splits, launch_capacity);
    }
    return std::min(host_small_t_default_splits(window, 1), launch_capacity);
}

HostTailPartition host_kvarn_tail_partition(int window, int tail_tokens, int launch_capacity) {
    const auto active = [&](int w) {
        if (w <= 0) { return launch_capacity; }
        return host_kvarn_active_splits(w, launch_capacity);
    };
    const int total       = active(window);
    const int tail_keys   = tail_tokens > 0 ? std::min(tail_tokens, window) : 0;
    const int body_window = window - tail_keys;
    int body_active       = body_window > 0 ? active(body_window) : 0;
    if (body_window > 0 || tail_keys > 0) {
        const int limit = tail_keys > 0 ? total - 1 : total;
        if (body_active > limit) { body_active = limit; }
        if (body_active < 1) { body_active = 1; }
    }
    return HostTailPartition{body_window, body_active, total - body_active};
}

// f16 bits -> float, so the F16-ring oracle reads exactly the values the kernel's `__half` reads.
inline float f16_bits_to_f32(std::uint16_t h) {
    const std::uint32_t sign = static_cast<std::uint32_t>(h & 0x8000U) << 16;
    std::uint32_t exponent   = (h >> 10) & 0x1FU;
    std::uint32_t mantissa   = h & 0x3FFU;
    std::uint32_t bits       = 0;
    if (exponent == 0) {
        if (mantissa == 0) {
            bits = sign;
        } else {
            int e = 127 - 15 + 1;
            while ((mantissa & 0x400U) == 0) {
                mantissa <<= 1;
                --e;
            }
            bits = sign | (static_cast<std::uint32_t>(e) << 23) | ((mantissa & 0x3FFU) << 13);
        }
    } else if (exponent == 0x1FU) {
        bits = sign | 0x7F800000U | (mantissa << 13);
    } else {
        bits = sign | ((exponent - 15 + 127) << 23) | (mantissa << 13);
    }
    float value = 0.0F;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

void round_to_f16(std::vector<float>& value) {
    for (float& x : value) { x = f16_bits_to_f32(f32_to_f16(x)); }
}

// The D256 Sylvester-Hadamard exactly as ops::kvarn::detail::hadamard_warp applies it (same
// butterfly, same 2^-4 scale), in double so the oracle is the independent exact evaluation.
void host_hadamard_d256(std::vector<double>& value) {
    std::vector<double> next(value.size(), 0.0);
    for (int span = 1; span < kD; span <<= 1) {
        for (int i = 0; i < kD; ++i) {
            const int low  = i & ~span;
            const int high = low + span;
            if (i & span) {
                next[i] = value[low] - value[high];
            } else {
                next[i] = value[low] + value[high];
            }
        }
        value.swap(next);
    }
    for (double& v : value) { v *= 0.0625; }
}

// `kvarn_hadamard` as the Op applies it to a query: W over each contiguous D256 vector, written back
// as BF16. The tail partial un-rotates whatever query it is handed, so a direct-kernel test has to
// hand it the same rotated domain the Op does.
std::vector<float> rotate_query_like_op(const std::vector<float>& host_query) {
    std::vector<float> out = host_query;
    for (std::size_t base = 0; base + kD <= out.size(); base += kD) {
        std::vector<double> vector(out.begin() + static_cast<std::ptrdiff_t>(base),
                                   out.begin() + static_cast<std::ptrdiff_t>(base + kD));
        host_hadamard_d256(vector);
        for (int d = 0; d < kD; ++d) { out[base + d] = static_cast<float>(vector[d]); }
    }
    round_to_bf16(out);
    return out;
}

// The exact-tail kernel recovers its query by un-rotating (with `detail::hadamard_warp`) the bf16
// rotated query the Op hands over. `hadamard_warp` is the same butterfly/scale `host_hadamard_d256`
// models, so the oracle's query is that un-rotation of the very bf16 values the kernel reads -- the
// residual is then only float (kernel) versus double (oracle) rounding, not the bf16 round-trip.
std::vector<double> unrotate_query_like_kernel(const std::vector<float>& rotated_query) {
    std::vector<double> out(rotated_query.begin(), rotated_query.end());
    for (std::size_t base = 0; base + kD <= out.size(); base += kD) {
        std::vector<double> vector(out.begin() + static_cast<std::ptrdiff_t>(base),
                                   out.begin() + static_cast<std::ptrdiff_t>(base + kD));
        host_hadamard_d256(vector);
        for (int d = 0; d < kD; ++d) { out[base + d] = vector[d]; }
    }
    return out;
}

template <int QueryHeads, int KVHeads, int SplitScale>
int run_exact_tail_partial_case(int window, int tail_tokens, int launch_capacity,
                                const std::string& label, bool f16_tail = false,
                                bool kvarn_long_tier = false, int ring_pages_override = 0) {
    using Geometry = ops::CausalAttentionGeometry<QueryHeads, KVHeads, SplitScale>;
    constexpr int kGroupSize = Geometry::GroupSize;
    // The production ring's page count follows the retention, not the window: the allocator hands the
    // tail ceil(N / 64) + 1 pages, so the ring holds the newest N keys and the tail interval cannot
    // alias two positions onto one slot. The default keeps the whole window in the ring, which is
    // what the small-window cases want; `ring_pages_override` sets the production geometry so a
    // window far above `N` still exercises the real page walk (and, for N a multiple of the wrap, a
    // full turn of the ring).
    const int ring_pages = ring_pages_override > 0 ? ring_pages_override : (window + 63) / 64;

    const HostTailPartition partition =
        kvarn_long_tier ? host_kvarn_tail_partition(window, tail_tokens, launch_capacity)
                        : host_tail_partition(window, tail_tokens, launch_capacity, SplitScale);

    // Exact ring: one sequence, positions [0, window) written at their ring slot. The rows are
    // BF16 and are the ONLY source both the kernel and the oracle read, so input rounding cancels.
    const std::size_t ring_values =
        static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages;
    std::vector<float> host_tail_k(ring_values, 0.0F), host_tail_v(ring_values, 0.0F);
    auto ring_index = [&](int position, int kv_head, int d) {
        const int page = (position / 64) % ring_pages;
        const int off  = position % 64;
        return static_cast<std::size_t>(kD) * (64 * (kv_head + KVHeads * page) + off) + d;
    };
    const int ring_capacity = 64 * ring_pages;
    const int ring_begin    = std::max(0, window - ring_capacity);
    for (int position = ring_begin; position < window; ++position) {
        for (int kv_head = 0; kv_head < KVHeads; ++kv_head) {
            for (int d = 0; d < kD; ++d) {
                const float k = 0.5F * std::sin(0.017F * (position + 1) * (d + 1) + kv_head);
                const float v = 0.5F * std::cos(0.013F * (position + 1) * (d + 3) - kv_head);
                host_tail_k[ring_index(position, kv_head, d)] = k;
                host_tail_v[ring_index(position, kv_head, d)] = v;
            }
        }
    }
    round_to_bf16(host_tail_k);
    round_to_bf16(host_tail_v);
    if (f16_tail) {
        round_to_f16(host_tail_k);
        round_to_f16(host_tail_v);
    }

    std::vector<float> host_query(static_cast<std::size_t>(kD) * QueryHeads, 0.0F);
    for (int q_head = 0; q_head < QueryHeads; ++q_head) {
        for (int d = 0; d < kD; ++d) {
            host_query[static_cast<std::size_t>(q_head) * kD + d] =
                0.5F * std::sin(0.011F * (q_head + 1) * (d + 5));
        }
    }
    round_to_bf16(host_query);

    // The Op rotates the query in place before the tail partial runs (the body reads the rotated
    // query), so the kernel is handed a rotated query and un-rotates it. The oracle evaluates the
    // same recovered query, so it is unchanged by the round-trip.
    std::vector<float> pipeline_query = rotate_query_like_op(host_query);
    const std::vector<double> oracle_query = unrotate_query_like_kernel(pipeline_query);
    DeviceBuffer device_query = to_device_bf16(pipeline_query);
    DeviceBuffer tail_k       = f16_tail
                                    ? to_device(std::vector<std::uint16_t>(
                                          [&] {
                                              std::vector<std::uint16_t> bits(host_tail_k.size());
                                              for (std::size_t i = 0; i < bits.size(); ++i) {
                                                  bits[i] = f32_to_f16(host_tail_k[i]);
                                              }
                                              return bits;
                                          }()))
                                    : to_device_bf16(host_tail_k);
    DeviceBuffer tail_v       = f16_tail
                                    ? to_device(std::vector<std::uint16_t>(
                                          [&] {
                                              std::vector<std::uint16_t> bits(host_tail_v.size());
                                              for (std::size_t i = 0; i < bits.size(); ++i) {
                                                  bits[i] = f32_to_f16(host_tail_v[i]);
                                              }
                                              return bits;
                                          }()))
                                    : to_device_bf16(host_tail_v);
    DeviceBuffer positions =
        to_device(std::vector<std::int32_t>{static_cast<std::int32_t>(window - 1)});

    const std::size_t acc_values = static_cast<std::size_t>(kD) * QueryHeads * launch_capacity;
    const std::size_t stat_values = static_cast<std::size_t>(QueryHeads) * launch_capacity;
    // Sentinels: an unwritten split must stay a neutral partial (m == -inf, l == 0, acc == 0) so the
    // merge below can run over every split without special cases, and so the N == 0 case can prove
    // the kernel wrote nothing.
    const float kUnwritten = -12345.0F;
    std::vector<float> host_acc(acc_values, kUnwritten);
    std::vector<float> host_m(stat_values, kUnwritten);
    std::vector<float> host_l(stat_values, kUnwritten);
    DeviceBuffer device_acc  = to_device_f32(host_acc);
    DeviceBuffer device_m    = to_device_f32(host_m);
    DeviceBuffer device_l    = to_device_f32(host_l);

    Tensor query_tensor(device_query.p, DType::BF16, {kD, QueryHeads, 1, 1});
    Tensor position_tensor(positions.p, DType::I32, {1, 1});
    Tensor acc_tensor(device_acc.p, DType::FP32, {kD, QueryHeads, 1, launch_capacity});
    Tensor m_tensor(device_m.p, DType::FP32, {QueryHeads, 1, launch_capacity});
    Tensor l_tensor(device_l.p, DType::FP32, {QueryHeads, 1, launch_capacity});
    Tensor tail_k_tensor(tail_k.p, f16_tail ? DType::FP16 : DType::BF16, {kD, 64, KVHeads, ring_pages});
    Tensor tail_v_tensor(tail_v.p, f16_tail ? DType::FP16 : DType::BF16, {kD, 64, KVHeads, ring_pages});

    ops::kvarn::exact_tail_partial(query_tensor, position_tensor, Tensor{}, tail_k_tensor,
                                   tail_v_tensor, ring_pages, tail_tokens, launch_capacity,
                                   0 /*column_begin*/, 1 /*width*/, window /*logical_capacity*/,
                                   1 /*batch_size*/, 0.0625F, acc_tensor, m_tensor, l_tensor,
                                   nullptr);
    cuda_synchronize();

    int failures = 0;

    if (tail_tokens <= 0) {
        // N == 0 must not trigger the path at all: every byte stays the sentinel.
        const std::vector<double> acc = from_device_f32(device_acc, acc_values);
        const std::vector<double> ms  = from_device_f32(device_m, stat_values);
        const bool untouched =
            std::all_of(acc.begin(), acc.end(), [](double x) { return x == kUnwritten; }) &&
            std::all_of(ms.begin(), ms.end(), [](double x) { return x == kUnwritten; });
        std::cout << label
                  << ": tail_tokens=0 leaves the partial untouched=" << (untouched ? "yes" : "NO")
                  << "\n";
        if (!untouched) { ++failures; }
        return failures;
    }

    const std::vector<double> acc = from_device_f32(device_acc, acc_values);
    const std::vector<double> m   = from_device_f32(device_m, stat_values);
    const std::vector<double> l   = from_device_f32(device_l, stat_values);
    // partial_stat_index(q_head, token=0, split) = q_head + QHeads * split
    const auto stat_at = [&](int q_head, int split) {
        return static_cast<std::size_t>(split) * QueryHeads + q_head;
    };
    // partial_acc_index(q_head, d, token=0, split) = d + D * (q_head + QHeads * split)
    const auto acc_at = [&](int q_head, int d, int split) {
        return (static_cast<std::size_t>(split) * QueryHeads + q_head) * kD + d;
    };

    // Merge the tail splits the kernel published, then un-rotate, and compare against the FP64
    // oracle of the exact attention over the keys [body_window, window).
    int neutral_splits_written = 0;
    double worst_rel           = 0.0;
    for (int q_head = 0; q_head < QueryHeads; ++q_head) {
        const int kv_head = q_head / kGroupSize;
        // The kernel must have written exactly the tail splits [body_active, total_active).
        const int total_active = partition.body_active + partition.tail_active;
        for (int split = 0; split < launch_capacity; ++split) {
            const bool is_tail = split >= partition.body_active && split < total_active;
            const float got_m  = m[stat_at(q_head, split)];
            const bool written = got_m != kUnwritten;
            if (written != is_tail) {
                std::cout << label << ": split " << split << " head " << q_head
                          << " written=" << written << " expected_tail=" << is_tail << "\n";
                ++failures;
            }
            if (is_tail && got_m == -std::numeric_limits<float>::infinity()) {
                ++neutral_splits_written;
            }
        }

        // Merge (fp64) over the splits the kernel actually wrote.
        double merged_m = -std::numeric_limits<double>::infinity();
        for (int split = 0; split < launch_capacity; ++split) {
            if (l[stat_at(q_head, split)] > 0.0F) {
                merged_m = std::max(merged_m, static_cast<double>(m[stat_at(q_head, split)]));
            }
        }
        std::vector<double> merged_acc(kD, 0.0);
        double merged_l = 0.0;
        for (int split = 0; split < launch_capacity; ++split) {
            const double ls = l[stat_at(q_head, split)];
            if (!(ls > 0.0) || m[stat_at(q_head, split)] == kUnwritten) { continue; }
            const double w = std::exp(static_cast<double>(m[stat_at(q_head, split)]) - merged_m);
            merged_l += ls * w;
            for (int d = 0; d < kD; ++d) {
                merged_acc[d] += static_cast<double>(acc[acc_at(q_head, d, split)]) * w;
            }
        }
        std::vector<double> merged = merged_acc;
        host_hadamard_d256(merged);
        for (int d = 0; d < kD; ++d) { merged[d] /= merged_l; }

        // Oracle: exact attention over the tail keys in the original domain. The engine's reduce
        // un-rotates the merged accumulator once, so the observable output is A / l in the original
        // domain -- no W on the oracle side.
        std::vector<double> oracle_acc(kD, 0.0);
        double oracle_m = -std::numeric_limits<double>::infinity();
        std::vector<double> scores;
        for (int key = partition.body_window; key < window; ++key) {
            double dot = 0.0;
            const std::size_t base = ring_index(key, kv_head, 0);
            for (int d = 0; d < kD; ++d) {
                dot += oracle_query[static_cast<std::size_t>(q_head) * kD + d] *
                       static_cast<double>(host_tail_k[base + d]);
            }
            scores.push_back(dot * 0.0625);
            oracle_m = std::max(oracle_m, scores.back());
        }
        double oracle_l = 0.0;
        for (std::size_t i = 0; i < scores.size(); ++i) {
            const double p = std::exp(scores[i] - oracle_m);
            oracle_l += p;
            const std::size_t base = ring_index(partition.body_window + static_cast<int>(i),
                                                kv_head, 0);
            for (int d = 0; d < kD; ++d) {
                oracle_acc[d] += p * static_cast<double>(host_tail_v[base + d]);
            }
        }
        std::vector<double> oracle = oracle_acc;
        double oracle_scale = 0.0;
        for (int d = 0; d < kD; ++d) {
            oracle[d] /= oracle_l;
            oracle_scale = std::max(oracle_scale, std::fabs(oracle[d]));
        }

        for (int d = 0; d < kD; ++d) {
            const double rel = std::fabs(merged[d] - oracle[d]) / (oracle_scale + 1.0e-9);
            worst_rel        = std::max(worst_rel, rel);
        }
    }

    const double kTailTolerance = 2.0e-3;
    // Every key the tail reads has to be inside the ring the oracle filled; a shorter ring is a
    // fixture error, not a kernel result.
    if (partition.body_window < ring_begin) {
        std::cout << label << ": tail starts at " << partition.body_window
                  << " but the ring only holds [" << ring_begin << ", " << window << ")\n";
        ++failures;
    }
    std::cout << label << ": window=" << window << " tail=" << tail_tokens
              << " ring_pages=" << ring_pages
              << " body_active=" << partition.body_active
              << " tail_active=" << partition.tail_active
              << " neutral_tail_splits=" << neutral_splits_written
              << " max_rel_vs_fp64=" << std::scientific << std::setprecision(3) << worst_rel
              << (worst_rel <= kTailTolerance ? "  OK\n" : "  FAIL\n");
    if (worst_rel > kTailTolerance) { ++failures; }
    return failures;
}

// WP6.3: the KVarN append writes the shared exact ring itself (`stage_exact_tail`), because the
// KVarN body owns its quantized write and never calls ops::kv_cache_append. The ring must hold
// exactly the rows the addressing says -- this batch row's slot `(position / 64) % ring_pages` --
// converted to the ring's element type, and must drop the rows a launch wider than the ring cannot
// place without aliasing two positions onto one slot.
int run_exact_tail_stage_case(int first, int width, int ring_pages, int batch, bool masked,
                              bool f16_ring, const std::string& label) {
    constexpr int KVHeads   = 4;
    const int ring_capacity = 64 * ring_pages;
    const auto kv_index     = [&](int d, int head, int token, int b) {
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (head + static_cast<std::size_t>(KVHeads) *
                               (token + static_cast<std::size_t>(width) * b));
    };
    const auto ring_index = [&](int b, int position, int head, int d) {
        const int ring = b * ring_pages + (position / 64) % ring_pages;
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (64 * (head + static_cast<std::size_t>(KVHeads) * ring) + position % 64);
    };

    std::vector<float> host_k(static_cast<std::size_t>(kD) * KVHeads * width * batch, 0.0F);
    std::vector<float> host_v(host_k.size(), 0.0F);
    std::vector<std::int32_t> host_positions(static_cast<std::size_t>(width) * batch, 0);
    std::vector<std::int32_t> host_valid(batch, width);
    for (int b = 0; b < batch; ++b) {
        host_valid[b] = masked ? (width * (b + 1)) / (batch + 1) : width;
        for (int token = 0; token < width; ++token) {
            host_positions[static_cast<std::size_t>(token) + static_cast<std::size_t>(width) * b] =
                first + token;
            for (int head = 0; head < KVHeads; ++head) {
                for (int d = 0; d < kD; ++d) {
                    host_k[kv_index(d, head, token, b)] =
                        0.25F * std::sin(0.011F * (d + 1) * (token + 3) + 0.5F * (head + b));
                    host_v[kv_index(d, head, token, b)] =
                        0.25F * std::cos(0.009F * (d + 2) * (token + 5) - 0.5F * (head + b));
                }
            }
        }
    }
    round_to_bf16(host_k);
    round_to_bf16(host_v);

    const std::size_t ring_values =
        static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages * batch;
    std::vector<std::uint16_t> expected_k(ring_values, 0);
    std::vector<std::uint16_t> expected_v(ring_values, 0);
    for (int b = 0; b < batch; ++b) {
        const int newest = first + host_valid[b] - 1;
        for (int token = 0; token < host_valid[b]; ++token) {
            const int position = first + token;
            if (newest - position >= ring_capacity) { continue; }
            for (int head = 0; head < KVHeads; ++head) {
                for (int d = 0; d < kD; ++d) {
                    const std::size_t slot = ring_index(b, position, head, d);
                    const float k          = host_k[kv_index(d, head, token, b)];
                    const float v          = host_v[kv_index(d, head, token, b)];
                    expected_k[slot]        = f16_ring ? f32_to_f16(k) : f32_to_bf16(k);
                    expected_v[slot]        = f16_ring ? f32_to_f16(v) : f32_to_bf16(v);
                }
            }
        }
    }

    DeviceBuffer device_k  = to_device_bf16(host_k);
    DeviceBuffer device_v  = to_device_bf16(host_v);
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer valid     = to_device(host_valid);
    DeviceBuffer ring_k    = to_device(std::vector<std::uint16_t>(ring_values, 0));
    DeviceBuffer ring_v    = to_device(std::vector<std::uint16_t>(ring_values, 0));

    const DType ring_dtype = f16_ring ? DType::FP16 : DType::BF16;
    Tensor key_tensor(device_k.p, DType::BF16, {kD, KVHeads, width, batch});
    Tensor value_tensor(device_v.p, DType::BF16, {kD, KVHeads, width, batch});
    Tensor position_tensor(positions.p, DType::I32, {width, batch});
    Tensor ring_k_tensor(ring_k.p, ring_dtype, {kD, 64, KVHeads, ring_pages * batch});
    Tensor ring_v_tensor(ring_v.p, ring_dtype, {kD, 64, KVHeads, ring_pages * batch});
    Tensor valid_tensor;
    if (masked) { valid_tensor = Tensor(valid.p, DType::I32, {batch}); }

    ops::kvarn::stage_exact_tail(key_tensor, value_tensor, position_tensor, valid_tensor, KVHeads,
                                 ring_k_tensor, ring_v_tensor, ring_pages, nullptr);
    cuda_synchronize();

    const auto got_k        = from_device<std::uint16_t>(ring_k, ring_values);
    const auto got_v        = from_device<std::uint16_t>(ring_v, ring_values);
    std::size_t first_bad_k = ring_values;
    std::size_t first_bad_v = ring_values;
    for (std::size_t i = 0; i < ring_values; ++i) {
        if (got_k[i] != expected_k[i] && first_bad_k == ring_values) { first_bad_k = i; }
        if (got_v[i] != expected_v[i] && first_bad_v == ring_values) { first_bad_v = i; }
    }
    int failures = 0;
    if (first_bad_k != ring_values || first_bad_v != ring_values) {
        std::cerr << label << ": exact-ring mismatch at k index " << first_bad_k << ", v index "
                  << first_bad_v << '\n';
        ++failures;
    }
    std::cout << label << ": first=" << first << " width=" << width << " ring_pages=" << ring_pages
              << " batch=" << batch << " masked=" << masked << " f16=" << f16_ring
              << (failures == 0 ? "  OK\n" : "  FAIL\n");
    return failures;
}

// WP6.3: the tail partial on a chunked, batched launch. `column_begin` and the per-sequence stride
// put each row's query and statistics at their own offset, each sequence brings its own window, and
// the launch-wide partition the tail uses is the one the body uses for that sequence -- so this
// checks the offsets and the partition placement, not only the arithmetic (which WP6.1 covers).
int run_exact_tail_partial_launch_case(int base0, int base1, int tail_tokens, int launch_capacity,
                                       const std::string& label) {
    constexpr int QueryHeads = 24;
    constexpr int KVHeads    = 4;
    constexpr int SplitScale = 1;
    using Geometry           = ops::CausalAttentionGeometry<QueryHeads, KVHeads, SplitScale>;
    constexpr int kGroupSize = Geometry::GroupSize;

    const int batch        = 2;
    const int full_width   = 3;
    const int column_begin = 1;
    const int width        = 2;
    const int host_base[2] = {base0, base1};
    const int longest      = (base0 > base1 ? base0 : base1) + column_begin + width;
    const int ring_pages   = (longest + 63) / 64;
    const std::size_t ring_values =
        static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages * batch;

    const auto ring_index = [&](int b, int position, int head, int d) {
        const int ring = b * ring_pages + (position / 64) % ring_pages;
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (64 * (head + static_cast<std::size_t>(KVHeads) * ring) + position % 64);
    };

    // Each sequence's live positions are its own: `pos[column_begin + t, b] = base[b] + column_begin
    // + t`, so sequence 1's window is a different tier from sequence 0's. K and V differ so a
    // swapped pair cannot hide.
    std::vector<float> host_ring_k(ring_values, 0.0F);
    std::vector<float> host_ring_v(ring_values, 0.0F);
    std::vector<std::int32_t> host_positions(static_cast<std::size_t>(full_width) * batch, 0);
    for (int b = 0; b < batch; ++b) {
        for (int column = 0; column < full_width; ++column) {
            host_positions[static_cast<std::size_t>(column) + static_cast<std::size_t>(full_width) *
                                                                b] = host_base[b] + column;
        }
        const int sequence_window = host_base[b] + column_begin + width;
        for (int position = 0; position < sequence_window; ++position) {
            for (int head = 0; head < KVHeads; ++head) {
                for (int d = 0; d < kD; ++d) {
                    const std::size_t slot = ring_index(b, position, head, d);
                    host_ring_k[slot] =
                        0.4F * std::sin(0.017F * (position + 1) * (d + 1) + head + b);
                    host_ring_v[slot] =
                        0.4F * std::cos(0.013F * (position + 1) * (d + 3) - head + b);
                }
            }
        }
    }
    round_to_bf16(host_ring_k);
    round_to_bf16(host_ring_v);

    std::vector<float> host_query(static_cast<std::size_t>(kD) * QueryHeads * full_width * batch,
                                  0.0F);
    const auto q_index = [&](int q_head, int d, int column, int b) {
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (q_head + static_cast<std::size_t>(QueryHeads) *
                                 (column + static_cast<std::size_t>(full_width) * b));
    };
    for (int b = 0; b < batch; ++b) {
        for (int column = 0; column < full_width; ++column) {
            for (int q_head = 0; q_head < QueryHeads; ++q_head) {
                for (int d = 0; d < kD; ++d) {
                    host_query[q_index(q_head, d, column, b)] =
                        0.5F * std::sin(0.011F * (q_head + 1) * (d + 5) + 0.25F * (column + 2 * b));
                }
            }
        }
    }
    round_to_bf16(host_query);

    const std::size_t acc_values =
        static_cast<std::size_t>(kD) * QueryHeads * width * launch_capacity * batch;
    const std::size_t stat_values =
        static_cast<std::size_t>(QueryHeads) * width * launch_capacity * batch;
    const float kUnwritten = -12345.0F;
    DeviceBuffer acc = to_device_f32(std::vector<float>(acc_values, kUnwritten));
    DeviceBuffer m   = to_device_f32(std::vector<float>(stat_values, kUnwritten));
    DeviceBuffer l   = to_device_f32(std::vector<float>(stat_values, kUnwritten));
    // Rotated the way the Op hands it over; the oracle evaluates the kernel's own un-rotation of
    // these bf16 values (see `unrotate_query_like_kernel`).
    const std::vector<float> pipeline_query = rotate_query_like_op(host_query);
    const std::vector<double> oracle_query  = unrotate_query_like_kernel(pipeline_query);
    DeviceBuffer query    = to_device_bf16(pipeline_query);
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer ring_k    = to_device_bf16(host_ring_k);
    DeviceBuffer ring_v    = to_device_bf16(host_ring_v);

    Tensor query_tensor(query.p, DType::BF16, {kD, QueryHeads, full_width, batch});
    Tensor position_tensor(positions.p, DType::I32, {full_width, batch});
    Tensor ring_k_tensor(ring_k.p, DType::BF16, {kD, 64, KVHeads, ring_pages * batch});
    Tensor ring_v_tensor(ring_v.p, DType::BF16, {kD, 64, KVHeads, ring_pages * batch});
    Tensor acc_tensor(acc.p, DType::FP32, {kD, QueryHeads, width, launch_capacity * batch});
    Tensor m_tensor(m.p, DType::FP32, {QueryHeads, width, launch_capacity * batch});
    Tensor l_tensor(l.p, DType::FP32, {QueryHeads, width, launch_capacity * batch});

    ops::kvarn::exact_tail_partial(query_tensor, position_tensor, Tensor{}, ring_k_tensor,
                                   ring_v_tensor, ring_pages, tail_tokens, launch_capacity,
                                   column_begin, width, longest + 8, batch, 0.5F, acc_tensor,
                                   m_tensor, l_tensor, nullptr);
    cuda_synchronize();

    const auto host_acc = from_device_f32(acc, acc_values);
    const auto host_m   = from_device_f32(m, stat_values);
    const auto host_l   = from_device_f32(l, stat_values);
    const auto stat_at  = [&](int b, int q_head, int token, int split) {
        return static_cast<std::size_t>(b) * QueryHeads * width * launch_capacity +
               (static_cast<std::size_t>(split) * width + token) * QueryHeads + q_head;
    };
    const auto acc_at = [&](int b, int q_head, int d, int token, int split) {
        return static_cast<std::size_t>(b) * kD * QueryHeads * width * launch_capacity +
               static_cast<std::size_t>(kD) *
                   (q_head + QueryHeads * (token + static_cast<std::size_t>(width) * split)) +
               d;
    };

    int failures            = 0;
    double worst_rel        = 0.0;
    int rows_checked        = 0;
    const double kTolerance = 2.0e-3;
    for (int b = 0; b < batch; ++b) {
        const int sequence_window = host_base[b] + column_begin + width;
        const HostTailPartition partition =
            host_tail_partition(sequence_window, tail_tokens, launch_capacity, SplitScale);
        const int total_active = partition.body_active + partition.tail_active;
        for (int token = 0; token < width; ++token) {
            const int qabs = host_base[b] + column_begin + token;
            for (int q_head = 0; q_head < QueryHeads; ++q_head) {
                const int kv_head = q_head / kGroupSize;
                for (int split = 0; split < launch_capacity; ++split) {
                    const bool is_tail = split >= partition.body_active && split < total_active;
                    const bool written = host_m[stat_at(b, q_head, token, split)] != kUnwritten;
                    if (written != is_tail) {
                        std::cerr << label << ": batch " << b << " token " << token << " head "
                                  << q_head << " split " << split << " written=" << written
                                  << " expected_tail=" << is_tail << '\n';
                        ++failures;
                    }
                }
                // Merge the tail splits in FP64, un-rotate once, and compare with the exact
                // attention over the causal tail keys [body_window, qabs].
                double merged_m = -std::numeric_limits<double>::infinity();
                for (int split = 0; split < launch_capacity; ++split) {
                    if (host_m[stat_at(b, q_head, token, split)] == kUnwritten) { continue; }
                    if (host_l[stat_at(b, q_head, token, split)] > 0.0F) {
                        merged_m = std::max(
                            merged_m,
                            static_cast<double>(host_m[stat_at(b, q_head, token, split)]));
                    }
                }
                std::vector<double> merged_acc(kD, 0.0);
                double merged_l = 0.0;
                for (int split = 0; split < launch_capacity; ++split) {
                    if (host_m[stat_at(b, q_head, token, split)] == kUnwritten) { continue; }
                    const double ls = host_l[stat_at(b, q_head, token, split)];
                    if (!(ls > 0.0)) { continue; }
                    const double w =
                        std::exp(static_cast<double>(host_m[stat_at(b, q_head, token, split)]) -
                                 merged_m);
                    merged_l += ls * w;
                    for (int d = 0; d < kD; ++d) {
                        merged_acc[d] +=
                            static_cast<double>(host_acc[acc_at(b, q_head, d, token, split)]) * w;
                    }
                }
                std::vector<double> merged = merged_acc;
                host_hadamard_d256(merged);
                for (int d = 0; d < kD; ++d) {
                    merged[d] = merged_l > 0.0 ? merged[d] / merged_l : 0.0;
                }

                std::vector<double> oracle_acc(kD, 0.0);
                double oracle_m = -std::numeric_limits<double>::infinity();
                std::vector<double> scores;
                for (int key = partition.body_window; key <= qabs; ++key) {
                    double dot = 0.0;
                    for (int d = 0; d < kD; ++d) {
                        dot += oracle_query[q_index(q_head, d, column_begin + token, b)] *
                               static_cast<double>(host_ring_k[ring_index(b, key, kv_head, d)]);
                    }
                    scores.push_back(dot * 0.5);
                    oracle_m = std::max(oracle_m, scores.back());
                }
                double oracle_l = 0.0;
                std::vector<double> oracle(kD, 0.0);
                for (std::size_t i = 0; i < scores.size(); ++i) {
                    const double p = std::exp(scores[i] - oracle_m);
                    oracle_l += p;
                    const int key = partition.body_window + static_cast<int>(i);
                    for (int d = 0; d < kD; ++d) {
                        oracle[d] +=
                            p * static_cast<double>(host_ring_v[ring_index(b, key, kv_head, d)]);
                    }
                }
                double oracle_scale = 0.0;
                for (int d = 0; d < kD; ++d) {
                    oracle[d]    = oracle_l > 0.0 ? oracle[d] / oracle_l : 0.0;
                    oracle_scale = std::max(oracle_scale, std::fabs(oracle[d]));
                }
                for (int d = 0; d < kD; ++d) {
                    worst_rel = std::max(worst_rel,
                                         std::fabs(merged[d] - oracle[d]) / (oracle_scale + 1.0e-9));
                }
                ++rows_checked;
            }
        }
    }
    std::cout << label << ": base0=" << base0 << " base1=" << base1 << " tail=" << tail_tokens
              << " rows=" << rows_checked << " max_rel_vs_fp64=" << std::scientific
              << std::setprecision(3) << worst_rel
              << (worst_rel <= kTolerance ? "  OK\n" : "  FAIL\n");
    if (worst_rel > kTolerance) { ++failures; }
    return failures;
}

// WP6.3: the whole wiring at the Op level -- the body, the tail partial and the reducer sharing one
// partition. The tail covers the entire window here (`tail_tokens >= window`), so the body scores
// nothing and publishes an empty split and the Op's output must be the exact attention over the ring.
// A partition the three sides disagreed about would drop or double-count keys and the output would
// not match; the ring's rows are the same ones the body appended, so no quantization oracle is
// needed. `first_query_position` puts the window above 1024 (packed columns) or below it (scalar).
int run_exact_tail_merged_case(int window, int query_width, int first_query_position,
                               const std::string& label) {
    constexpr int QueryHeads = 24;
    constexpr int KVHeads    = 4;
    constexpr int Pages      = 34;
    using Geometry           = ops::CausalAttentionGeometry<QueryHeads, KVHeads, 1>;
    constexpr int kGroupSize = Geometry::GroupSize;
    const int ring_pages     = (window + 63) / 64;

    CacheFixture<KVHeads, Pages, 4> cache;
    const std::vector<float> body_k = make_cache_values(window, 0x7101U, KVHeads);
    const std::vector<float> body_v = make_cache_values(window, 0x7102U, KVHeads);
    append_cache(cache, body_k, body_v, 0, false);

    const auto kv_index = [&](int d, int head, int token) {
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (head + static_cast<std::size_t>(KVHeads) * token);
    };
    const auto ring_index = [&](int position, int head, int d) {
        const int ring = (position / 64) % ring_pages;
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (64 * (head + static_cast<std::size_t>(KVHeads) * ring) + position % 64);
    };

    std::vector<float> host_ring_k(static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages, 0.0F);
    std::vector<float> host_ring_v(host_ring_k.size(), 0.0F);
    for (int position = 0; position < window; ++position) {
        for (int head = 0; head < KVHeads; ++head) {
            for (int d = 0; d < kD; ++d) {
                host_ring_k[ring_index(position, head, d)] = body_k[kv_index(d, head, position)];
                host_ring_v[ring_index(position, head, d)] = body_v[kv_index(d, head, position)];
            }
        }
    }
    round_to_bf16(host_ring_k);
    round_to_bf16(host_ring_v);

    std::vector<float> host_query = make_cache_values(QueryHeads * query_width, 0x7103U);
    std::vector<std::int32_t> host_positions(query_width);
    for (int column = 0; column < query_width; ++column) {
        host_positions[column] = first_query_position + column;
    }

    DeviceBuffer device_query = to_device_bf16(host_query);
    DeviceBuffer positions    = to_device(host_positions);
    DeviceBuffer rows         = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer ring_k       = to_device_bf16(host_ring_k);
    DeviceBuffer ring_v       = to_device_bf16(host_ring_v);
    DeviceBuffer output(host_query.size() * sizeof(std::uint16_t));

    Tensor query_tensor(device_query.p, DType::BF16, {kD, QueryHeads, query_width, 1});
    Tensor output_tensor(output.p, DType::BF16, {kD, QueryHeads, query_width, 1});
    Tensor position_tensor(positions.p, DType::I32, {query_width, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});

    ops::KvarnPagedBatchLayerView view = cache.view();
    view.tail.k_pages                  = Tensor(ring_k.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
    view.tail.v_pages                  = Tensor(ring_v.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
    view.tail.page_count               = ring_pages;
    view.tail.retention                = window; // the tail holds every key of the window

    const ops::CausalAttentionExecutionEnvelope envelope{
        1, static_cast<std::uint32_t>(first_query_position + query_width)};
    WorkspaceArena workspace(std::max<std::size_t>(
        1, ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, query_width,
                                                         query_width)));
    ops::kvarn_attention_cached(query_tensor, position_tensor, rows_tensor, 0.0625F, view, envelope,
                                workspace, output_tensor, nullptr);
    cuda_synchronize();

    // Oracle: exact attention over the ring rows [0, pos+1) in the ORIGINAL domain. The Op rotates
    // the query in place, the tail partial writes W(sum p V_orig), and the reducer applies one
    // inverse rotation -- W is self-inverse, so it cancels end to end and the observable output is
    // sum p V_orig / l with NO net Hadamard. The body contributes a neutral split here (the tail
    // covers the whole window), so the oracle needs no body term.
    const auto q_at = [&](int q_head, int d, int column) {
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (q_head + static_cast<std::size_t>(QueryHeads) * column);
    };
    std::vector<double> expected(host_query.size());
    for (int column = 0; column < query_width; ++column) {
        const int visible = host_positions[column] + 1;
        for (int q_head = 0; q_head < QueryHeads; ++q_head) {
            const int kv_head = q_head / kGroupSize;
            std::vector<double> scores(visible);
            double maximum = -std::numeric_limits<double>::infinity();
            for (int key = 0; key < visible; ++key) {
                double dot = 0.0;
                for (int d = 0; d < kD; ++d) {
                    dot += static_cast<double>(host_query[q_at(q_head, d, column)]) *
                           static_cast<double>(host_ring_k[ring_index(key, kv_head, d)]);
                }
                scores[key] = dot * 0.0625;
                maximum     = std::max(maximum, scores[key]);
            }
            double denominator = 0.0;
            std::vector<double> accumulated(kD, 0.0);
            for (int key = 0; key < visible; ++key) {
                const double probability = std::exp(scores[key] - maximum);
                denominator += probability;
                for (int d = 0; d < kD; ++d) {
                    accumulated[d] +=
                        probability * static_cast<double>(host_ring_v[ring_index(key, kv_head, d)]);
                }
            }
            for (int d = 0; d < kD; ++d) {
                expected[q_at(q_head, d, column)] =
                    denominator > 0.0 ? accumulated[d] / denominator : 0.0;
            }
        }
    }

    return compare_profile(label.c_str(), from_device_bf16(output, host_query.size()), expected,
                           8.0e-3, /*report=*/true);
}

// WP6.4 (A6/A7 scaffolding). Three additions, all host-side:
//   * the KVarN long tier above 8198 keys *with a tail* (see `host_kvarn_active_splits` and the
//     `kvarn_long_tier` cases below) -- no host oracle covered it before;
//   * the needle instrument, which pins "the tail counted every key exactly once" at the
//     boundaries where the tail interval, the 128-token body group and the 64-token ring page
//     disagree;
//   * the `tail = 0` no-op regression.

constexpr int kNeedlePages = 80; // 128-token body pages, enough for the ~8.7k-token windows below

// A window's body rows are appended in 1024-token chunks. 1024 is a multiple of the 128-token KVarN
// record, so every record is still encoded from its own complete group of rows and the records are
// exactly what one wide append would have produced.
template <int Heads, int Pages, int Bits>
void append_cache_window(CacheFixture<Heads, Pages, Bits>& cache, std::uint32_t seed, int window) {
    constexpr int kChunk     = 1024;
    const std::size_t stride = static_cast<std::size_t>(kD) * Heads;
    const std::vector<float> key   = make_cache_values(window, seed, Heads);
    const std::vector<float> value = make_cache_values(window, seed + 1U, Heads);
    for (int begin = 0; begin < window; begin += kChunk) {
        const int count = std::min(kChunk, window - begin);
        std::vector<float> key_chunk(stride * count);
        std::vector<float> value_chunk(stride * count);
        std::memcpy(key_chunk.data(), key.data() + stride * begin, stride * count * sizeof(float));
        std::memcpy(value_chunk.data(), value.data() + stride * begin,
                    stride * count * sizeof(float));
        append_cache(cache, key_chunk, value_chunk, begin, false);
    }
}

// WP6.4: the A7 needle instrument, at the Op level.
//
// The ring carries two "needle" rows whose key projects to 1000 on the query axis and whose values
// are large and different; every other row in the window projects to ~0 on that axis. The query is
// one axis (`q = e_0`), so the two needles score ~62 nats above every other key and the softmax is
// (1/2, 1/2, ~0, ~0, ...): the observable output must be exactly (V_a + V_b) / 2, with no body term
// and no split arithmetic in the oracle. The neighbouring outcomes are all distinct, so the
// instrument pins "counted exactly once" rather than "approximately right":
//   * a dropped needle gives V_b (or V_a);
//   * a double-counted needle gives (2 V_a + V_b) / 3 (or (V_a + 2 V_b) / 3);
//   * a needle read from the wrong ring slot gives some other value entirely.
// Because 62 nats of margin leave the ~9k non-needle keys with a total weight under 1e-14, the body
// needs no quantization model here: whatever the body reads only has to be *small*, which the
// appended rows are. `needle_a` / `needle_b` are absolute positions, chosen so the tail interval's
// ends disagree with the 128-token body group and the 64-token ring page boundaries.
int run_exact_tail_needle_case(int window, int tail_tokens, int needle_a, int needle_b,
                               int ring_pages, const std::string& label) {
    constexpr int QueryHeads = 24;
    constexpr int KVHeads    = 4;

    const int ring_capacity = 64 * ring_pages;
    const int ring_begin    = std::max(0, window - ring_capacity);
    const int tail_keys     = std::min(tail_tokens, window);
    const int body_window   = window - tail_keys;
    const HostTailPartition partition = host_kvarn_tail_partition(window, tail_tokens, 82);

    if (ring_begin > body_window || needle_a < body_window || needle_a >= window ||
        needle_b < body_window || needle_b >= window || needle_a == needle_b) {
        std::cerr << label << ": needle case preconditions not met (window=" << window
                  << " tail=" << tail_tokens << " body_window=" << body_window << " ring=["
                  << ring_begin << ", " << window << ") needles=" << needle_a << "," << needle_b
                  << ")\n";
        return 1;
    }

    CacheFixture<KVHeads, kNeedlePages, kBits> cache;
    append_cache_window(cache, 0x7a01U, window);

    const auto ring_index = [&](int position, int head, int d) {
        const int page = (position / 64) % ring_pages;
        return static_cast<std::size_t>(d) +
               static_cast<std::size_t>(kD) *
                   (64 * (head + static_cast<std::size_t>(KVHeads) * page) + position % 64);
    };
    // `first` is the needle at `needle_a`; the two value profiles differ in both the low and the
    // high dims, so a drop, a double count and a swap are three different observable outputs. The
    // magnitudes are deliberately not BF16-exact, so the result is a real rounded average rather
    // than a value that happens to land on the grid.
    const auto needle_value = [](bool first, int d) {
        const bool low = (d % 4) == 0;
        if (first) { return low ? 8.3F : -4.1F; }
        return low ? -2.1F : 6.3F;
    };
    // What the kernel reads out of the ring is the BF16 image of those values.
    const auto needle_read = [&](bool first, int d) {
        return static_cast<double>(bf16_to_f32(f32_to_bf16(needle_value(first, d))));
    };

    std::vector<float> host_ring_k(static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages, 0.0F);
    std::vector<float> host_ring_v(host_ring_k.size(), 0.0F);
    for (int position = ring_begin; position < window; ++position) {
        const bool is_needle = position == needle_a || position == needle_b;
        for (int head = 0; head < KVHeads; ++head) {
            for (int d = 0; d < kD; ++d) {
                const std::size_t slot = ring_index(position, head, d);
                if (is_needle) {
                    host_ring_k[slot] = d == 0 ? 1000.0F : 0.0F;
                    host_ring_v[slot] = needle_value(position == needle_a, d);
                } else {
                    host_ring_k[slot] = 0.0F;
                    host_ring_v[slot] = 0.5F * std::sin(0.01F * (position + 1) * (d + 1) + head);
                }
            }
        }
    }
    round_to_bf16(host_ring_k);
    round_to_bf16(host_ring_v);

    // q = e_0: one axis, so `q . k` is `k[0]` and the two needles' score is 1000 * 0.0625 = 62.5.
    std::vector<float> host_query(static_cast<std::size_t>(kD) * QueryHeads, 0.0F);
    for (int head = 0; head < QueryHeads; ++head) { host_query[static_cast<std::size_t>(head) * kD] = 1.0F; }
    const std::vector<std::int32_t> host_positions{window - 1};

    DeviceBuffer device_query = to_device_bf16(host_query);
    DeviceBuffer positions    = to_device(host_positions);
    DeviceBuffer rows         = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer ring_k       = to_device_bf16(host_ring_k);
    DeviceBuffer ring_v       = to_device_bf16(host_ring_v);
    DeviceBuffer output(host_query.size() * sizeof(std::uint16_t));

    Tensor query_tensor(device_query.p, DType::BF16, {kD, QueryHeads, 1, 1});
    Tensor output_tensor(output.p, DType::BF16, {kD, QueryHeads, 1, 1});
    Tensor position_tensor(positions.p, DType::I32, {1, 1});
    Tensor rows_tensor(rows.p, DType::I32, {1});

    ops::KvarnPagedBatchLayerView view = cache.view();
    view.tail.k_pages    = Tensor(ring_k.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
    view.tail.v_pages    = Tensor(ring_v.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
    view.tail.page_count = ring_pages;
    view.tail.retention  = tail_tokens;

    const ops::CausalAttentionExecutionEnvelope envelope{1, static_cast<std::uint32_t>(window)};
    WorkspaceArena workspace(std::max<std::size_t>(
        1, ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, 1, 1)));
    ops::kvarn_attention_cached(query_tensor, position_tensor, rows_tensor, 0.0625F, view, envelope,
                                workspace, output_tensor, nullptr);
    cuda_synchronize();

    const auto at = [](int head, int d) {
        return static_cast<std::size_t>(d) + static_cast<std::size_t>(kD) * head;
    };
    std::vector<double> expected(host_query.size());
    std::vector<double> dropped_a(host_query.size());
    std::vector<double> dropped_b(host_query.size());
    std::vector<double> doubled_a(host_query.size());
    std::vector<double> doubled_b(host_query.size());
    for (int head = 0; head < QueryHeads; ++head) {
        for (int d = 0; d < kD; ++d) {
            const double a = needle_read(true, d);
            const double b = needle_read(false, d);
            // The merge is `sum p V / l` with p_a == p_b and l == 2 p, so the two weights cancel and
            // the only rounding left between the needle values and the BF16 output is the output's
            // own quantization -- which the oracle applies too.
            const double mean = static_cast<double>(bf16_to_f32(f32_to_bf16(
                static_cast<float>((a + b) / 2.0))));
            expected[at(head, d)]  = mean;
            dropped_a[at(head, d)] = b;
            dropped_b[at(head, d)] = a;
            doubled_a[at(head, d)] = (2.0 * a + b) / 3.0;
            doubled_b[at(head, d)] = (a + 2.0 * b) / 3.0;
        }
    }
    const std::vector<double> got = from_device_bf16(output, host_query.size());
    const auto distance = [&](const std::vector<double>& reference) {
        double error = 0.0;
        for (std::size_t i = 0; i < got.size(); ++i) { error += std::fabs(got[i] - reference[i]); }
        return error;
    };
    const double miss      = distance(dropped_a) + distance(dropped_b);
    const double duplicate = distance(doubled_a) + distance(doubled_b);
    const double nearest_alternative = std::min(miss, duplicate);
    std::cout << label << ": window=" << window << " tail=" << tail_tokens
              << " needle_a=" << needle_a << " needle_b=" << needle_b
              << " body_window=" << body_window << " body_active=" << partition.body_active
              << " tail_active=" << partition.tail_active << '\n';
    int failures = compare_profile(label.c_str(), got, expected, 2.0e-2, true);
    // The instrument is only worth its tolerance if the wrong answers are far outside it. A dropped
    // needle misses by |V_a - V_b| / 2 per element and a double count by a third of that, so the
    // nearest wrong profile has to be orders of magnitude further than the rounding the oracle
    // absorbs; a shrinking margin means the needles stopped carrying the softmax.
    const double kMinDiscrimination = 1.0e2;
    std::cout << label << ": nearest_wrong_profile_l1=" << std::scientific << std::setprecision(3)
              << nearest_alternative << (nearest_alternative >= kMinDiscrimination ? "  OK\n"
                                                                                  : "  FAIL\n");
    if (nearest_alternative < kMinDiscrimination) { ++failures; }
    if (failures != 0) {
        std::cerr << label << ": |got-mean|=" << distance(expected)
                  << " |got-drop_a|=" << distance(dropped_a)
                  << " |got-drop_b|=" << distance(dropped_b)
                  << " |got-double_a|=" << distance(doubled_a)
                  << " |got-double_b|=" << distance(doubled_b) << '\n';
    }
    return failures;
}

// WP6.4: the A6 "tail = 0 leaves the output unchanged" regression, in the form a test can pin.
//
// D-23 settled that byte identity *across binaries* is unattainable by construction: the WP6.2
// BFloat16 -> FP32 per-split accumulator is a required deliverable, so a tail-free output must move
// relative to the pre-WP6.2 build, and no reference binary exists in the tree. What is still
// observable, and what this case pins, is the same-binary form of the invariant: a cache view that
// carries an *allocated* ring at `retention == 0` must produce the very same bytes as a view with no
// ring at all -- the tail has to be inert, not merely equal in value -- and the same launch must be
// self-deterministic, which is what makes the bitwise criterion single-shot decidable (the 08-16
// synchronisation fix removed the only known source of run-to-run drift).
int run_tail_zero_regression_case(int window, int first_query_position, const std::string& label) {
    constexpr int QueryHeads = 24;
    constexpr int KVHeads    = 4;
    constexpr int ring_pages = 4;

    CacheFixture<KVHeads, kNeedlePages, kBits> cache;
    append_cache_window(cache, 0x7b01U, window);

    const std::vector<std::int32_t> host_positions{first_query_position};
    const std::vector<float> host_query = make_cache_values(QueryHeads, 0x7b02U);
    DeviceBuffer positions = to_device(host_positions);
    DeviceBuffer rows      = to_device(std::vector<std::int32_t>{0});
    DeviceBuffer ring_k(static_cast<std::size_t>(kD) * 64 * KVHeads * ring_pages *
                        sizeof(std::uint16_t));
    DeviceBuffer ring_v(ring_k.bytes);
    ring_k.fill();
    ring_v.fill();

    const ops::CausalAttentionExecutionEnvelope envelope{
        1, static_cast<std::uint32_t>(first_query_position + 1)};

    const auto run = [&](bool with_ring) {
        DeviceBuffer query = to_device_bf16(host_query);
        DeviceBuffer output(host_query.size() * sizeof(std::uint16_t));
        Tensor query_tensor(query.p, DType::BF16, {kD, QueryHeads, 1, 1});
        Tensor output_tensor(output.p, DType::BF16, {kD, QueryHeads, 1, 1});
        Tensor position_tensor(positions.p, DType::I32, {1, 1});
        Tensor rows_tensor(rows.p, DType::I32, {1});
        ops::KvarnPagedBatchLayerView view = cache.view();
        if (with_ring) {
            view.tail.k_pages    = Tensor(ring_k.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
            view.tail.v_pages    = Tensor(ring_v.p, DType::BF16, {kD, 64, KVHeads, ring_pages});
            view.tail.page_count = ring_pages;
            view.tail.retention  = 0; // allocated, but the feature is off
        }
        WorkspaceArena workspace(std::max<std::size_t>(
            1, ops::kvarn_attention_workspace_capacity_bytes(QueryHeads, envelope, 1, 1, 1)));
        ops::kvarn_attention_cached(query_tensor, position_tensor, rows_tensor, 0.0625F, view,
                                    envelope, workspace, output_tensor, nullptr);
        cuda_synchronize();
        return from_device<std::uint16_t>(output, host_query.size());
    };

    const std::vector<std::uint16_t> bare     = run(false);
    const std::vector<std::uint16_t> ringed   = run(true);
    const std::vector<std::uint16_t> repeated = run(false);
    const bool inert      = bare == ringed;
    const bool repeatable = bare == repeated;
    std::cout << label << ": window=" << window << " position=" << first_query_position
              << " ring-at-retention-zero-identical=" << (inert ? "yes" : "NO")
              << " self-deterministic=" << (repeatable ? "yes" : "NO") << '\n';
    return (inert && repeatable) ? 0 : 1;
}


} // namespace

int main() {
    if (cuda_unavailable()) {
        std::cout << "SKIP: no usable CUDA device\n";
        return 77;
    }
    int failures = 0;
    failures += run_codec_case<4>();
    failures += run_codec_case<5>();
    failures += run_codec_case<6>();
    failures += run_codec_bit_order_case();
    failures += run_hadamard_case();
    failures += run_publication_settlement_case<2, 1>();
    failures += run_publication_settlement_case<4, 16>();
    failures += run_append_attention_oracle<4, 24>(0, 384, false);
    failures += run_append_attention_oracle<4, 24>(254, 256, false);
    failures += run_append_attention_oracle<4, 24>(254, 256, true);
    failures += run_append_attention_oracle<2, 16>(254, 256, false);
    for (const int width : {8, 16}) {
        failures += run_append_attention_oracle<4, 24>(254, width, false);
        failures += run_append_attention_oracle<2, 16>(254, width, false);
        failures += run_append_attention_oracle<4, 24>(254, width, true);
    }
    failures += run_cache_lifecycle_case();
    failures += run_27b_attention_case();
    failures += run_27b_grouped_decode_case<2>();
    failures += run_27b_grouped_decode_case<3>();
    failures += run_27b_grouped_decode_case<4>();
    failures += run_27b_grouped_decode_case<4, 3>();
    failures += run_27b_grouped_decode_case<4, 4, 4095>();
    failures += run_27b_grouped_decode_case<5>();
    failures += run_27b_grouped_decode_case<6>();
    failures += run_27b_grouped_decode_case<8, 7, 32784>();
    failures += run_27b_grouped_decode_case<16, 15, 196624>();
    failures += run_27b_grouped_decode_case<16, 7, 32784>();
    failures += run_27b_grouped_decode_case<16, 16, 32784>();
    failures += run_35b_attention_case();
    failures += run_prefill_slab_boundary_case();
    failures += run_batched_attention_case<4>(24, "KVarN H24/KV4 B=2 attention");
    failures += run_batched_attention_case<2>(16, "KVarN H16/KV2 B=2 attention");
    failures += run_tail_staging_case(6);
    failures += run_tail_staging_case(8);
    failures += run_tail_staging_case(16);
    for (int width = 1; width <= 6; ++width) {
        failures += run_speculative_boundary_case<4, 24>(width, width, 1, 2111);
        if (width > 1) { failures += run_speculative_boundary_case<4, 24>(width, width, 1, 2110); }
    }
    failures += run_speculative_boundary_case<4, 24>(6, 3, 2, 2110);
    for (int first : {1022, 1086, 4093, 4094, 4095, 4096, 8190, 8196, 8198}) {
        for (int width = 2; width <= 6; ++width) {
            failures += run_speculative_boundary_case<4, 24>(width, width, 1, first);
        }
    }
    for (int offset = 0; offset < kGroup; ++offset) {
        failures += run_speculative_boundary_case<4, 24>(6, 6, 1, 2048 + offset);
    }
    for (int valid = 0; valid <= 6; ++valid) {
        for (int accepted = 0; accepted <= valid; ++accepted) {
            failures += run_speculative_boundary_case<4, 24>(6, valid, accepted, 4094);
        }
    }
    failures += run_speculative_boundary_case<4, 24, 520>(6, 6, 6, 32798);
    failures += run_speculative_boundary_case<4, 24, 1940>(6, 6, 1, 122878);
    failures += run_speculative_boundary_case<4, 24, 1940>(4, 3, 2, 122879);
    for (int width : {8, 16}) {
        failures += run_speculative_boundary_case<4, 24>(width, width, 1, 4094);
        failures += run_speculative_boundary_case<4, 24>(width, width, 1, 8190);
        failures += run_speculative_boundary_case<4, 24, 1940>(width, width - 1, 2, 122878);
    }
    failures += run_speculative_boundary_case<4, 24>(16, 7, 2, 2110);
    failures += run_speculative_boundary_case<4, 24, 3074>(4, 4, 1, 196604);
    failures += run_speculative_boundary_case<2, 16>(16, 13, 1, 190);
    CacheFixture<4, 34> packed_cache;
    append_cache(packed_cache, make_cache_values(2128, 0xe001U, 4),
                 make_cache_values(2128, 0xe002U, 4), 0, false);
    failures +=
        run_cached_attention_case(packed_cache, 24, 2112, 6, "KVarN random packed attention");
    failures +=
        run_cached_attention_case(packed_cache, 24, 2112, 8, "KVarN random width-8 attention");
    failures +=
        run_cached_attention_case(packed_cache, 24, 2112, 16, "KVarN random width-16 attention");
    // The 5- and 6-bit record decode also runs this packed-cache attention path, so it is covered
    // end to end and not only by the codec oracle.
    CacheFixture<4, 34, 5> packed_k5;
    append_cache(packed_k5, make_cache_values(2128, 0xe001U, 4), make_cache_values(2128, 0xe002U, 4),
                 0, false);
    failures += run_cached_attention_case(packed_k5, 24, 2112, 6, "KVarN k5v5 packed attention");
    CacheFixture<4, 34, 6> packed_k6;
    append_cache(packed_k6, make_cache_values(2128, 0xe001U, 4), make_cache_values(2128, 0xe002U, 4),
                 0, false);
    failures += run_cached_attention_case(packed_k6, 24, 2112, 6, "KVarN k6v6 packed attention");
    // WP6.1: the rotated-domain exact-tail partial against an independent FP64 oracle.
    failures += run_exact_tail_partial_case<24, 4, 1>(256, 128, 8, "KVarN exact tail window 256 N128");
    failures +=
        run_exact_tail_partial_case<24, 4, 1>(1024, 384, 16, "KVarN exact tail window 1024 N384");
    failures += run_exact_tail_partial_case<24, 4, 1>(100, 100, 8, "KVarN exact tail whole window");
    failures += run_exact_tail_partial_case<24, 4, 1>(1024, 0, 16, "KVarN exact tail disabled");
    failures += run_exact_tail_partial_case<16, 2, 2>(512, 256, 12, "KVarN H16/KV2 exact tail");
    failures += run_exact_tail_partial_case<24, 4, 1>(1024, 384, 16, "KVarN exact tail f16 ring",
                                                      true);
    // WP6.3: the KVarN append's own ring write, and the tail partial on a chunked, batched launch
    // (per-sequence window, per-row query/statistic offsets).
    failures += run_exact_tail_stage_case(0, 200, 4, 1, false, false, "KVarN exact tail stage bf16");
    failures += run_exact_tail_stage_case(0, 200, 4, 1, false, true, "KVarN exact tail stage f16");
    failures += run_exact_tail_stage_case(0, 640, 4, 1, false, false, "KVarN exact tail stage clipped");
    failures +=
        run_exact_tail_stage_case(0, 200, 4, 2, false, true, "KVarN exact tail stage f16 batch");
    failures +=
        run_exact_tail_stage_case(1000, 6, 4, 2, true, false, "KVarN exact tail stage masked batch");
    failures += run_exact_tail_partial_launch_case(1000, 4200, 384, 16,
                                                   "KVarN exact tail launch batched chunked");
    failures += run_exact_tail_partial_launch_case(100, 300, 128, 8,
                                                   "KVarN exact tail launch short window");
    // WP6.3: the Op-level wiring, with the tail covering the whole window (so the oracle is the
    // exact attention over the ring and needs no quantization model). One window above 1024 selects
    // the packed two-column columns route, one below it the scalar route.
    failures += run_exact_tail_merged_case(1000, 6, 994, "KVarN exact tail merged scalar");
    failures += run_exact_tail_merged_case(1200, 16, 1184, "KVarN exact tail merged packed");
    // WP6.4 (A6/A7 scaffolding): the KVarN long tier above 8198 keys with a tail -- no host oracle
    // covered it before -- plus the retention boundaries and a full turn of the ring.
    failures += run_exact_tail_partial_case<24, 4, 1>(8200, 384, 82,
                                                      "KVarN exact tail crossing 8198", false, true);
    failures += run_exact_tail_partial_case<24, 4, 1>(8700, 384, 82,
                                                      "KVarN exact tail long body", false, true, 7);
    failures += run_exact_tail_partial_case<24, 4, 1>(122881, 384, 82,
                                                      "KVarN exact tail long-split cap", false, true, 7);
    failures += run_exact_tail_partial_case<24, 4, 1>(8703, 400, 82,
                                                      "KVarN exact tail ring wrap N400", false, true,
                                                      8);
    failures += run_exact_tail_partial_case<24, 4, 1>(200, 1000, 8,
                                                      "KVarN exact tail N above window");
    // The needle instrument at the boundaries the tail interval disagrees with: a body-group edge
    // mid-page, the exact group/page boundary, and two needles inside one ring page.
    failures += run_exact_tail_needle_case(8700, 384, 8316, 8699, 7, "KVarN tail needle mid-page");
    failures += run_exact_tail_needle_case(8576, 384, 8192, 8384, 7,
                                           "KVarN tail needle on the group boundary");
    failures += run_exact_tail_needle_case(8600, 384, 8332, 8333, 7,
                                           "KVarN tail needle same ring page");
    // The whole window inside the tail (N above the window), where the body scores nothing at all.
    failures += run_exact_tail_needle_case(1000, 2000, 500, 999, 17,
                                           "KVarN tail needle whole window");
    // The tail = 0 no-op regression (same-binary form; see the case's comment and D-23).
    failures += run_tail_zero_regression_case(8400, 8399, "KVarN tail=0 regression");
    std::cout << (failures == 0 ? "OK" : "FAIL") << " kvarn correctness\n";
    return failures == 0 ? 0 : 1;
}
