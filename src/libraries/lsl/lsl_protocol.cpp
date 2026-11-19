//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_protocol.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Encoding and decoding of the liblsl wire format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_protocol.h"

#include <base/log.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <span>
#include <sstream>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

template<typename T>
void appendValue(std::string& out, T value)
{
    out.append(reinterpret_cast<const char*>(&value), sizeof(T));
}

template<typename T>
T loadValue(const char* at, bool reverse) noexcept
{
    char bytes[sizeof(T)];
    std::memcpy(bytes, at, sizeof(T));
    if (reverse) {
        std::reverse(bytes, bytes + sizeof(T));
    }
    T value;
    std::memcpy(&value, bytes, sizeof(T));
    return value;
}

template<typename T>
T rounded(double value) noexcept
{
    return static_cast<T>(std::llround(value));
}

//=============================================================================================================

std::string trim(const std::string& text)
{
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

/** @return The text of field @p key of @p info as the shortinfo XML spells it, false if not a known field. */
bool field(const stream_info& info, const std::string& key, std::string& value)
{
    if (key == "name") {
        value = info.name();
    } else if (key == "type") {
        value = info.type();
    } else if (key == "source_id") {
        value = info.source_id();
    } else if (key == "uid") {
        value = info.uid();
    } else if (key == "hostname") {
        value = info.hostname();
    } else if (key == "session_id") {
        value = "default";
    } else if (key == "channel_count") {
        value = std::to_string(info.channel_count());
    } else if (key == "channel_format") {
        value = formatName(info.channel_format());
    } else {
        return false;
    }
    return true;
}


} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::vector<std::string> mne::detail::discoveryTargets()
{
    return {"239.255.172.215", "224.0.0.183", "255.255.255.255", kLoopback};
}

//=============================================================================================================

std::vector<std::string> mne::detail::discoveryGroups()
{
    return {"239.255.172.215", "224.0.0.183"};
}

//=============================================================================================================

void mne::detail::joinDiscoveryGroups(UdpSocket& socket)
{
    // Either interface may lack a multicast route (WebAssembly has none at all); one membership is enough
    int joined = 0;
    for (const std::string& group : discoveryGroups()) {
        for (const std::string& networkInterface : {std::string(), std::string(kLoopback)}) {
            joined += socket.join_group(group, networkInterface) ? 0 : 1;
        }
    }
    if (joined == 0) {
        Log::debug("[lsl] No discovery group could be joined; only queries by broadcast and to localhost arrive");
    }
}

//=============================================================================================================

void mne::detail::sendQuery(UdpSocket& socket, std::string_view query)
{
    const auto bytes = std::as_bytes(std::span(query));
    int sent = 0;
    if (!socket.set_multicast_interface()) {
        for (const std::string& target : discoveryTargets()) {
            sent += socket.send_to(bytes, target, kDiscoveryPort) ? 0 : 1;
        }
    }
    // Over loopback a group query reaches every local outlet, which share the discovery port
    if (!socket.set_multicast_interface(kLoopback)) {
        sent += socket.send_to(bytes, discoveryGroups().front(), kDiscoveryPort) ? 0 : 1;
    }
    if (sent == 0) {
        Log::debug("[lsl] The query could not be sent anywhere");
    }
}

//=============================================================================================================

std::size_t mne::detail::valueSize(ChannelFormat format) noexcept
{
    switch (format) {
        case ChannelFormat::Float32:
        case ChannelFormat::Int32:
            return 4;
        case ChannelFormat::Double64:
        case ChannelFormat::Int64:
            return 8;
        case ChannelFormat::Int16:
            return 2;
        case ChannelFormat::Int8:
            return 1;
        default:
            return 0;
    }
}

//=============================================================================================================

std::string mne::detail::formatName(ChannelFormat format)
{
    switch (format) {
        case ChannelFormat::Float32:
            return "float32";
        case ChannelFormat::Double64:
            return "double64";
        case ChannelFormat::String:
            return "string";
        case ChannelFormat::Int32:
            return "int32";
        case ChannelFormat::Int16:
            return "int16";
        case ChannelFormat::Int8:
            return "int8";
        case ChannelFormat::Int64:
            return "int64";
        default:
            return "undefined";
    }
}

//=============================================================================================================

ChannelFormat mne::detail::formatFromName(const std::string& name)
{
    for (ChannelFormat format : {ChannelFormat::Float32, ChannelFormat::Double64, ChannelFormat::String, ChannelFormat::Int32,
                                 ChannelFormat::Int16, ChannelFormat::Int8, ChannelFormat::Int64}) {
        if (formatName(format) == name) {
            return format;
        }
    }
    return ChannelFormat::Undefined;
}

//=============================================================================================================

bool mne::detail::littleEndian() noexcept
{
    return std::endian::native == std::endian::little;
}

//=============================================================================================================

const char* mne::detail::byteOrderName() noexcept
{
    return littleEndian() ? "1234" : "4321";
}

//=============================================================================================================

double mne::detail::testPatternValue(ChannelFormat format, int k, int offset) noexcept
{
    const double sign = k % 2 == 0 ? 1.0 : -1.0;
    switch (format) {
        case ChannelFormat::Double64:
            return (k + offset + 16777217.0) * sign;
        case ChannelFormat::String:
            return (k + 10) * sign;
        case ChannelFormat::Int32:
            return (k + 65537.0 + offset) * sign;
        case ChannelFormat::Int16:
            return (k + 257.0 + offset) * sign;
        case ChannelFormat::Int8:
            return (k + 1.0 + offset) * sign;
        case ChannelFormat::Int64:
            return (k + 2147483649.0 + offset) * sign;
        default:
            return (k + offset) * sign;
    }
}

