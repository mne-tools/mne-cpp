//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_convert.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    decode(), encode() and the bit-field converters.
 *
 * Each converter is one loop nest per (format, destination type), instantiated from templates: the format
 * is a compile-time parameter, so the inner loops over channels and samples contain no branch on it and the
 * compiler vectorises them. Blocks of kBlock samples keep source and destination of a block in cache when
 * the destination is transposed relative to the ring. The library is built with floating-point
 * contraction off, so the documented arithmetic (multiply, then add, no FMA) holds on every target.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_convert.h"

#include <algorithm>
#include <array>
#include <utility>
#include <cmath>
#include <cstring>
#include <type_traits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

constexpr std::size_t kBlock = 64;

template<typename T>
struct IsComplex : std::false_type
{
};
template<typename T>
struct IsComplex<std::complex<T>> : std::true_type
{
};

//=============================================================================================================
// NATIVE ELEMENT ACCESS
//=============================================================================================================

/** Properties of a format; integer formats also give their word width. */
template<AcqSampleFormat F>
struct Native;

template<>
struct Native<AcqSampleFormat::Int16>
{
    static constexpr bool integer = true;
    static constexpr bool complex = false;
    static constexpr int bits = 16;
};
template<>
struct Native<AcqSampleFormat::Int24Packed>
{
    static constexpr bool integer = true;
    static constexpr bool complex = false;
    static constexpr int bits = 24;
};
template<>
struct Native<AcqSampleFormat::Int32>
{
    static constexpr bool integer = true;
    static constexpr bool complex = false;
    static constexpr int bits = 32;
};
template<>
struct Native<AcqSampleFormat::Float32>
{
    static constexpr bool integer = false;
    static constexpr bool complex = false;
};
template<>
struct Native<AcqSampleFormat::Float64>
{
    static constexpr bool integer = false;
    static constexpr bool complex = false;
};
template<>
struct Native<AcqSampleFormat::ComplexInt16>
{
    static constexpr bool integer = true;
    static constexpr bool complex = true;
    static constexpr int bits = 16;
};
template<>
struct Native<AcqSampleFormat::ComplexFloat32>
{
    static constexpr bool integer = false;
    static constexpr bool complex = true;
};

/** Bytes of one integer component (one of I/Q for complex formats). */
template<AcqSampleFormat F>
constexpr std::size_t componentBytes() noexcept
{
    return F == AcqSampleFormat::Int24Packed ? 3 : static_cast<std::size_t>(Native<F>::bits) / 8;
}

/** Integer word of component @p k (unsigned, little-endian). */
template<AcqSampleFormat F>
std::uint32_t loadWord(const std::byte* element, int k) noexcept
{
    const std::byte* p = element + static_cast<std::size_t>(k) * componentBytes<F>();
    if constexpr (F == AcqSampleFormat::Int24Packed) {
        return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1])) << 8U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2])) << 16U);
    } else if constexpr (Native<F>::bits == 32) {
        std::uint32_t w = 0;
        std::memcpy(&w, p, sizeof(w));
        return w;
    } else {
        std::uint16_t w = 0;
        std::memcpy(&w, p, sizeof(w));
        return w;
    }
}

template<AcqSampleFormat F>
void storeWord(std::byte* element, int k, std::uint32_t w) noexcept
{
    std::byte* p = element + static_cast<std::size_t>(k) * componentBytes<F>();
    if constexpr (F == AcqSampleFormat::Int24Packed) {
        p[0] = static_cast<std::byte>(w & 0xFFU);
        p[1] = static_cast<std::byte>((w >> 8U) & 0xFFU);
        p[2] = static_cast<std::byte>((w >> 16U) & 0xFFU);
    } else if constexpr (Native<F>::bits == 32) {
        std::memcpy(p, &w, sizeof(w));
    } else {
        const auto h = static_cast<std::uint16_t>(w);
        std::memcpy(p, &h, sizeof(h));
    }
}

/** Analog integer of component @p k with @p d digital bits on top: signExtend(low W-d bits) << d. */
template<AcqSampleFormat F>
std::int64_t loadInteger(const std::byte* element, int k, int d) noexcept
{
    constexpr int kBits = Native<F>::bits;
    const auto w = static_cast<std::uint64_t>(loadWord<F>(element, k));
    const int analog = kBits - d;
    const std::uint64_t low = w & ((std::uint64_t(1) << analog) - 1);
    const std::uint64_t sign = std::uint64_t(1) << (analog - 1);
    const auto value = static_cast<std::int64_t>(low ^ sign) - static_cast<std::int64_t>(sign);
    return value * (std::int64_t(1) << d);
}

template<AcqSampleFormat F>
double loadFloating(const std::byte* element, int k) noexcept
{
    if constexpr (F == AcqSampleFormat::Float64) {
        double v = 0;
        std::memcpy(&v, element, sizeof(v));
        return v;
    } else {
        float v = 0;
        std::memcpy(&v, element + static_cast<std::size_t>(k) * sizeof(float), sizeof(v));
        return v;
    }
}

/** Component @p k as Real, optionally scaled: Real(raw) * Real(scale) [+ Real(offset) for the real part]. */
template<AcqSampleFormat F, typename Real>
Real component(const std::byte* element, int k, int d, bool physical, Real scale, Real offset) noexcept
{
    Real v = 0;
    if constexpr (Native<F>::integer) {
        v = static_cast<Real>(loadInteger<F>(element, k, d));
    } else {
        v = static_cast<Real>(loadFloating<F>(element, k));
    }
    if (physical) {
        v = v * scale;
        if (k == 0) {
            v = v + offset;
        }
    }
    return v;
}

