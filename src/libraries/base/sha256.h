//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sha256.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    SHA-256 (FIPS 180-4) in standard C++, the counterpart of QCryptographicHash::Sha256.
 */

#ifndef BASE_SHA256_H
#define BASE_SHA256_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "base_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Computes SHA-256 digests in one call or incrementally, e.g. to fingerprint pipeline inputs.
 *
 * Use to_hex() from string_utils.h for the hex text QCryptographicHash::hash(...).toHex() gives.
 *
 * @snippet ex_base/main.cpp sha256_usage
 */
class BASESHARED_EXPORT Sha256
{
public:
    /** The 32-byte digest. */
    using Digest = std::array<std::byte, 32>;

    //=========================================================================================================
    /**
     * Appends bytes to the hashed message.
     *
     * @param[in] data   The bytes to append.
     *
     * @return This hash, to chain calls.
     */
    Sha256& update(std::span<const std::byte> data);

    //=========================================================================================================
    /**
     * Appends text, as its bytes, to the hashed message.
     *
     * @param[in] text   The text to append.
     *
     * @return This hash, to chain calls.
     */
    Sha256& update(std::string_view text);

    //=========================================================================================================
    /**
     * @return The digest of everything appended so far; more data can still be appended.
     */
    [[nodiscard]] Digest digest() const;

    //=========================================================================================================
    /**
     * Starts a new, empty message.
     */
    void reset();

    //=========================================================================================================
    /**
     * @param[in] data   The message.
     *
     * @return The digest of @p data.
     */
    [[nodiscard]] static Digest hash(std::span<const std::byte> data);

    //=========================================================================================================
    /**
     * @param[in] text   The message.
     *
     * @return The digest of the bytes of @p text.
     */
    [[nodiscard]] static Digest hash(std::string_view text);

private:
    void compress(const std::byte* block);

    std::array<std::uint32_t, 8> m_state{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::array<std::byte, 64> m_block{};
    std::size_t m_blockSize = 0;
    std::uint64_t m_length = 0;
};

} // namespace mne

#endif // BASE_SHA256_H
