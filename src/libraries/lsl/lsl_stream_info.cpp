//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_info.cpp
 * @since March 2026
 * @brief Implements stream_info construction, accessors and the wire serialisation used by LSL discovery datagrams.
 *
 * The constructors populate the two transport-identity fields that
 * cannot be supplied by the caller: a per-instance @c uid generated
 * via @c QUuid::createUuid (the same UUID format used by liblsl) and
 * the originating @c hostname obtained from @c QHostInfo. The data
 * port and data host stay zero/empty until either the outlet binds a
 * TCP server (and writes the bound port back in) or the discovery
 * code observes the stream on the wire (and writes the sender
 * address back in).
 *
 * The @c to_string / @c from_string pair defines the on-wire
 * representation broadcast over UDP multicast and stored by
 * applications that want to remember a stream across runs. The
 * format is a deliberately minimal, line-oriented key=value
 * encoding rather than the XML used by upstream liblsl: it is
 * trivial to parse without an external dependency, fits comfortably
 * in a single UDP datagram, and round-trips every field exposed by
 * the public accessors.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_info.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHostInfo>
#include <QUuid>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <sstream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_info::stream_info()
: m_name()
, m_type()
, m_channel_count(0)
, m_nominal_srate(0.0)
, m_channel_format(ChannelFormat::Undefined)
, m_source_id()
, m_uid()
, m_hostname()
, m_data_port(0)
, m_data_host()
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
, m_uid(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString())
, m_hostname(QHostInfo::localHostName().toStdString())
, m_data_port(0)
, m_data_host()
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

std::string stream_info::to_string() const
{
    // Pipe-delimited format:
    // MNELSL1|name|type|channel_count|nominal_srate|uid|hostname|source_id|data_port|channel_format
    std::ostringstream oss;
    oss << "MNELSL1"
        << "|" << m_name
        << "|" << m_type
        << "|" << m_channel_count
        << "|" << m_nominal_srate
        << "|" << m_uid
        << "|" << m_hostname
        << "|" << m_source_id
        << "|" << m_data_port
        << "|" << static_cast<int>(m_channel_format);
    return oss.str();
}

//=============================================================================================================

stream_info stream_info::from_string(const std::string& data)
{
    // Parse pipe-delimited format
    std::istringstream iss(data);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, '|')) {
        tokens.push_back(token);
    }

    // Validate: need at least 10 fields (header + 9 data fields)
    if (tokens.size() < 10 || tokens[0] != "MNELSL1") {
        return stream_info();  // return invalid/empty
    }

    stream_info info;
    info.m_name          = tokens[1];
    info.m_type          = tokens[2];
    info.m_channel_count = std::stoi(tokens[3]);
    info.m_nominal_srate = std::stod(tokens[4]);
    info.m_uid           = tokens[5];
    info.m_hostname      = tokens[6];
    info.m_source_id     = tokens[7];
    info.m_data_port     = std::stoi(tokens[8]);
    info.m_channel_format = static_cast<ChannelFormat>(std::stoi(tokens[9]));

    return info;
}