//=============================================================================================================
// DECODE
//=============================================================================================================

template<AcqSampleFormat F, typename T>
void decodeRun(const AcqBlockPart& part, std::size_t firstChannel, std::span<const double> scales,
               std::span<const double> offsets, std::span<const std::uint8_t> digital, const AcqMatrixRef<T>& out,
               std::size_t firstColumn, AcqScaling scaling) noexcept
{
    using Real = std::conditional_t<IsComplex<T>::value, float, T>;
    const bool physical = scaling == AcqScaling::Physical;
    for (std::size_t s0 = 0; s0 < part.samples; s0 += kBlock) {
        const std::size_t s1 = std::min(part.samples, s0 + kBlock);
        for (std::size_t r = 0; r < out.rows; ++r) {
            const std::size_t c = firstChannel + r;
            const Real scale = physical ? static_cast<Real>(scales[c]) : Real(1);
            const Real offset = physical ? static_cast<Real>(offsets[c]) : Real(0);
            const int d = digital.empty() ? 0 : digital[c];
            T* row = out.data + static_cast<std::ptrdiff_t>(r) * out.rowStride;
            for (std::size_t s = s0; s < s1; ++s) {
                const std::byte* element = part.at(c, s);
                T& dst = row[static_cast<std::ptrdiff_t>(firstColumn + s) * out.colStride];
                if constexpr (IsComplex<T>::value) {
                    const Real re = component<F, Real>(element, 0, d, physical, scale, offset);
                    Real im = 0;
                    if constexpr (Native<F>::complex) {
                        im = component<F, Real>(element, 1, 0, physical, scale, offset);
                    }
                    dst = T(re, im);
                } else {
                    dst = component<F, Real>(element, 0, d, physical, scale, offset);
                }
            }
        }
    }
}

template<typename T>
using DecodeFn = void (*)(const AcqBlockPart&, std::size_t, std::span<const double>, std::span<const double>,
                          std::span<const std::uint8_t>, const AcqMatrixRef<T>&, std::size_t, AcqScaling) noexcept;

template<typename T>
DecodeFn<T> decoderFor(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return &decodeRun<AcqSampleFormat::Int16, T>;
        case AcqSampleFormat::Int24Packed:
            return &decodeRun<AcqSampleFormat::Int24Packed, T>;
        case AcqSampleFormat::Int32:
            return &decodeRun<AcqSampleFormat::Int32, T>;
        case AcqSampleFormat::Float32:
            return &decodeRun<AcqSampleFormat::Float32, T>;
        case AcqSampleFormat::Float64:
            return &decodeRun<AcqSampleFormat::Float64, T>;
        case AcqSampleFormat::ComplexInt16:
            if constexpr (IsComplex<T>::value) {
                return &decodeRun<AcqSampleFormat::ComplexInt16, T>;
            }
            return nullptr;
        case AcqSampleFormat::ComplexFloat32:
            if constexpr (IsComplex<T>::value) {
                return &decodeRun<AcqSampleFormat::ComplexFloat32, T>;
            }
            return nullptr;
    }
    return nullptr;
}

/** Channels of a range; @return false if it does not fit @p channels. */
bool resolveRange(const AcqChannelRange& range, std::size_t channels, std::size_t& first, std::size_t& count) noexcept
{
    if (range.first > channels) {
        return false;
    }
    first = range.first;
    count = range.count == std::numeric_limits<std::size_t>::max() ? channels - first : range.count;
    return count <= channels - first;
}

//=============================================================================================================
// FAST PATHS: real formats, channels without digital bits
//=============================================================================================================
// The general kernels above handle every case one element at a time. The kernels below cover the common case
// with the format and the scaling as template parameters, typed loads and stores, and loops whose innermost
// dimension is contiguous in both memories wherever the layouts allow it, so the compiler vectorises them.
// The arithmetic is the same expression as above, so the results are bit-identical.

/** Channels processed per pass of the contiguous-channel kernels (per-channel factors live on the stack). */
constexpr std::size_t kChannelChunk = 256;

/** Native value of a real-format element. */
template<AcqSampleFormat F>
auto loadReal(const std::byte* p) noexcept
{
    if constexpr (F == AcqSampleFormat::Int16) {
        std::int16_t v = 0;
        std::memcpy(&v, p, sizeof(v));
        return static_cast<std::int32_t>(v);
    } else if constexpr (F == AcqSampleFormat::Int24Packed) {
        const auto u = static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1])) << 8U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2])) << 16U);
        return static_cast<std::int32_t>(u << 8U) >> 8;
    } else if constexpr (F == AcqSampleFormat::Int32) {
        std::int32_t v = 0;
        std::memcpy(&v, p, sizeof(v));
        return v;
    } else if constexpr (F == AcqSampleFormat::Float32) {
        float v = 0;
        std::memcpy(&v, p, sizeof(v));
        return v;
    } else {
        double v = 0;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }
}

template<AcqSampleFormat F>
constexpr std::size_t realBytes() noexcept
{
    return F == AcqSampleFormat::Int24Packed ? 3
                                             : (F == AcqSampleFormat::Int16 ? 2 : (F == AcqSampleFormat::Float64 ? 8 : 4));
}

/** Decoded value: Real(raw) * scale + offset (no FMA), or Real(raw). */
template<AcqSampleFormat F, typename Real, bool Physical>
Real decodeValue(const std::byte* p, Real scale, Real offset) noexcept
{
    const auto raw = static_cast<Real>(loadReal<F>(p));
    if constexpr (Physical) {
        const Real scaled = raw * scale;
        return scaled + offset;
    } else {
        return raw;
    }
}

