//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sha256.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Sha256 class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sha256.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <bit>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::array<std::uint32_t, 64> K{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
    0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
    0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
    0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
    0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void Sha256::compress(const std::byte* block)
{
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (std::to_integer<std::uint32_t>(block[4 * i]) << 24) | (std::to_integer<std::uint32_t>(block[(4 * i) + 1]) << 16) |
            (std::to_integer<std::uint32_t>(block[(4 * i) + 2]) << 8) | std::to_integer<std::uint32_t>(block[(4 * i) + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
        const std::uint32_t s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const std::uint32_t s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    auto [a, b, c, d, e, f, g, h] = m_state;
    for (std::size_t i = 0; i < 64; ++i) {
        const std::uint32_t t1 = h + (std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + w[i];
        const std::uint32_t t2 = (std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}

//=============================================================================================================

Sha256& Sha256::update(std::span<const std::byte> data)
{
    m_length += data.size();
    if (m_blockSize > 0) {
        const std::size_t take = std::min(data.size(), m_block.size() - m_blockSize);
        std::ranges::copy(data.first(take), m_block.begin() + static_cast<std::ptrdiff_t>(m_blockSize));
        m_blockSize += take;
        data = data.subspan(take);
        if (m_blockSize < m_block.size())
            return *this;
        compress(m_block.data());
        m_blockSize = 0;
    }
    for (; data.size() >= m_block.size(); data = data.subspan(m_block.size()))
        compress(data.data());
    std::ranges::copy(data, m_block.begin());
    m_blockSize = data.size();
    return *this;
}

//=============================================================================================================

Sha256& Sha256::update(std::string_view text)
{
    return update(std::as_bytes(std::span(text)));
}

//=============================================================================================================

Sha256::Digest Sha256::digest() const
{
    Sha256 last = *this;
    const std::uint64_t bits = m_length * 8;
    const std::size_t padding = (m_blockSize < 56 ? 56 : 120) - m_blockSize;
    std::array<std::byte, 72> tail{};
    tail[0] = std::byte{0x80};
    for (std::size_t i = 0; i < 8; ++i)
        tail[padding + i] = static_cast<std::byte>(bits >> (56 - (8 * i)));
    last.update(std::span(tail).first(padding + 8));

    Digest out{};
    for (std::size_t i = 0; i < 8; ++i) {
        for (std::size_t k = 0; k < 4; ++k)
            out[(4 * i) + k] = static_cast<std::byte>(last.m_state[i] >> (24 - (8 * k)));
    }
    return out;
}

//=============================================================================================================

void Sha256::reset()
{
    *this = Sha256();
}

//=============================================================================================================

Sha256::Digest Sha256::hash(std::span<const std::byte> data)
{
    return Sha256().update(data).digest();
}

//=============================================================================================================

Sha256::Digest Sha256::hash(std::string_view text)
{
    return Sha256().update(text).digest();
}
