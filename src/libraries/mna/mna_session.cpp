//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_session.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaSession — ordered @ref MnaRecording list plus opaque id and forward-compatible @c extras.
 *
 * Implementation mirrors the rest of the project tree: each
 * recording is serialised through @ref MnaRecording::toJson /
 * @c toCbor, ordering is preserved to keep run numbering stable,
 * and any unknown sidecar metadata supplied by newer tooling is
 * captured into @c extras on read and re-emitted on write so the
 * round-trip stays lossless across MNALIB versions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_session.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QJsonObject MnaSession::toJson() const
{
    QJsonObject json = extras;
    json[QLatin1String("id")] = id;

    QJsonArray recsArr;
    for(const MnaRecording& r : recordings) {
        recsArr.append(r.toJson());
    }
    json[QLatin1String("recordings")] = recsArr;

    return json;
}

//=============================================================================================================

MnaSession MnaSession::fromJson(const QJsonObject& json)
{
    MnaSession session;
    session.id = json[QLatin1String("id")].toString();

    const QJsonArray recsArr = json[QLatin1String("recordings")].toArray();
    for(const QJsonValue& v : recsArr) {
        session.recordings.append(MnaRecording::fromJson(v.toObject()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("recordings")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            session.extras.insert(it.key(), it.value());
    }

    return session;
}

//=============================================================================================================

QCborMap MnaSession::toCbor() const
{
    QCborMap cbor = QCborMap::fromJsonObject(extras);
    cbor[QLatin1String("id")] = id;

    QCborArray recsArr;
    for(const MnaRecording& r : recordings) {
        recsArr.append(r.toCbor());
    }
    cbor[QLatin1String("recordings")] = recsArr;

    return cbor;
}

//=============================================================================================================

MnaSession MnaSession::fromCbor(const QCborMap& cbor)
{
    MnaSession session;
    session.id = cbor[QLatin1String("id")].toString();

    const QCborArray recsArr = cbor[QLatin1String("recordings")].toArray();
    for(const QCborValue& v : recsArr) {
        session.recordings.append(MnaRecording::fromCbor(v.toMap()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("recordings")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            session.extras.insert(it.key(), it.value());
    }

    return session;
}