/** @return Whether a channel in [first, first + count) carries digital bits (those take the general path). */
bool anyDigitalBits(std::span<const std::uint8_t> digital, std::size_t first, std::size_t count) noexcept
{
    return !digital.empty() &&
        std::any_of(digital.begin() + static_cast<std::ptrdiff_t>(first),
                    digital.begin() + static_cast<std::ptrdiff_t>(first + count), [](std::uint8_t d) { return d != 0; });
}

template<AcqSampleFormat F, typename Real, bool Physical>
void decodeFastRun(const AcqBlockPart& part, std::size_t firstChannel, std::span<const double> scales,
                   std::span<const double> offsets, const AcqMatrixRef<Real>& out, std::size_t firstColumn) noexcept
{
    constexpr auto kBytes = static_cast<std::ptrdiff_t>(realBytes<F>());
    const auto factor = [&](std::span<const double> v, std::size_t c, double fallback) {
        return static_cast<Real>(Physical ? v[c] : fallback);
    };
    if (part.sampleStride == kBytes && out.colStride == 1) {
        // Samples contiguous in the ring (planar) and in the matrix (row-major): one stream per channel.
        for (std::size_t r = 0; r < out.rows; ++r) {
            const std::byte* src = part.at(firstChannel + r, 0);
            Real* dst = out.data + static_cast<std::ptrdiff_t>(r) * out.rowStride + static_cast<std::ptrdiff_t>(firstColumn);
            const Real scale = factor(scales, firstChannel + r, 1.0);
            const Real offset = factor(offsets, firstChannel + r, 0.0);
            for (std::size_t s = 0; s < part.samples; ++s) {
                dst[s] = decodeValue<F, Real, Physical>(src + static_cast<std::ptrdiff_t>(s) * kBytes, scale, offset);
            }
        }
        return;
    }
    if (part.channelStride == kBytes && out.rowStride == 1) {
        // Channels contiguous in the ring (interleaved) and in the matrix (column-major): one frame per column.
        std::array<Real, kChannelChunk> scale{};
        std::array<Real, kChannelChunk> offset{};
        for (std::size_t c0 = 0; c0 < out.rows; c0 += kChannelChunk) {
            const std::size_t n = std::min(kChannelChunk, out.rows - c0);
            for (std::size_t i = 0; i < n; ++i) {
                scale[i] = factor(scales, firstChannel + c0 + i, 1.0);
                offset[i] = factor(offsets, firstChannel + c0 + i, 0.0);
            }
            for (std::size_t s = 0; s < part.samples; ++s) {
                const std::byte* src = part.at(firstChannel + c0, s);
                Real* dst = out.data + static_cast<std::ptrdiff_t>(firstColumn + s) * out.colStride +
                    static_cast<std::ptrdiff_t>(c0);
                for (std::size_t i = 0; i < n; ++i) {
                    dst[i] = decodeValue<F, Real, Physical>(src + static_cast<std::ptrdiff_t>(i) * kBytes, scale[i],
                                                            offset[i]);
                }
            }
        }
        return;
    }
    if (part.channelStride == kBytes && out.colStride == 1) {
        // Interleaved ring into a row-major matrix: decode a tile of frames along the contiguous channels, then
        // write it out row by row, so both memories are read and written in order.
        constexpr std::size_t kTileSamples = 64;
        constexpr std::size_t kTileChannels = 32;
        std::array<Real, kTileSamples * kTileChannels> tile{};
        for (std::size_t c0 = 0; c0 < out.rows; c0 += kTileChannels) {
            const std::size_t n = std::min(kTileChannels, out.rows - c0);
            std::array<Real, kTileChannels> scale{};
            std::array<Real, kTileChannels> offset{};
            for (std::size_t i = 0; i < n; ++i) {
                scale[i] = factor(scales, firstChannel + c0 + i, 1.0);
                offset[i] = factor(offsets, firstChannel + c0 + i, 0.0);
            }
            for (std::size_t s0 = 0; s0 < part.samples; s0 += kTileSamples) {
                const std::size_t m = std::min(kTileSamples, part.samples - s0);
                for (std::size_t s = 0; s < m; ++s) {
                    const std::byte* src = part.at(firstChannel + c0, s0 + s);
                    Real* row = tile.data() + s * kTileChannels;
                    for (std::size_t i = 0; i < n; ++i) {
                        row[i] = decodeValue<F, Real, Physical>(src + static_cast<std::ptrdiff_t>(i) * kBytes, scale[i],
                                                                offset[i]);
                    }
                }
                for (std::size_t i = 0; i < n; ++i) {
                    Real* dst = out.data + static_cast<std::ptrdiff_t>(c0 + i) * out.rowStride +
                        static_cast<std::ptrdiff_t>(firstColumn + s0);
                    for (std::size_t s = 0; s < m; ++s) {
                        dst[s] = tile[s * kTileChannels + i];
                    }
                }
            }
        }
        return;
    }
    // Other strides: blocks of kBlock samples, so a block's source and destination stay in cache.
    for (std::size_t s0 = 0; s0 < part.samples; s0 += kBlock) {
        const std::size_t s1 = std::min(part.samples, s0 + kBlock);
        for (std::size_t r = 0; r < out.rows; ++r) {
            const Real scale = factor(scales, firstChannel + r, 1.0);
            const Real offset = factor(offsets, firstChannel + r, 0.0);
            const std::byte* src = part.at(firstChannel + r, 0);
            Real* dst = out.data + static_cast<std::ptrdiff_t>(r) * out.rowStride;
            for (std::size_t s = s0; s < s1; ++s) {
                dst[static_cast<std::ptrdiff_t>(firstColumn + s) * out.colStride] =
                    decodeValue<F, Real, Physical>(src + static_cast<std::ptrdiff_t>(s) * part.sampleStride, scale, offset);
            }
        }
    }
}

