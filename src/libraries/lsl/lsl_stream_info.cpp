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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHostInfo>
#include <QUuid>
#include <QXmlStreamReader>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
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
, m_uid(QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString())
, m_hostname(QHostInfo::localHostName().toStdString())
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

std::string stream_info::to_string() const
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
        << "\t<v6service_port>0</v6service_port>\n"
        << "\t<desc />\n</info>\n";
    return xml.str();
}

//=============================================================================================================

stream_info stream_info::from_string(const std::string& data)
{
    stream_info info;
    QXmlStreamReader xml(QByteArray::fromStdString(data));
    if (!xml.readNextStartElement() || xml.name() != QLatin1String("info")) {
        return {};
    }
    while (xml.readNextStartElement()) {
        const QString key = xml.name().toString();
        if (key == QLatin1String("desc")) {
            xml.skipCurrentElement();
            continue;
        }
        const QString text = xml.readElementText(QXmlStreamReader::SkipChildElements);
        const std::string value = text.toStdString();
        if (key == QLatin1String("name")) {
            info.m_name = value;
        } else if (key == QLatin1String("type")) {
            info.m_type = value;
        } else if (key == QLatin1String("channel_count")) {
            info.m_channel_count = text.toInt();
        } else if (key == QLatin1String("channel_format")) {
            info.m_channel_format = formatFromName(value);
        } else if (key == QLatin1String("source_id")) {
            info.m_source_id = value;
        } else if (key == QLatin1String("nominal_srate")) {
            info.m_nominal_srate = text.toDouble();
        } else if (key == QLatin1String("created_at")) {
            info.m_created_at = text.toDouble();
        } else if (key == QLatin1String("uid")) {
            info.m_uid = value;
        } else if (key == QLatin1String("hostname")) {
            info.m_hostname = value;
        } else if (key == QLatin1String("v4data_port")) {
            info.m_data_port = text.toInt();
        } else if (key == QLatin1String("v4service_port")) {
            info.m_service_port = text.toInt();
        } else if (key == QLatin1String("v4address") && !value.empty()) {
            info.m_data_host = value;
        }
    }
    if (xml.hasError() || info.m_name.empty()) {
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
