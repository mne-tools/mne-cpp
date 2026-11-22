//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_info.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implements stream_info construction, accessors and its shortinfo XML, as liblsl exchanges it.
 *
 * The constructors populate the transport-identity fields the caller cannot supply: a per-instance @c uid
 * (a UUID, as in liblsl), the originating @c hostname and the creation time. The data and service ports and
 * the data host stay empty until an outlet binds its sockets or discovery observes the stream on the wire.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_info.h"
#include "lsl_protocol.h"

#include <mne/base/process.h>
#include <mne/base/string_utils.h>
#include <mne/base/xml.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
#include <chrono>
#include <mutex>
#include <cstdint>
#include <format>
#include <random>
#include <sstream>
#include <utility>

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

/** @return A random (version 4) UUID in lower-case hex with dashes, as liblsl gives every stream. */
std::string randomUuid()
{
    static std::mt19937_64 generator{std::random_device{}()};
    static std::mutex mutex;
    std::array<std::uint64_t, 2> words{};
    {
        const std::lock_guard lock(mutex);
        words = {generator(), generator()};
    }
    words[0] = (words[0] & ~std::uint64_t{0xF000}) | std::uint64_t{0x4000};           // version 4
    words[1] = (words[1] & ~(std::uint64_t{0xC} << 60)) | (std::uint64_t{0x8} << 60); // RFC 4122 variant
    return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", words[0] >> 32, (words[0] >> 16) & 0xFFFF, words[0] & 0xFFFF,
                       words[1] >> 48, words[1] & 0xFFFFFFFFFFFFULL);
}