template<typename Real>
using DecodeFastFn = void (*)(const AcqBlockPart&, std::size_t, std::span<const double>, std::span<const double>,
                              const AcqMatrixRef<Real>&, std::size_t) noexcept;

/** I/Q formats into complex<float>: I * scale + offset, Q * scale; the same expressions as component(). */
template<AcqSampleFormat F, bool Physical>
void decodeComplexFastRun(const AcqBlockPart& part, std::size_t firstChannel, std::span<const double> scales,
                          std::span<const double> offsets, const AcqMatrixRef<std::complex<float>>& out,
                          std::size_t firstColumn) noexcept
{
    using Component = std::conditional_t<F == AcqSampleFormat::ComplexInt16, std::int16_t, float>;
    constexpr auto kBytes = static_cast<std::ptrdiff_t>(2 * sizeof(Component));
    const auto element = [](const std::byte* p, [[maybe_unused]] float scale, [[maybe_unused]] float offset) {
        std::array<Component, 2> iq{};
        std::memcpy(iq.data(), p, sizeof(iq));
        auto re = static_cast<float>(iq[0]);
        auto im = static_cast<float>(iq[1]);
        if constexpr (Physical) {
            re = re * scale;
            re = re + offset;
            im = im * scale;
        }
        return std::complex<float>(re, im);
    };
    const auto factor = [&](std::span<const double> v, std::size_t c, double fallback) {
        return static_cast<float>(Physical ? v[c] : fallback);
    };
    if (part.channelStride == kBytes && out.rowStride == 1) {
        std::array<float, kChannelChunk> scale{};
        std::array<float, kChannelChunk> offset{};
        for (std::size_t c0 = 0; c0 < out.rows; c0 += kChannelChunk) {
            const std::size_t n = std::min(kChannelChunk, out.rows - c0);
            for (std::size_t i = 0; i < n; ++i) {
                scale[i] = factor(scales, firstChannel + c0 + i, 1.0);
                offset[i] = factor(offsets, firstChannel + c0 + i, 0.0);
            }
            for (std::size_t s = 0; s < part.samples; ++s) {
                const std::byte* src = part.at(firstChannel + c0, s);
                std::complex<float>* dst =
                    out.data + static_cast<std::ptrdiff_t>(firstColumn + s) * out.colStride + static_cast<std::ptrdiff_t>(c0);
                for (std::size_t i = 0; i < n; ++i) {
                    dst[i] = element(src + static_cast<std::ptrdiff_t>(i) * kBytes, scale[i], offset[i]);
                }
            }
        }
        return;
    }
    for (std::size_t r = 0; r < out.rows; ++r) {
        const float scale = factor(scales, firstChannel + r, 1.0);
        const float offset = factor(offsets, firstChannel + r, 0.0);
        const std::byte* src = part.at(firstChannel + r, 0);
        std::complex<float>* dst = out.data + static_cast<std::ptrdiff_t>(r) * out.rowStride;
        for (std::size_t s = 0; s < part.samples; ++s) {
            dst[static_cast<std::ptrdiff_t>(firstColumn + s) * out.colStride] =
                element(src + static_cast<std::ptrdiff_t>(s) * part.sampleStride, scale, offset);
        }
    }
}

template<typename Real, bool Physical>
DecodeFastFn<Real> fastDecoderFor(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return &decodeFastRun<AcqSampleFormat::Int16, Real, Physical>;
        case AcqSampleFormat::Int24Packed:
            return &decodeFastRun<AcqSampleFormat::Int24Packed, Real, Physical>;
        case AcqSampleFormat::Int32:
            return &decodeFastRun<AcqSampleFormat::Int32, Real, Physical>;
        case AcqSampleFormat::Float32:
            return &decodeFastRun<AcqSampleFormat::Float32, Real, Physical>;
        case AcqSampleFormat::Float64:
            return &decodeFastRun<AcqSampleFormat::Float64, Real, Physical>;
        default:
            return nullptr;
    }
}

/**
 * Stores the native value of a real format.
 *
 * @return 1 if the value could not be represented (NaN, outside the channel range or the native type), else 0;
 *         returned rather than accumulated through a reference, which the byte stores could alias.
 */
template<AcqSampleFormat F, bool Physical>
std::uint64_t encodeValue(double x, std::byte* p, double scale, double offset, double lo, double hi) noexcept
{
    const bool nan = std::isnan(x);
    double v = nan ? 0.0 : x;
    std::uint64_t clip = nan ? 1U : 0U;
    if constexpr (Physical) {
        const double c = std::fmin(std::fmax(v, lo), hi);
        clip += !nan && c != v ? 1U : 0U;
        v = nan ? 0.0 : (c - offset) / scale;
    }
    if constexpr (F == AcqSampleFormat::Float64) {
        const bool inf = std::isinf(v);
        clip += inf ? 1U : 0U;
        v = inf ? std::copysign(std::numeric_limits<double>::max(), v) : v;
        std::memcpy(p, &v, sizeof(v));
    } else if constexpr (F == AcqSampleFormat::Float32) {
        constexpr double kMax = std::numeric_limits<float>::max();
        const double c = std::min(std::max(v, -kMax), kMax);
        clip += c != v ? 1U : 0U;
        const auto f = static_cast<float>(c);
        std::memcpy(p, &f, sizeof(f));
    } else {
        constexpr double kTop = F == AcqSampleFormat::Int16 ? 32768.0 : (F == AcqSampleFormat::Int24Packed ? 8388608.0 : 2147483648.0);
        // Round to nearest, ties to even: exact for |v| < 2^51; anything larger saturates anyway.
        constexpr double kRound = 0x1.8p52;
        const double bounded = std::fmin(std::fmax(v, -0x1p40), 0x1p40);
        const double rounded = (bounded + kRound) - kRound;
        const double q = std::fmin(std::fmax(rounded, -kTop), kTop - 1.0);
        clip += q != rounded ? 1U : 0U;
        const auto i = static_cast<std::int32_t>(q);
        if constexpr (F == AcqSampleFormat::Int16) {
            const auto h = static_cast<std::int16_t>(i);
            std::memcpy(p, &h, sizeof(h));
        } else if constexpr (F == AcqSampleFormat::Int24Packed) {
            const auto u = static_cast<std::uint32_t>(i);
            p[0] = static_cast<std::byte>(u & 0xFFU);
            p[1] = static_cast<std::byte>((u >> 8U) & 0xFFU);
            p[2] = static_cast<std::byte>((u >> 16U) & 0xFFU);
        } else {
            std::memcpy(p, &i, sizeof(i));
        }
    }
    return clip;
}

