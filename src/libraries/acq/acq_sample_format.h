//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_sample_format.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Native sample formats and memory layouts stored in an acquisition ring.
 *
 * Rings hold the device's own sample format in host byte order; backends that receive another byte
 * order swap while writing. Physical values are produced only on request by the converters in
 * @ref acq_convert.h. Packed int24 is little-endian, three bytes per sample, as delivered by most
 * 24-bit biosignal ADCs.
 */

#ifndef ACQ_SAMPLE_FORMAT_H
#define ACQ_SAMPLE_FORMAT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"

#include <complex>
#include <cstddef>
#include <cstdint>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Native sample format of a stream. Values are part of the ABI.
 */
enum class AcqSampleFormat : std::uint8_t
{
    Int16 = 1,         /**< Signed 16-bit integer. */
    Int24Packed = 2,   /**< Signed 24-bit integer, 3 bytes little-endian (AcqInt24). */
    Int32 = 3,         /**< Signed 32-bit integer. */
    Float32 = 4,       /**< IEEE 754 binary32. */
    Float64 = 5,       /**< IEEE 754 binary64. */
    ComplexInt16 = 6,  /**< I/Q pair of signed 16-bit integers (AcqComplexInt16). */
    ComplexFloat32 = 7 /**< std::complex<float>. */
};

//=============================================================================================================
/**
 * Memory layout of a ring.
 *
 * Interleaved: one frame (all channels of one sample index) follows the next; typical for USB/TCP devices.
 * Planar: each channel occupies its own contiguous ring of the same capacity; typical for digitizer
 * cards and DMA engines that write one channel per buffer.
 */
enum class AcqLayout : std::uint8_t
{
    Interleaved = 1, /**< Element (channel c, sample s) at byte (s * channels + c) * bytesPerSample. */
    Planar = 2       /**< Element (channel c, sample s) at byte (c * capacity + s) * bytesPerSample. */
};

//=============================================================================================================
/**
 * Packed signed 24-bit sample, little-endian. Trivially copyable, size 3, alignment 1.
 */
struct AcqInt24
{
    std::uint8_t bytes[3]; /**< Least significant byte first. */

    /** @return The sign-extended value. */
    [[nodiscard]] constexpr std::int32_t value() const noexcept
    {
        const auto raw = static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8U) |
            (static_cast<std::uint32_t>(bytes[2]) << 16U);
        return static_cast<std::int32_t>(raw << 8U) >> 8;
    }
};
static_assert(sizeof(AcqInt24) == 3, "AcqInt24 must be packed");

//=============================================================================================================
/**
 * I/Q sample of two signed 16-bit integers, real part first. std::complex is only specified for
 * floating-point types, hence this type.
 */
struct AcqComplexInt16
{
    std::int16_t re; /**< In-phase component. */
    std::int16_t im; /**< Quadrature component. */
};
static_assert(sizeof(AcqComplexInt16) == 4, "AcqComplexInt16 must be packed");

//=============================================================================================================
/**
 * @param[in] format   Sample format.
 *
 * @return Bytes per sample of one channel (an I/Q pair counts as one sample); 0 for an invalid value.
 */
[[nodiscard]] constexpr std::size_t bytesPerSample(AcqSampleFormat format) noexcept
{
    switch (format) {
        case AcqSampleFormat::Int16:
            return 2;
        case AcqSampleFormat::Int24Packed:
            return 3;
        case AcqSampleFormat::Int32:
            return 4;
        case AcqSampleFormat::Float32:
            return 4;
        case AcqSampleFormat::Float64:
            return 8;
        case AcqSampleFormat::ComplexInt16:
            return 4;
        case AcqSampleFormat::ComplexFloat32:
            return 8;
    }
    return 0;
}

/**
 * @param[in] format   Sample format.
 *
 * @return true for the complex formats.
 */
[[nodiscard]] constexpr bool isComplex(AcqSampleFormat format) noexcept
{
    return format == AcqSampleFormat::ComplexInt16 || format == AcqSampleFormat::ComplexFloat32;
}

/**
 * @param[in] format   Sample format.
 *
 * @return The stable lowercase name, e.g. "float32", "cint16"; "unknown" for invalid values.
 */
[[nodiscard]] ACQSHARED_EXPORT std::string_view sampleFormatName(AcqSampleFormat format) noexcept;

//=============================================================================================================
/// @cond INTERNAL
// Maps a C++ element type to its sample format, for map<T>() in acq_eigen.h.
template<typename T>
struct AcqSampleTraits; // undefined for unsupported types

template<>
struct AcqSampleTraits<std::int16_t>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::Int16;
};
template<>
struct AcqSampleTraits<AcqInt24>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::Int24Packed;
};
template<>
struct AcqSampleTraits<std::int32_t>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::Int32;
};
template<>
struct AcqSampleTraits<float>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::Float32;
};
template<>
struct AcqSampleTraits<double>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::Float64;
};
template<>
struct AcqSampleTraits<AcqComplexInt16>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::ComplexInt16;
};
template<>
struct AcqSampleTraits<std::complex<float>>
{
    static constexpr AcqSampleFormat format = AcqSampleFormat::ComplexFloat32;
};
/// @endcond

} // namespace mne

#endif // ACQ_SAMPLE_FORMAT_H
