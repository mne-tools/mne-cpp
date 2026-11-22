//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     rtserver_client.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>
 * @since    0.1.0
 * @date     December, 2012
 * @brief    The mne_rt_server protocol over TCP.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver_transport.h"
#include "../transport_error.h"
#include <mne/com/tcp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <format>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;
using mne::TcpSocket;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr std::size_t kTagHeader = 16;
constexpr std::uint32_t kMaxTag = 1U << 30; // a corrupt size must not allocate gigabytes

void put_big(std::vector<std::byte>& out, std::uint32_t value, std::size_t bytes)
{
    for (std::size_t i = bytes; i-- > 0;) {
        out.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFFU));
    }
}

std::uint32_t get_big(std::span<const std::byte> data, std::size_t offset, std::size_t bytes)
{
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < bytes; ++i) {
        value = (value << 8) | std::to_integer<std::uint32_t>(data[offset + i]);
    }
    return value;
}

/** @return @p text, UTF-8, as big-endian UTF-16. */
std::vector<std::byte> utf16(std::string_view text)
{
    std::vector<std::byte> out;
    out.reserve(2 * text.size());
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
            put_big(out, 0xD800 + (code >> 10), 2);
            put_big(out, 0xDC00 + (code & 0x3FF), 2);
        } else {
            put_big(out, code, 2);
        }
    }
    return out;
}

/** @return Big-endian UTF-16 as UTF-8. */
std::string utf8(std::span<const std::byte> text)
{
    std::string out;
    out.reserve(text.size() / 2);
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        std::uint32_t code = get_big(text, i, 2);
        if (code >= 0xD800 && code < 0xDC00 && i + 3 < text.size()) {
            code = 0x10000 + ((code - 0xD800) << 10) + (get_big(text, i + 2, 2) - 0xDC00);
            i += 2;
        }
        const auto byte = [](std::uint32_t v) {
            return static_cast<char>(v);
        };
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

class RtServerClient final : public RtServerTransport
{
public:
    RtServerClient(TcpSocket commands, TcpSocket data, std::string name)
    : m_commands(std::move(commands))
    , m_data(std::move(data))
    , m_name(std::move(name))
    {
    }

    std::string description() const override
    {
        return m_name;
    }

    AcqExpected<std::string> command(std::string_view line, std::chrono::milliseconds timeout) override
    {
        // Replies to earlier commands, e.g. a second one to "start", belong to those
        while (true) {
            auto stale = reply(std::chrono::milliseconds(0));
            if (!stale) {
                if (stale.error() != AcqErrorCode::Timeout) {
                    return stale.error();
                }
                break;
            }
        }
        const std::vector<std::byte> text = utf16(line);
        std::vector<std::byte> frame;
        put_big(frame, static_cast<std::uint32_t>(4 + text.size()), 2);
        put_big(frame, static_cast<std::uint32_t>(text.size()), 4);
        frame.insert(frame.end(), text.begin(), text.end());
        if (const std::error_code e = m_commands.write(frame)) {
            return transport_error(e, AcqErrorCode::DeviceIo, std::format("cannot send to {}", m_name));
        }
        return reply(timeout);
    }

    AcqError send(std::int32_t command, std::string_view text) override
    {
        std::vector<std::byte> tag;
        put_big(tag, static_cast<std::uint32_t>(rt::kCommand), 4);
        put_big(tag, static_cast<std::uint32_t>(fiff::kTypeVoid), 4);
        put_big(tag, static_cast<std::uint32_t>(4 + text.size()), 4);
        put_big(tag, 0, 4);
        put_big(tag, static_cast<std::uint32_t>(command), 4);
        const auto* chars = reinterpret_cast<const std::byte*>(text.data());
        tag.insert(tag.end(), chars, chars + text.size());
        return transport_error(m_data.write(tag), AcqErrorCode::DeviceIo, std::format("cannot send to {}", m_name));
    }

    AcqExpected<const NeuromagTag*> next(std::chrono::milliseconds timeout) override
    {
        std::array<std::byte, kTagHeader> header{};
        if (const std::error_code e = m_data.read(header, timeout)) {
            return transport_error(e, AcqErrorCode::DeviceIo, std::format("{} failed", m_name));
        }
        const std::uint32_t size = get_big(header, 8, 4);
        if (size > kMaxTag) {
            return AcqError(AcqErrorCode::DeviceIo, std::format("{} sent a tag of {} bytes", m_name, size));
        }
        m_tag.kind = static_cast<int>(get_big(header, 0, 4));
        m_tag.type = static_cast<int>(get_big(header, 4, 4));
        m_tag.data.resize(size);
        if (m_data.read(m_tag.data, std::chrono::milliseconds(5000))) {
            return AcqError(AcqErrorCode::DeviceIo, std::format("{} sent an incomplete tag", m_name));
        }
        big_to_native(m_tag.type, m_tag.data);
        return &m_tag;
    }

private:
    /** @return The text of the next reply frame on the command port. */
    AcqExpected<std::string> reply(std::chrono::milliseconds timeout)
    {
        std::array<std::byte, 2> size{};
        if (const std::error_code e = m_commands.read(size, timeout)) {
            return transport_error(e, AcqErrorCode::DeviceIo, std::format("{} failed", m_name));
        }
        std::vector<std::byte> frame(get_big(size, 0, 2));
        if (m_commands.read(frame, std::chrono::milliseconds(5000))) {
            return AcqError(AcqErrorCode::DeviceIo, std::format("{} sent an incomplete reply", m_name));
        }
        if (frame.size() < 4) {
            return std::string();
        }
        const std::uint32_t length = get_big(frame, 0, 4); // 0xFFFFFFFF for a null string
        return utf8(std::span<const std::byte>(frame).subspan(4, std::min<std::size_t>(length, frame.size() - 4)));
    }

    TcpSocket m_commands;
    TcpSocket m_data;
    std::string m_name;
    NeuromagTag m_tag;
};

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

AcqExpected<std::unique_ptr<RtServerTransport>> mne::detail::connect_rtserver(const std::string& host, std::uint16_t port,
                                                                              std::chrono::milliseconds timeout)
{
    if (port == 65535) {
        return AcqError(AcqErrorCode::InvalidParameter, "the data port follows the command port; 65535 leaves none");
    }
    auto commands = TcpSocket::connect(host, port, timeout);
    if (!commands) {
        return AcqError(AcqErrorCode::DeviceNotFound, std::format("no mne_rt_server: cannot connect to {}:{} ({})", host, port, commands.error().message()));
    }
    auto data = TcpSocket::connect(host, static_cast<std::uint16_t>(port + 1), timeout);
    if (!data) {
        return AcqError(AcqErrorCode::DeviceNotFound,
                        std::format("no mne_rt_server data port: cannot connect to {}:{} ({})", host, port + 1, data.error().message()));
    }
    return std::unique_ptr<RtServerTransport>(std::make_unique<RtServerClient>(std::move(*commands), std::move(*data), std::format("{}:{}", host, port)));
}
