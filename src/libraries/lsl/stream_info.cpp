//=============================================================================================================
/**
 * @file     stream_info.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the definition of the stream_info class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stream_info.h"

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

using namespace lsl;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_info::stream_info()
: m_name()
, m_type()
, m_channel_count(0)
, m_nominal_srate(0.0)
, m_channel_format(cf_undefined)
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
                         channel_format_t channel_format,
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

std::string stream_info::name() const
{
    return m_name;
}

//=============================================================================================================

std::string stream_info::type() const
{
    return m_type;
}

//=============================================================================================================

int stream_info::channel_count() const
{
    return m_channel_count;
}

//=============================================================================================================

double stream_info::nominal_srate() const
{
    return m_nominal_srate;
}

//=============================================================================================================

channel_format_t stream_info::channel_format() const
{
    return m_channel_format;
}

//=============================================================================================================

std::string stream_info::source_id() const
{
    return m_source_id;
}

//=============================================================================================================

std::string stream_info::uid() const
{
    return m_uid;
}

//=============================================================================================================

std::string stream_info::hostname() const
{
    return m_hostname;
}

//=============================================================================================================

int stream_info::data_port() const
{
    return m_data_port;
}

//=============================================================================================================

std::string stream_info::data_host() const
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
    info.m_channel_format = static_cast<channel_format_t>(std::stoi(tokens[9]));

    return info;
}
