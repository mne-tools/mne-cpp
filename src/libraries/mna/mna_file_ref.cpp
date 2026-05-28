//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_file_ref.cpp
 * @since April 2026
 * @brief JSON and CBOR codecs for @ref MnaFileRef, including base64 framing of embedded payloads and preservation of unknown keys in @c extras.
 *
 * The implementation is symmetric: every field declared in the
 * struct round-trips losslessly through both the JSON and CBOR
 * paths, embedded bytes are encoded as base64 in JSON (for
 * human-editability) and as a native byte string in CBOR (for
 * compactness), and any key not recognised by the current build
 * is shovelled into @c extras on read and re-emitted verbatim on
 * write to keep forward compatibility intact.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_file_ref.h"

#include <QJsonValue>
#include <QCborValue>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QJsonObject MnaFileRef::toJson() const
{
    QJsonObject json = extras;
    json[QLatin1String("role")]       = mnaFileRoleToString(role);
    json[QLatin1String("path")]       = path;
    json[QLatin1String("sha256")]     = sha256;
    json[QLatin1String("format")]     = format;
    json[QLatin1String("size_bytes")] = sizeBytes;
    json[QLatin1String("embedded")]   = embedded;
    if(embedded && !data.isEmpty()) {
        json[QLatin1String("data")] = QString::fromLatin1(data.toBase64());
    }
    return json;
}

//=============================================================================================================

MnaFileRef MnaFileRef::fromJson(const QJsonObject& json)
{
    MnaFileRef ref;
    ref.role      = mnaFileRoleFromString(json[QLatin1String("role")].toString());
    ref.path      = json[QLatin1String("path")].toString();
    ref.sha256    = json[QLatin1String("sha256")].toString();
    ref.format    = json[QLatin1String("format")].toString();
    ref.sizeBytes = static_cast<qint64>(json[QLatin1String("size_bytes")].toDouble());
    ref.embedded  = json[QLatin1String("embedded")].toBool();
    if(ref.embedded) {
        ref.data = QByteArray::fromBase64(json[QLatin1String("data")].toString().toLatin1());
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("role"), QStringLiteral("path"),
        QStringLiteral("sha256"), QStringLiteral("format"),
        QStringLiteral("size_bytes"), QStringLiteral("embedded"),
        QStringLiteral("data")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            ref.extras.insert(it.key(), it.value());
    }

    return ref;
}

//=============================================================================================================

QCborMap MnaFileRef::toCbor() const
{
    QCborMap cbor = QCborMap::fromJsonObject(extras);
    cbor[QLatin1String("role")]       = mnaFileRoleToString(role);
    cbor[QLatin1String("path")]       = path;
    cbor[QLatin1String("sha256")]     = sha256;
    cbor[QLatin1String("format")]     = format;
    cbor[QLatin1String("size_bytes")] = sizeBytes;
    cbor[QLatin1String("embedded")]   = embedded;
    if(embedded && !data.isEmpty()) {
        cbor[QLatin1String("data")] = QCborValue(data);
    }
    return cbor;
}

//=============================================================================================================

MnaFileRef MnaFileRef::fromCbor(const QCborMap& cbor)
{
    MnaFileRef ref;
    ref.role      = mnaFileRoleFromString(cbor[QLatin1String("role")].toString());
    ref.path      = cbor[QLatin1String("path")].toString();
    ref.sha256    = cbor[QLatin1String("sha256")].toString();
    ref.format    = cbor[QLatin1String("format")].toString();
    ref.sizeBytes = cbor[QLatin1String("size_bytes")].toInteger();
    ref.embedded  = cbor[QLatin1String("embedded")].toBool();
    if(ref.embedded) {
        ref.data = cbor[QLatin1String("data")].toByteArray();
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("role"), QStringLiteral("path"),
        QStringLiteral("sha256"), QStringLiteral("format"),
        QStringLiteral("size_bytes"), QStringLiteral("embedded"),
        QStringLiteral("data")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            ref.extras.insert(it.key(), it.value());
    }

    return ref;
}