template<AcqSampleFormat F, typename T, bool Physical>
void encodeFastRun(const AcqMatrixRef<const T>& in, std::size_t firstColumn, const AcqWritePart& part,
                   std::size_t firstChannel, std::span<const double> scales, std::span<const double> offsets,
                   std::span<const double> rangeMin, std::span<const double> rangeMax, std::uint64_t& clipped) noexcept
{
    constexpr auto kBytes = static_cast<std::ptrdiff_t>(realBytes<F>());
    constexpr double kInf = std::numeric_limits<double>::infinity();
    struct Limits
    {
        double lo;
        double hi;
    };
    // A plain struct, not std::pair, whose by-value passing GCC flags as changed in its ABI (-Wpsabi note).
    const auto limits = [&](std::size_t c) {
        return Limits{std::isnan(rangeMin[c]) ? -kInf : rangeMin[c], std::isnan(rangeMax[c]) ? kInf : rangeMax[c]};
    };
    std::uint64_t clip = 0;
    if (part.sampleStride == kBytes && in.colStride == 1) {
        for (std::size_t r = 0; r < in.rows; ++r) {
            const std::size_t c = firstChannel + r;
            const auto [lo, hi] = limits(c);
            const double scale = scales[c];
            const double offset = offsets[c];
            const T* src = in.data + static_cast<std::ptrdiff_t>(r) * in.rowStride + static_cast<std::ptrdiff_t>(firstColumn);
            std::byte* dst = part.at(c, 0);
            for (std::size_t s = 0; s < part.samples; ++s) {
                clip += encodeValue<F, Physical>(static_cast<double>(src[s]), dst + static_cast<std::ptrdiff_t>(s) * kBytes,
                                                 scale, offset, lo, hi);
            }
        }
    } else if (part.channelStride == kBytes && in.rowStride == 1) {
        // Per-channel factors on the stack: the byte stores below could alias the spans' memory.
        std::array<double, kChannelChunk> lo{};
        std::array<double, kChannelChunk> hi{};
        std::array<double, kChannelChunk> scale{};
        std::array<double, kChannelChunk> offset{};
        for (std::size_t c0 = 0; c0 < in.rows; c0 += kChannelChunk) {
            const std::size_t n = std::min(kChannelChunk, in.rows - c0);
            for (std::size_t i = 0; i < n; ++i) {
                const Limits l = limits(firstChannel + c0 + i);
                lo[i] = l.lo;
                hi[i] = l.hi;
                scale[i] = scales[firstChannel + c0 + i];
                offset[i] = offsets[firstChannel + c0 + i];
            }
            for (std::size_t s = 0; s < part.samples; ++s) {
                const T* src = in.data + static_cast<std::ptrdiff_t>(firstColumn + s) * in.colStride + static_cast<std::ptrdiff_t>(c0);
                std::byte* dst = part.at(firstChannel + c0, s);
                for (std::size_t i = 0; i < n; ++i) {
                    clip += encodeValue<F, Physical>(static_cast<double>(src[i]), dst + static_cast<std::ptrdiff_t>(i) * kBytes,
                                                     scale[i], offset[i], lo[i], hi[i]);
                }
            }
        }
    } else {
        for (std::size_t s0 = 0; s0 < part.samples; s0 += kBlock) {
            const std::size_t s1 = std::min(part.samples, s0 + kBlock);
            for (std::size_t r = 0; r < in.rows; ++r) {
                const std::size_t c = firstChannel + r;
                const auto [lo, hi] = limits(c);
                const double scale = scales[c];
                const double offset = offsets[c];
                const T* src = in.data + static_cast<std::ptrdiff_t>(r) * in.rowStride;
                for (std::size_t s = s0; s < s1; ++s) {
                    clip += encodeValue<F, Physical>(static_cast<double>(src[static_cast<std::ptrdiff_t>(firstColumn + s) * in.colStride]),
                                                     part.at(c, s), scale, offset, lo, hi);
                }
            }
        }
    }
    clipped += clip;
}

template<typename T>
using EncodeFastFn = void (*)(const AcqMatrixRef<const T>&, std::size_t, const AcqWritePart&, std::size_t,
                              std::span<const double>, std::span<const double>, std::span<const double>,
                              std::span<const double>, std::uint64_t&) noexcept;

