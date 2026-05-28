//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_port.cpp
 * @since 2026
 * @date  April 2026
 * @brief JSON and CBOR codecs for @ref MnaPort including data-kind enum mapping, optional stream binding fields and cached-result references.
 *
 * The implementation converts @ref MnaDataKind and
 * @ref MnaPortDir through the canonical string helpers from
 * @ref mna_types.h so the on-disk vocabulary stays single-source.
 * Stream-related fields (@c streamProtocol, @c streamEndpoint,
 * @c streamBufferMs) and cache-related fields
 * (@c cachedResultPath, @c cachedResultHash) are only written when
 * non-default so simple file-based projects stay compact, and any
 * unknown attribute is shovelled into @c extras to keep the round
 * trip lossless across MNALIB versions.
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
    QJsonObject json = extras;
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

    static const QSet<QString> knownKeys = {
        QStringLiteral("name"), QStringLiteral("data_kind"),
        QStringLiteral("direction"), QStringLiteral("source_node"),
        QStringLiteral("source_port"), QStringLiteral("stream_protocol"),
        QStringLiteral("stream_endpoint"), QStringLiteral("stream_buffer_ms"),
        QStringLiteral("cached_result"), QStringLiteral("cached_result_hash")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            port.extras.insert(it.key(), it.value());
    }

    return port;
}

//=============================================================================================================

QCborMap MnaPort::toCbor() const
{
    QCborMap cbor = QCborMap::fromJsonObject(extras);
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

    static const QSet<QString> knownKeys = {
        QStringLiteral("name"), QStringLiteral("data_kind"),
        QStringLiteral("direction"), QStringLiteral("source_node"),
        QStringLiteral("source_port"), QStringLiteral("stream_protocol"),
        QStringLiteral("stream_endpoint"), QStringLiteral("stream_buffer_ms"),
        QStringLiteral("cached_result"), QStringLiteral("cached_result_hash")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            port.extras.insert(it.key(), it.value());
    }

    return port;
}
