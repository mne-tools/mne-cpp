//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandthread.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandThread Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandthread.h"

#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

namespace
{

/** QDataStream's byte count of a null QString. */
constexpr std::uint32_t kNullString = 0xFFFFFFFFU;

/** Appends the @p bytes lowest bytes of @p value to @p out, big-endian. */
void putBig(std::vector<std::byte>& out, std::uint32_t value, int bytes)
{
    for (int i = bytes - 1; i >= 0; --i) {
        out.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFU));
    }
}

/** @return The @p bytes bytes at @p offset of @p data as big-endian number. */
std::uint32_t getBig(std::span<const std::byte> data, std::size_t offset, int bytes)
{
    std::uint32_t value = 0;
    for (int i = 0; i < bytes; ++i) {
        value = (value << 8) | std::to_integer<std::uint32_t>(data[offset + static_cast<std::size_t>(i)]);
    }
    return value;
}

/** Appends @p text, UTF-8, to @p out as big-endian UTF-16. */
void putUtf16(std::vector<std::byte>& out, std::string_view text)
{
    for (std::size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i]);
        const std::size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2
            : lead < 0xF0                                        ? 3
                                                                 : 4;
        std::uint32_t code = length == 1 ? lead : lead & (0x7FU >> length);
        for (std::size_t k = 1; k < length && i + k < text.size(); ++k) {
            code = (code << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3FU);
        }
        i += length;
        if (code >= 0x10000) {
            code -= 0x10000;
            putBig(out, 0xD800 + (code >> 10), 2);
            putBig(out, 0xDC00 + (code & 0x3FF), 2);
        } else {
            putBig(out, code, 2);
        }
    }
}

/** @return Big-endian UTF-16 as UTF-8. */
std::string fromUtf16(std::span<const std::byte> text)
{
    std::string out;
    out.reserve(text.size() / 2);
    const auto byte = [](std::uint32_t v) {
        return static_cast<char>(v);
    };
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        std::uint32_t code = getBig(text, i, 2);
        if (code >= 0xD800 && code < 0xDC00 && i + 3 < text.size()) {
            code = 0x10000 + ((code - 0xD800) << 10) + (getBig(text, i + 2, 2) - 0xDC00);
            i += 2;
        }
        if (code < 0x80) {
            out += byte(code);
        } else if (code < 0x800) {
            out += {byte(0xC0 | (code >> 6)), byte(0x80 | (code & 0x3F))};
        } else if (code < 0x10000) {
            out += {byte(0xE0 | (code >> 12)), byte(0x80 | ((code >> 6) & 0x3F)), byte(0x80 | (code & 0x3F))};
        } else {
            out += {byte(0xF0 | (code >> 18)), byte(0x80 | ((code >> 12) & 0x3F)), byte(0x80 | ((code >> 6) & 0x3F)), byte(0x80 | (code & 0x3F))};
        }
    }
    return out;
}

/** @return @p text without leading and trailing whitespace and with inner whitespace runs as one space (QString::simplified). */
std::string simplified(std::string_view text)
{
    std::string out;
    for (const std::string_view field : mne::split_whitespace(text)) {
        if (!out.empty()) {
            out += ' ';
        }
        out += field;
    }
    return out;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandThread::CommandThread(mne::TcpSocket socket, std::int32_t p_iId, CommandCallback onCommand)
: m_socket(std::move(socket))
, m_bIsRunning(false)
, m_bIsFinished(false)
, m_iThreadID(p_iId)
, m_onCommand(std::move(onCommand))
{
}

//=============================================================================================================

CommandThread::~CommandThread()
{
    stop();
}

//=============================================================================================================

void CommandThread::attachCommandReply(const std::string& p_blockReply, std::int32_t p_iID)
{
    mne::Log::debug("CommandThread::attachCommandReply");
    if (p_iID == m_iThreadID) {
        const std::scoped_lock lock(m_qMutex);
        m_qSendData = p_blockReply;
    }
}

//=============================================================================================================

void CommandThread::start()
{
    m_bIsRunning = true;
    m_thread = std::thread([this] { run(); });
}

//=============================================================================================================

void CommandThread::stop()
{
    m_bIsRunning = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

//=============================================================================================================

bool CommandThread::isFinished() const
{
    return m_bIsFinished;
}

//=============================================================================================================

void CommandThread::run()
{
    mne::Log::info("CommandClient connection accepted (ID {})\n", m_iThreadID);

    std::vector<std::byte> t_received;
    std::array<std::byte, 4096> t_chunk{};

    while (m_socket.is_open() && m_bIsRunning) {
        //
        // Write available data
        //
        std::string t_sReply;
        {
            const std::scoped_lock lock(m_qMutex);
            t_sReply.swap(m_qSendData);
        }
        if (!t_sReply.empty()) {
            std::vector<std::byte> t_text;
            putUtf16(t_text, t_sReply);
            std::vector<std::byte> block;
            putBig(block, static_cast<std::uint16_t>(sizeof(std::uint32_t) + t_text.size()), 2);
            putBig(block, static_cast<std::uint32_t>(t_text.size()), 4);
            block.insert(block.end(), t_text.begin(), t_text.end());

            if (m_socket.write(block)) {
                break;
            }
        }

        //
        // Read: Wait 100ms for incomming data, parse complete frames and continue
        //
        const auto t_read = m_socket.read_some(t_chunk, std::chrono::milliseconds(100));
        if (t_read) {
            t_received.insert(t_received.end(), t_chunk.begin(), t_chunk.begin() + static_cast<std::ptrdiff_t>(*t_read));
        } else if (t_read.error() != std::errc::timed_out) {
            break;
        }

        while (t_received.size() >= sizeof(std::uint16_t)) {
            const std::uint32_t blockSize = getBig(t_received, 0, 2);
            if (blockSize >= 65000) { //Sanity Check -> allowed maximal blocksize is 65.000
                t_received.clear();
                break;
            }
            if (t_received.size() < sizeof(std::uint16_t) + blockSize) {
                break;
            }
            const std::span<const std::byte> t_block = std::span<const std::byte>(t_received).subspan(sizeof(std::uint16_t), blockSize);

            std::string t_sCommand;
            if (t_block.size() >= sizeof(std::uint32_t)) {
                const std::uint32_t length = getBig(t_block, 0, 4);
                if (length != kNullString && length <= t_block.size() - sizeof(std::uint32_t)) {
                    t_sCommand = simplified(fromUtf16(t_block.subspan(sizeof(std::uint32_t), length)));
                }
            }
            t_received.erase(t_received.begin(), t_received.begin() + static_cast<std::ptrdiff_t>(sizeof(std::uint16_t) + blockSize));

            //
            // Parse command
            //
            if (!t_sCommand.empty() && m_onCommand) {
                m_onCommand(t_sCommand, m_iThreadID);
            }
        }
    }

    m_socket.close();
    m_bIsFinished = true;
}