template<typename T, bool Physical>
EncodeFastFn<T> fastEncoderFor(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return &encodeFastRun<AcqSampleFormat::Int16, T, Physical>;
        case AcqSampleFormat::Int24Packed:
            return &encodeFastRun<AcqSampleFormat::Int24Packed, T, Physical>;
        case AcqSampleFormat::Int32:
            return &encodeFastRun<AcqSampleFormat::Int32, T, Physical>;
        case AcqSampleFormat::Float32:
            return &encodeFastRun<AcqSampleFormat::Float32, T, Physical>;
        case AcqSampleFormat::Float64:
            return &encodeFastRun<AcqSampleFormat::Float64, T, Physical>;
        default:
            return nullptr;
    }
}

template<typename T>
AcqError decodeView(const AcqBlockView& view, const AcqMatrixRef<T>& out, const AcqConvertOptions& options) noexcept
{
    if (!view.memory().hostAccessible()) {
        return AcqErrorCode::DeviceMemory;
    }
    std::size_t first = 0;
    std::size_t count = 0;
    if (!resolveRange(options.channels, view.channelCount(), first, count) || out.data == nullptr ||
        out.rows != count || out.cols < options.firstColumn + view.sampleCount()) {
        return AcqErrorCode::ShapeMismatch;
    }
    const DecodeFn<T> fn = decoderFor<T>(view.format());
    if (fn == nullptr) {
        return AcqErrorCode::UnsupportedFormat;
    }
    const AcqMatrixRef<T> rows{out.data, count, out.cols, out.rowStride, out.colStride};
    std::size_t column = options.firstColumn;
    DecodeFastFn<T> fast = nullptr;
    const bool physical = options.scaling == AcqScaling::Physical;
    if (!anyDigitalBits(view.digitalBits(), first, count)) {
        if constexpr (IsComplex<T>::value) {
            if (view.format() == AcqSampleFormat::ComplexInt16) {
                fast = physical ? &decodeComplexFastRun<AcqSampleFormat::ComplexInt16, true>
                                : &decodeComplexFastRun<AcqSampleFormat::ComplexInt16, false>;
            } else if (view.format() == AcqSampleFormat::ComplexFloat32) {
                fast = physical ? &decodeComplexFastRun<AcqSampleFormat::ComplexFloat32, true>
                                : &decodeComplexFastRun<AcqSampleFormat::ComplexFloat32, false>;
            }
        } else {
            fast = physical ? fastDecoderFor<T, true>(view.format()) : fastDecoderFor<T, false>(view.format());
        }
    }
    if (fast != nullptr) {
        for (std::size_t p = 0; p < view.partCount(); ++p) {
            fast(view.part(p), first, view.scales(), view.offsets(), rows, column);
            column += view.part(p).samples;
        }
        return {};
    }
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        fn(view.part(p), first, view.scales(), view.offsets(), view.digitalBits(), rows, column, options.scaling);
        column += view.part(p).samples;
    }
    return {};
}

//=============================================================================================================
// ENCODE
//=============================================================================================================

/** Physical or raw value to the native value of one component; counts clipping in @p clipped. */
template<AcqSampleFormat F>
void encodeComponent(double value, std::byte* element, int k, int d, bool physical, double scale, double offset,
                     double lo, double hi, std::uint64_t& clipped) noexcept
{
    if (std::isnan(value)) {
        value = 0.0;
        physical = false;
        ++clipped;
    }
    if (physical) {
        if (value < lo || value > hi) {
            value = value < lo ? lo : hi;
            ++clipped;
        }
        value = (value - (k == 0 ? offset : 0.0)) / scale;
    }
    if constexpr (F == AcqSampleFormat::Float64) {
        if (std::isinf(value)) {
            value = std::copysign(std::numeric_limits<double>::max(), value);
            ++clipped;
        }
        std::memcpy(element, &value, sizeof(value));
    } else if constexpr (!Native<F>::integer) {
        constexpr double kMax = std::numeric_limits<float>::max();
        if (value > kMax || value < -kMax) {
            value = std::copysign(kMax, value);
            ++clipped;
        }
        const auto v = static_cast<float>(value);
        std::memcpy(element + static_cast<std::size_t>(k) * sizeof(float), &v, sizeof(v));
    } else {
        constexpr int kBits = Native<F>::bits;
        const int analog = kBits - d;
        const double top = std::ldexp(1.0, analog - 1);
        // Default rounding mode: nearest, ties to even.
        double q = std::nearbyint(std::ldexp(value, -d));
        if (q < -top || q > top - 1.0) {
            q = q < -top ? -top : top - 1.0;
            ++clipped;
        }
        const std::uint64_t lowMask = (std::uint64_t(1) << analog) - 1;
        const auto low = static_cast<std::uint64_t>(static_cast<std::int64_t>(q)) & lowMask;
        const auto old = static_cast<std::uint64_t>(loadWord<F>(element, k));
        storeWord<F>(element, k, static_cast<std::uint32_t>((old & ~lowMask) | low));
    }
}