std::string escaped(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&apos;";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

//=============================================================================================================

std::string number(double value)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text.precision(17);
    text << value;
    return text.str();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_info::stream_info()
: m_channel_count(0)
, m_nominal_srate(0.0)
, m_channel_format(ChannelFormat::Undefined)
, m_data_port(0)
{
}

//=============================================================================================================

stream_info::stream_info(const std::string& name,
                         const std::string& type,
                         int channel_count,
                         double nominal_srate,
                         ChannelFormat channel_format,
                         const std::string& source_id)
: m_name(name)
, m_type(type)
, m_channel_count(channel_count)
, m_nominal_srate(nominal_srate)
, m_channel_format(channel_format)
, m_source_id(source_id)
, m_uid(randomUuid())
, m_hostname(host_name())
, m_data_port(0)
, m_created_at(local_clock())
{
}

//=============================================================================================================

std::string stream_info::name() const noexcept
{
    return m_name;
}

//=============================================================================================================

std::string stream_info::type() const noexcept
{
    return m_type;
}

//=============================================================================================================

int stream_info::channel_count() const noexcept
{
    return m_channel_count;
}

//=============================================================================================================

double stream_info::nominal_srate() const noexcept
{
    return m_nominal_srate;
}

//=============================================================================================================

ChannelFormat stream_info::channel_format() const noexcept
{
    return m_channel_format;
}

//=============================================================================================================

std::string stream_info::source_id() const noexcept
{
    return m_source_id;
}

//=============================================================================================================

std::string stream_info::uid() const noexcept
{
    return m_uid;
}

//=============================================================================================================

std::string stream_info::hostname() const noexcept
{
    return m_hostname;
}

//=============================================================================================================

int stream_info::data_port() const noexcept
{
    return m_data_port;
}

//=============================================================================================================

std::string stream_info::data_host() const noexcept
{
    return m_data_host;
}

//=============================================================================================================

void stream_info::set_data_port(int port)
{
    m_data_port = port;
}

//=============================================================================================================

void stream_info::set_data_host(const std::string& host)
{
    m_data_host = host;
}

//=============================================================================================================

int stream_info::service_port() const noexcept
{
    return m_service_port;
}

//=============================================================================================================

void stream_info::set_service_port(int port)
{
    m_service_port = port;
}

//=============================================================================================================

double stream_info::created_at() const noexcept
{
    return m_created_at;
}

//=============================================================================================================

const std::vector<channel_description>& stream_info::channels() const noexcept
{
    return m_channels;
}

//=============================================================================================================

void stream_info::set_channels(std::vector<channel_description> channels)
{
    m_channels = std::move(channels);
}

//=============================================================================================================

std::string stream_info::to_string() const
{
    return xml(false);
}

//=============================================================================================================

std::string stream_info::as_xml() const
{
    return xml(true);
}

//=============================================================================================================

std::string stream_info::xml(bool full) const
{
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\"?>\n<info>\n"
        << "\t<name>" << escaped(m_name) << "</name>\n"
        << "\t<type>" << escaped(m_type) << "</type>\n"
        << "\t<channel_count>" << m_channel_count << "</channel_count>\n"
        << "\t<channel_format>" << formatName(m_channel_format) << "</channel_format>\n"
        << "\t<source_id>" << escaped(m_source_id) << "</source_id>\n"
        << "\t<nominal_srate>" << number(m_nominal_srate) << "</nominal_srate>\n"
        << "\t<version>1.1</version>\n"
        << "\t<created_at>" << number(m_created_at) << "</created_at>\n"
        << "\t<uid>" << escaped(m_uid) << "</uid>\n"
        << "\t<session_id>default</session_id>\n"
        << "\t<hostname>" << escaped(m_hostname) << "</hostname>\n"
        << "\t<v4address></v4address>\n"
        << "\t<v4data_port>" << m_data_port << "</v4data_port>\n"
        << "\t<v4service_port>" << m_service_port << "</v4service_port>\n"
        << "\t<v6address></v6address>\n"
        << "\t<v6data_port>0</v6data_port>\n"
        << "\t<v6service_port>0</v6service_port>\n";
    if (!full || m_channels.empty()) {
        xml << "\t<desc />\n</info>\n";
        return xml.str();
    }
    xml << "\t<desc>\n\t\t<channels>\n";
    for (const channel_description& channel : m_channels) {
        xml << "\t\t\t<channel>\n"
            << "\t\t\t\t<label>" << escaped(channel.label) << "</label>\n"
            << "\t\t\t\t<type>" << escaped(channel.type) << "</type>\n"
            << "\t\t\t\t<unit>" << escaped(channel.unit) << "</unit>\n"
            << "\t\t\t</channel>\n";
    }
    xml << "\t\t</channels>\n\t</desc>\n</info>\n";
    return xml.str();
}

//=============================================================================================================

stream_info stream_info::from_string(const std::string& data)
{
    stream_info info;
    const Expected<XmlElement> root = parse_xml(data);
    if (!root || root->local_name() != "info") {
        return {};
    }
    for (const XmlElement& element : root->children) {
        const std::string_view key = element.local_name();
        const std::string& value = element.text;
        if (key == "name") {
            info.m_name = value;
        } else if (key == "type") {
            info.m_type = value;
        } else if (key == "channel_count") {
            info.m_channel_count = to_int(value).value_or(0);
        } else if (key == "channel_format") {
            info.m_channel_format = formatFromName(value);
        } else if (key == "source_id") {
            info.m_source_id = value;
        } else if (key == "nominal_srate") {
            info.m_nominal_srate = to_double(value).value_or(0.0);
        } else if (key == "created_at") {
            info.m_created_at = to_double(value).value_or(0.0);
        } else if (key == "uid") {
            info.m_uid = value;
        } else if (key == "hostname") {
            info.m_hostname = value;
        } else if (key == "v4data_port") {
            info.m_data_port = to_int(value).value_or(0);
        } else if (key == "v4service_port") {
            info.m_service_port = to_int(value).value_or(0);
        } else if (key == "v4address" && !value.empty()) {
            info.m_data_host = value;
        } else if (key == "desc") {
            if (const XmlElement* channels = element.child("channels")) {
                for (const XmlElement& channel : channels->children) {
                    if (channel.local_name() != "channel") {
                        continue;
                    }
                    channel_description description;
                    for (const XmlElement& property : channel.children) {
                        const std::string_view tag = property.local_name();
                        if (tag == "label") {
                            description.label = property.text;
                        } else if (tag == "type") {
                            description.type = property.text;
                        } else if (tag == "unit") {
                            description.unit = property.text;
                        }
                    }
                    info.m_channels.push_back(std::move(description));
                }
            }
        }
    }
    if (info.m_name.empty()) {
        return {};
    }
    return info;
}

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

double mne::local_clock()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
