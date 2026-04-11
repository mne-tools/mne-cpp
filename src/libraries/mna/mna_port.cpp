//=============================================================================================================
/**
 * @file     mna_port.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    MnaPort struct implementation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_port.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

static QString dataKindToString(MnaDataKind kind)
{
    switch (kind) {
    case MnaDataKind::FiffRaw:          return QStringLiteral("fiff_raw");
    case MnaDataKind::Forward:          return QStringLiteral("forward");
    case MnaDataKind::Inverse:          return QStringLiteral("inverse");
    case MnaDataKind::Covariance:       return QStringLiteral("covariance");
    case MnaDataKind::SourceEstimate:   return QStringLiteral("source_estimate");
    case MnaDataKind::Epochs:           return QStringLiteral("epochs");
    case MnaDataKind::Evoked:           return QStringLiteral("evoked");
    case MnaDataKind::Matrix:           return QStringLiteral("matrix");
    case MnaDataKind::Volume:           return QStringLiteral("volume");
    case MnaDataKind::Surface:          return QStringLiteral("surface");
    case MnaDataKind::Bem:              return QStringLiteral("bem");
    case MnaDataKind::Annotation:       return QStringLiteral("annotation");
    case MnaDataKind::Label:            return QStringLiteral("label");
    case MnaDataKind::RealTimeStream:   return QStringLiteral("real_time_stream");
    case MnaDataKind::Custom:           return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

//=============================================================================================================

static MnaDataKind dataKindFromString(const QString& str)
{
    if (str == QLatin1String("fiff_raw"))           return MnaDataKind::FiffRaw;
    if (str == QLatin1String("forward"))            return MnaDataKind::Forward;
    if (str == QLatin1String("inverse"))            return MnaDataKind::Inverse;
    if (str == QLatin1String("covariance"))         return MnaDataKind::Covariance;
    if (str == QLatin1String("source_estimate"))    return MnaDataKind::SourceEstimate;
    if (str == QLatin1String("epochs"))             return MnaDataKind::Epochs;
    if (str == QLatin1String("evoked"))             return MnaDataKind::Evoked;
    if (str == QLatin1String("matrix"))             return MnaDataKind::Matrix;
    if (str == QLatin1String("volume"))             return MnaDataKind::Volume;
    if (str == QLatin1String("surface"))            return MnaDataKind::Surface;
    if (str == QLatin1String("bem"))                return MnaDataKind::Bem;
    if (str == QLatin1String("annotation"))         return MnaDataKind::Annotation;
    if (str == QLatin1String("label"))              return MnaDataKind::Label;
    if (str == QLatin1String("real_time_stream"))   return MnaDataKind::RealTimeStream;
    return MnaDataKind::Custom;
}

//=============================================================================================================

static QString portDirToString(MnaPortDir dir)
{
    switch (dir) {
    case MnaPortDir::Input:  return QStringLiteral("input");
    case MnaPortDir::Output: return QStringLiteral("output");
    }
    return QStringLiteral("input");
}

//=============================================================================================================

static MnaPortDir portDirFromString(const QString& str)
{
    if (str == QLatin1String("output")) return MnaPortDir::Output;
    return MnaPortDir::Input;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QJsonObject MnaPort::toJson() const
{
    QJsonObject json;
    json[QLatin1String("name")]      = name;
    json[QLatin1String("data_kind")] = dataKindToString(dataKind);
    json[QLatin1String("direction")] = portDirToString(direction);

    if (!sourceNodeId.isEmpty())
        json[QLatin1String("source_node")] = sourceNodeId;
    if (!sourcePortName.isEmpty())
        json[QLatin1String("source_port")] = sourcePortName;

    if (!streamProtocol.isEmpty()) {
        json[QLatin1String("stream_protocol")]  = streamProtocol;
        json[QLatin1String("stream_endpoint")]  = streamEndpoint;
        json[QLatin1String("stream_buffer_ms")] = streamBufferMs;
    }

    if (!cachedResultPath.isEmpty())
        json[QLatin1String("cached_result")]      = cachedResultPath;
    if (!cachedResultHash.isEmpty())
        json[QLatin1String("cached_result_hash")] = cachedResultHash;

    return json;
}

//=============================================================================================================

MnaPort MnaPort::fromJson(const QJsonObject& json)
{
    MnaPort port;
    port.name           = json[QLatin1String("name")].toString();
    port.dataKind       = dataKindFromString(json[QLatin1String("data_kind")].toString());
    port.direction      = portDirFromString(json[QLatin1String("direction")].toString());
    port.sourceNodeId   = json[QLatin1String("source_node")].toString();
    port.sourcePortName = json[QLatin1String("source_port")].toString();
    port.streamProtocol = json[QLatin1String("stream_protocol")].toString();
    port.streamEndpoint = json[QLatin1String("stream_endpoint")].toString();
    port.streamBufferMs = json[QLatin1String("stream_buffer_ms")].toInt(0);
    port.cachedResultPath = json[QLatin1String("cached_result")].toString();
    port.cachedResultHash = json[QLatin1String("cached_result_hash")].toString();
    return port;
}

//=============================================================================================================

QCborMap MnaPort::toCbor() const
{
    QCborMap cbor;
    cbor[QLatin1String("name")]      = name;
    cbor[QLatin1String("data_kind")] = dataKindToString(dataKind);
    cbor[QLatin1String("direction")] = portDirToString(direction);

    if (!sourceNodeId.isEmpty())
        cbor[QLatin1String("source_node")] = sourceNodeId;
    if (!sourcePortName.isEmpty())
        cbor[QLatin1String("source_port")] = sourcePortName;
    if (!streamProtocol.isEmpty()) {
        cbor[QLatin1String("stream_protocol")]  = streamProtocol;
        cbor[QLatin1String("stream_endpoint")]  = streamEndpoint;
        cbor[QLatin1String("stream_buffer_ms")] = streamBufferMs;
    }
    if (!cachedResultPath.isEmpty())
        cbor[QLatin1String("cached_result")]      = cachedResultPath;
    if (!cachedResultHash.isEmpty())
        cbor[QLatin1String("cached_result_hash")] = cachedResultHash;

    return cbor;
}

//=============================================================================================================

MnaPort MnaPort::fromCbor(const QCborMap& cbor)
{
    MnaPort port;
    port.name           = cbor[QLatin1String("name")].toString();
    port.dataKind       = dataKindFromString(cbor[QLatin1String("data_kind")].toString());
    port.direction      = portDirFromString(cbor[QLatin1String("direction")].toString());
    port.sourceNodeId   = cbor[QLatin1String("source_node")].toString();
    port.sourcePortName = cbor[QLatin1String("source_port")].toString();
    port.streamProtocol = cbor[QLatin1String("stream_protocol")].toString();
    port.streamEndpoint = cbor[QLatin1String("stream_endpoint")].toString();
    port.streamBufferMs = cbor[QLatin1String("stream_buffer_ms")].toInteger();
    port.cachedResultPath = cbor[QLatin1String("cached_result")].toString();
    port.cachedResultHash = cbor[QLatin1String("cached_result_hash")].toString();
    return port;
}