template<AcqSampleFormat F, typename T>
void encodeRun(const AcqMatrixRef<const T>& in, std::size_t firstColumn, const AcqWritePart& part,
               std::size_t firstChannel, std::span<const double> scales, std::span<const double> offsets,
               std::span<const double> rangeMin, std::span<const double> rangeMax, std::span<const std::uint8_t> digital,
               AcqScaling scaling, std::uint64_t& clipped) noexcept
{
    const bool physical = scaling == AcqScaling::Physical;
    for (std::size_t s0 = 0; s0 < part.samples; s0 += kBlock) {
        const std::size_t s1 = std::min(part.samples, s0 + kBlock);
        for (std::size_t r = 0; r < in.rows; ++r) {
            const std::size_t c = firstChannel + r;
            const double scale = physical ? scales[c] : 1.0;
            const double offset = physical ? offsets[c] : 0.0;
            const double lo = std::isnan(rangeMin[c]) ? -std::numeric_limits<double>::infinity() : rangeMin[c];
            const double hi = std::isnan(rangeMax[c]) ? std::numeric_limits<double>::infinity() : rangeMax[c];
            const int d = digital.empty() ? 0 : digital[c];
            const T* row = in.data + static_cast<std::ptrdiff_t>(r) * in.rowStride;
            for (std::size_t s = s0; s < s1; ++s) {
                std::byte* element = part.at(c, s);
                const T& value = row[static_cast<std::ptrdiff_t>(firstColumn + s) * in.colStride];
                if constexpr (IsComplex<T>::value) {
                    encodeComponent<F>(value.real(), element, 0, d, physical, scale, offset, lo, hi, clipped);
                    if constexpr (Native<F>::complex) {
                        encodeComponent<F>(value.imag(), element, 1, 0, physical, scale, offset, lo, hi, clipped);
                    } else if (value.imag() != 0.0F) {
                        ++clipped;
                    }
                } else {
                    encodeComponent<F>(static_cast<double>(value), element, 0, d, physical, scale, offset, lo, hi,
                                       clipped);
                    if constexpr (Native<F>::complex) {
                        std::uint64_t ignored = 0;
                        encodeComponent<F>(0.0, element, 1, 0, physical, scale, offset, lo, hi, ignored);
                    }
                }
            }
        }
    }
}

template<typename T>
using EncodeFn = void (*)(const AcqMatrixRef<const T>&, std::size_t, const AcqWritePart&, std::size_t,
                          std::span<const double>, std::span<const double>, std::span<const double>,
                          std::span<const double>, std::span<const std::uint8_t>, AcqScaling, std::uint64_t&) noexcept;

template<typename T>
EncodeFn<T> encoderFor(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return &encodeRun<AcqSampleFormat::Int16, T>;
        case AcqSampleFormat::Int24Packed:
            return &encodeRun<AcqSampleFormat::Int24Packed, T>;
        case AcqSampleFormat::Int32:
            return &encodeRun<AcqSampleFormat::Int32, T>;
        case AcqSampleFormat::Float32:
            return &encodeRun<AcqSampleFormat::Float32, T>;
        case AcqSampleFormat::Float64:
            return &encodeRun<AcqSampleFormat::Float64, T>;
        case AcqSampleFormat::ComplexInt16:
            return &encodeRun<AcqSampleFormat::ComplexInt16, T>;
        case AcqSampleFormat::ComplexFloat32:
            return &encodeRun<AcqSampleFormat::ComplexFloat32, T>;
    }
    return nullptr;
}

template<typename T>
AcqExpected<AcqEncodeResult> encodeView(const AcqMatrixRef<const T>& in, const AcqWriteView& view,
                                        const AcqConvertOptions& options) noexcept
{
    if (!view.memory().hostAccessible()) {
        return AcqErrorCode::DeviceMemory;
    }
    std::size_t first = 0;
    std::size_t count = 0;
    if (!resolveRange(options.channels, view.channelCount(), first, count) || in.data == nullptr ||
        in.rows != count || in.cols < options.firstColumn + view.sampleCount()) {
        return AcqErrorCode::ShapeMismatch;
    }
    const EncodeFn<T> fn = encoderFor<T>(view.format());
    if (fn == nullptr) {
        return AcqErrorCode::UnsupportedFormat;
    }
    const AcqMatrixRef<const T> rows{in.data, count, in.cols, in.rowStride, in.colStride};
    AcqEncodeResult result;
    std::size_t column = options.firstColumn;
    if constexpr (!IsComplex<T>::value) {
        const EncodeFastFn<T> fast = anyDigitalBits(view.digitalBits(), first, count)
            ? nullptr
            : (options.scaling == AcqScaling::Physical ? fastEncoderFor<T, true>(view.format())
                                                       : fastEncoderFor<T, false>(view.format()));
        if (fast != nullptr) {
            for (std::size_t p = 0; p < view.partCount(); ++p) {
                fast(rows, column, view.part(p), first, view.scales(), view.offsets(), view.rangeMin(),
                     view.rangeMax(), result.clipped);
                column += view.part(p).samples;
            }
            return result;
        }
    }
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        fn(rows, column, view.part(p), first, view.scales(), view.offsets(), view.rangeMin(), view.rangeMax(),
           view.digitalBits(), options.scaling, result.clipped);
        column += view.part(p).samples;
    }
    return result;
}

//=============================================================================================================
// BIT FIELDS
//=============================================================================================================

/** Word bits of a format's channel word; 0 for floating-point formats. */
int integerWordBits(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
        case AcqSampleFormat::ComplexInt16:
            return 16;
        case AcqSampleFormat::Int24Packed:
            return 24;
        case AcqSampleFormat::Int32:
            return 32;
        default:
            return 0;
    }
}

/** Fields must lie inside their channel's digital bits, at the top of the word. */
template<typename View>
AcqError checkFields(const View& view, std::span<const AcqBitField> fields) noexcept
{
    const int bits = integerWordBits(view.format());
    if (bits == 0) {
        return AcqErrorCode::UnsupportedFormat;
    }
    for (const AcqBitField& field : fields) {
        if (field.channel >= view.channelCount() || field.width == 0) {
            return AcqErrorCode::InvalidParameter;
        }
        const int digital = view.digitalBits()[field.channel];
        if (field.firstBit + field.width > bits || field.firstBit < bits - digital) {
            return AcqErrorCode::InvalidParameter;
        }
    }
    return {};
}

std::uint32_t wordOf(const std::byte* element, AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int24Packed:
            return loadWord<AcqSampleFormat::Int24Packed>(element, 0);
        case AcqSampleFormat::Int32:
            return loadWord<AcqSampleFormat::Int32>(element, 0);
        default:
            return loadWord<AcqSampleFormat::Int16>(element, 0);
    }
}

