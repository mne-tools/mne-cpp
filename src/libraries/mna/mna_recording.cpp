//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_recording.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaRecording — ordered @ref MnaFileRef list plus opaque id and forward-compatible @c extras.
 *
 * The implementation defers to @ref MnaFileRef::toJson and the
 * matching @c fromCbor for each file entry, preserving order so
 * the first @c MnaFileRole::Raw element keeps its conventional
 * meaning as the canonical recording artefact. Unknown keys are
 * routed through @c extras on read/write so newer fields added by
 * tooling round-trip cleanly through this build.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_recording.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QJsonObject MnaRecording::toJson() const
{
    QJsonObject json = extras;
    json[QLatin1String("id")] = id;

    QJsonArray filesArr;
    for(const MnaFileRef& f : files) {
        filesArr.append(f.toJson());
    }
    json[QLatin1String("files")] = filesArr;

    return json;
}

//=============================================================================================================

MnaRecording MnaRecording::fromJson(const QJsonObject& json)
{
    MnaRecording rec;
    rec.id = json[QLatin1String("id")].toString();

    const QJsonArray filesArr = json[QLatin1String("files")].toArray();
    for(const QJsonValue& v : filesArr) {
        rec.files.append(MnaFileRef::fromJson(v.toObject()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("files")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            rec.extras.insert(it.key(), it.value());
    }

    return rec;
}

//=============================================================================================================

QCborMap MnaRecording::toCbor() const
{
    QCborMap cbor = QCborMap::fromJsonObject(extras);
    cbor[QLatin1String("id")] = id;

    QCborArray filesArr;
    for(const MnaFileRef& f : files) {
        filesArr.append(f.toCbor());
    }
    cbor[QLatin1String("files")] = filesArr;

    return cbor;
}

//=============================================================================================================

MnaRecording MnaRecording::fromCbor(const QCborMap& cbor)
{
    MnaRecording rec;
    rec.id = cbor[QLatin1String("id")].toString();

    const QCborArray filesArr = cbor[QLatin1String("files")].toArray();
    for(const QCborValue& v : filesArr) {
        rec.files.append(MnaFileRef::fromCbor(v.toMap()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("files")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            rec.extras.insert(it.key(), it.value());
    }

    return rec;
}
