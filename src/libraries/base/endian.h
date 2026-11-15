//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     endian.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Byte-order conversion of integers and floating-point values in standard C++.
 */

#ifndef BASE_ENDIAN_H
#define BASE_ENDIAN_H

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/** Integers and floating-point values of 1, 2, 4 or 8 bytes. */
template<typename T>
concept Swappable = std::is_arithmetic_v<T> && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

//=============================================================================================================
/**
 * @param[in] value   Any swappable value.
 *
 * @return @p value with its bytes in reverse order.
 */
template<Swappable T>
[[nodiscard]] constexpr T byteSwap(T value) noexcept
{
    if constexpr (sizeof(T) == 1) {
        return value;
    } else {
        using U = std::conditional_t<sizeof(T) == 2, std::uint16_t, std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>>;
        U bits = std::bit_cast<U>(value);
        U swapped = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            swapped = static_cast<U>((swapped << 8) | (bits & 0xFFu));
            bits = static_cast<U>(bits >> 8);
        }
        return std::bit_cast<T>(swapped);
    }
}

//=============================================================================================================
/**
 * @param[in] value   A value in host byte order.
 *
 * @return @p value in big-endian (network, FIFF) byte order.
 */
template<Swappable T>
[[nodiscard]] constexpr T toBigEndian(T value) noexcept
{
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return byteSwap(value);
    }
}

//=============================================================================================================
/**
 * @param[in] value   A value in big-endian byte order.
 *
 * @return @p value in host byte order.
 */
template<Swappable T>
[[nodiscard]] constexpr T fromBigEndian(T value) noexcept
{
    return toBigEndian(value);
}

//=============================================================================================================
/**
 * Reverses the bytes of every element of @p data.
 *
 * @param[in, out] data    The values.
 * @param[in]      count   Number of values.
 */
template<Swappable T>
void byteSwapInPlace(T* data, std::size_t count) noexcept
{
    for (std::size_t i = 0; i < count; ++i) {
        data[i] = byteSwap(data[i]);
    }
}

} // namespace mne

#endif // BASE_ENDIAN_H