void setWord(std::byte* element, AcqSampleFormat format, std::uint32_t w) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int24Packed:
            storeWord<AcqSampleFormat::Int24Packed>(element, 0, w);
            break;
        case AcqSampleFormat::Int32:
            storeWord<AcqSampleFormat::Int32>(element, 0, w);
            break;
        default:
            storeWord<AcqSampleFormat::Int16>(element, 0, w);
            break;
    }
}

std::uint32_t fieldMask(const AcqBitField& field) noexcept
{
    return field.width >= 32 ? 0xFFFFFFFFU : (std::uint32_t(1) << field.width) - 1;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqError mne::decode(const AcqBlockView& view, const AcqMatrixRef<float>& out,
                     const AcqConvertOptions& options) noexcept
{
    return decodeView(view, out, options);
}

AcqError mne::decode(const AcqBlockView& view, const AcqMatrixRef<double>& out,
                     const AcqConvertOptions& options) noexcept
{
    return decodeView(view, out, options);
}

AcqError mne::decode(const AcqBlockView& view, const AcqMatrixRef<std::complex<float>>& out,
                     const AcqConvertOptions& options) noexcept
{
    return decodeView(view, out, options);
}

//=============================================================================================================

AcqExpected<AcqEncodeResult> mne::encode(const AcqMatrixRef<const float>& in, const AcqWriteView& view,
                                         const AcqConvertOptions& options) noexcept
{
    return encodeView(in, view, options);
}

AcqExpected<AcqEncodeResult> mne::encode(const AcqMatrixRef<const double>& in, const AcqWriteView& view,
                                         const AcqConvertOptions& options) noexcept
{
    return encodeView(in, view, options);
}

AcqExpected<AcqEncodeResult> mne::encode(const AcqMatrixRef<const std::complex<float>>& in,
                                         const AcqWriteView& view, const AcqConvertOptions& options) noexcept
{
    return encodeView(in, view, options);
}

//=============================================================================================================

AcqError mne::decodeBits(const AcqBlockView& view, std::span<const AcqBitField> fields,
                         const AcqMatrixRef<std::uint32_t>& out, std::size_t firstColumn) noexcept
{
    if (!view.memory().hostAccessible()) {
        return AcqErrorCode::DeviceMemory;
    }
    if (AcqError error = checkFields(view, fields); !error.ok()) {
        return error;
    }
    if (out.data == nullptr || out.rows != fields.size() || out.cols < firstColumn + view.sampleCount()) {
        return AcqErrorCode::ShapeMismatch;
    }
    std::size_t column = firstColumn;
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        const AcqBlockPart& part = view.part(p);
        for (std::size_t f = 0; f < fields.size(); ++f) {
            const AcqBitField& field = fields[f];
            std::uint32_t* row = out.data + static_cast<std::ptrdiff_t>(f) * out.rowStride;
            for (std::size_t s = 0; s < part.samples; ++s) {
                const std::uint32_t w = wordOf(part.at(field.channel, s), view.format());
                row[static_cast<std::ptrdiff_t>(column + s) * out.colStride] = (w >> field.firstBit) & fieldMask(field);
            }
        }
        column += part.samples;
    }
    return {};
}

//=============================================================================================================

AcqExpected<AcqEncodeResult> mne::encodeBits(const AcqMatrixRef<const std::uint32_t>& in,
                                             std::span<const AcqBitField> fields, const AcqWriteView& view,
                                             std::size_t firstColumn) noexcept
{
    if (!view.memory().hostAccessible()) {
        return AcqErrorCode::DeviceMemory;
    }
    if (AcqError error = checkFields(view, fields); !error.ok()) {
        return error;
    }
    if (in.data == nullptr || in.rows != fields.size() || in.cols < firstColumn + view.sampleCount()) {
        return AcqErrorCode::ShapeMismatch;
    }
    AcqEncodeResult result;
    std::size_t column = firstColumn;
    for (std::size_t p = 0; p < view.partCount(); ++p) {
        const AcqWritePart& part = view.part(p);
        for (std::size_t f = 0; f < fields.size(); ++f) {
            const AcqBitField& field = fields[f];
            const std::uint32_t mask = fieldMask(field);
            const std::uint32_t* row = in.data + static_cast<std::ptrdiff_t>(f) * in.rowStride;
            for (std::size_t s = 0; s < part.samples; ++s) {
                const std::uint32_t value = row[static_cast<std::ptrdiff_t>(column + s) * in.colStride];
                result.clipped += (value & ~mask) != 0 ? 1U : 0U;
                std::byte* element = part.at(field.channel, s);
                const std::uint32_t w = wordOf(element, view.format());
                setWord(element, view.format(), (w & ~(mask << field.firstBit)) | ((value & mask) << field.firstBit));
            }
        }
        column += part.samples;
    }
    return result;
}

//=============================================================================================================

AcqError mne::decodePart(const AcqBlockPart& part, AcqSampleFormat format, std::span<const double> scales,
                         std::span<const double> offsets, const AcqMatrixRef<float>& out,
                         AcqScaling scaling) noexcept
{
    if (out.data == nullptr || out.cols < part.samples ||
        (scaling == AcqScaling::Physical && (scales.size() != out.rows || offsets.size() != out.rows))) {
        return AcqErrorCode::ShapeMismatch;
    }
    const DecodeFn<float> fn = decoderFor<float>(format);
    if (fn == nullptr) {
        return AcqErrorCode::UnsupportedFormat;
    }
    fn(part, 0, scales, offsets, {}, out, 0, scaling);
    return {};
}