//=============================================================================================================

void mne::detail::appendSample(std::string& out, ChannelFormat format, const std::vector<double>& values, double timestamp,
                               bool deduced)
{
    if (deduced) {
        appendValue(out, kTagDeduced);
    } else {
        appendValue(out, kTagTransmitted);
        appendValue(out, timestamp);
    }
    for (const double v : values) {
        switch (format) {
            case ChannelFormat::Float32:
                appendValue(out, static_cast<float>(v));
                break;
            case ChannelFormat::Double64:
                appendValue(out, v);
                break;
            case ChannelFormat::Int32:
                appendValue(out, rounded<std::int32_t>(v));
                break;
            case ChannelFormat::Int16:
                appendValue(out, rounded<std::int16_t>(v));
                break;
            case ChannelFormat::Int8:
                appendValue(out, rounded<std::int8_t>(v));
                break;
            case ChannelFormat::Int64:
                appendValue(out, static_cast<std::int64_t>(std::llround(v)));
                break;
            case ChannelFormat::String: {
                std::ostringstream text;
                text.precision(17);
                text << v;
                const std::string s = text.str();
                appendValue(out, std::uint8_t(1));
                appendValue(out, static_cast<std::uint8_t>(s.size()));
                out.append(s);
                break;
            }
            default:
                break;
        }
    }
}

//=============================================================================================================

SampleReader::SampleReader(ChannelFormat format, int channels, bool reverseBytes) noexcept
: m_format(format)
, m_channels(channels)
, m_reverse(reverseBytes)
{
}

//=============================================================================================================

int SampleReader::read(std::string_view data, std::size_t& pos, std::uint8_t& tag, double& timestamp,
                       std::vector<double>& values) const
{
    const char* base = data.data();
    std::size_t at = pos;
    const auto need = [&](std::uint64_t n) {
        return data.size() >= at && data.size() - at >= n;
    };
    if (!need(1)) {
        return 0;
    }
    tag = static_cast<std::uint8_t>(base[at++]);
    if (tag == kTagTransmitted) {
        if (!need(8)) {
            return 0;
        }
        timestamp = loadValue<double>(base + at, m_reverse);
        at += 8;
    } else if (tag != kTagDeduced) {
        return -1;
    }
    values.resize(static_cast<std::size_t>(m_channels));
    if (m_format == ChannelFormat::String) {
        for (double& v : values) {
            if (!need(1)) {
                return 0;
            }
            const auto lengthSize = static_cast<std::uint8_t>(base[at++]);
            if (lengthSize != 1 && lengthSize != 4 && lengthSize != 8) {
                return -1;
            }
            if (!need(lengthSize)) {
                return 0;
            }
            std::uint64_t length = 0;
            if (lengthSize == 1) {
                length = static_cast<std::uint8_t>(base[at]);
            } else if (lengthSize == 4) {
                length = loadValue<std::uint32_t>(base + at, m_reverse);
            } else {
                length = loadValue<std::uint64_t>(base + at, m_reverse);
            }
            at += lengthSize;
            if (!need(length)) {
                return 0;
            }
            v = std::strtod(std::string(base + at, static_cast<std::size_t>(length)).c_str(), nullptr);
            at += static_cast<std::size_t>(length);
        }
    } else {
        const std::size_t size = valueSize(m_format);
        if (size == 0) {
            return -1;
        }
        if (!need(size * static_cast<std::size_t>(m_channels))) {
            return 0;
        }
        for (double& v : values) {
            switch (m_format) {
                case ChannelFormat::Float32:
                    v = loadValue<float>(base + at, m_reverse);
                    break;
                case ChannelFormat::Double64:
                    v = loadValue<double>(base + at, m_reverse);
                    break;
                case ChannelFormat::Int32:
                    v = loadValue<std::int32_t>(base + at, m_reverse);
                    break;
                case ChannelFormat::Int16:
                    v = loadValue<std::int16_t>(base + at, m_reverse);
                    break;
                case ChannelFormat::Int8:
                    v = loadValue<std::int8_t>(base + at, m_reverse);
                    break;
                default:
                    v = static_cast<double>(loadValue<std::int64_t>(base + at, m_reverse));
                    break;
            }
            at += size;
        }
    }
    pos = at;
    return 1;
}

//=============================================================================================================

bool mne::detail::matchesQuery(const stream_info& info, const std::string& predicate)
{
    std::string rest = trim(predicate);
    if (rest.empty()) {
        return true;
    }
    while (!rest.empty()) {
        const auto eq = rest.find('=');
        if (eq == std::string::npos || eq + 1 >= rest.size() || rest[eq + 1] != '\'') {
            return false;
        }
        const std::string key = trim(rest.substr(0, eq));
        const auto close = rest.find('\'', eq + 2);
        if (close == std::string::npos) {
            return false;
        }
        const std::string wanted = rest.substr(eq + 2, close - eq - 2);
        std::string actual;
        if (key == "nominal_srate") {
            char* end = nullptr;
            const double rate = std::strtod(wanted.c_str(), &end);
            if (end == wanted.c_str() || rate != info.nominal_srate()) {
                return false;
            }
        } else if (!field(info, key, actual) || actual != wanted) {
            return false;
        }
        rest = trim(rest.substr(close + 1));
        if (rest.empty()) {
            break;
        }
        if (rest.compare(0, 4, "and ") != 0) {
            return false;
        }
        rest = trim(rest.substr(4));
    }
    return true;
}

//=============================================================================================================

std::expected<UdpSocket, std::error_code> mne::detail::bindQuerySocket(std::uint16_t port)
{
    return UdpSocket::bind("0.0.0.0", port);
}
