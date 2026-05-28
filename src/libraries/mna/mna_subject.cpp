//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_subject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaSubject — subject id, FreeSurfer @c $SUBJECTS_DIR link, ordered session list and forward-compatible @c extras.
 *
 * The implementation walks the @ref MnaSession list with the
 * matching session codec and emits @c freeSurferDir as a relative
 * POSIX path so projects remain portable between hosts. Any
 * subject-level fields not recognised by this build — demographics,
 * consent flags, clinical metadata — are routed through @c extras
 * to guarantee round-trip fidelity with newer MNALIB versions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_subject.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QJsonObject MnaSubject::toJson() const
{
    QJsonObject json = extras;
    json[QLatin1String("id")]             = id;
    json[QLatin1String("freesurfer_dir")] = freeSurferDir;

    QJsonArray sessArr;
    for(const MnaSession& s : sessions) {
        sessArr.append(s.toJson());
    }
    json[QLatin1String("sessions")] = sessArr;

    return json;
}

//=============================================================================================================

MnaSubject MnaSubject::fromJson(const QJsonObject& json)
{
    MnaSubject subj;
    subj.id            = json[QLatin1String("id")].toString();
    subj.freeSurferDir = json[QLatin1String("freesurfer_dir")].toString();

    const QJsonArray sessArr = json[QLatin1String("sessions")].toArray();
    for(const QJsonValue& v : sessArr) {
        subj.sessions.append(MnaSession::fromJson(v.toObject()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("freesurfer_dir"),
        QStringLiteral("sessions")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            subj.extras.insert(it.key(), it.value());
    }

    return subj;
}

//=============================================================================================================

QCborMap MnaSubject::toCbor() const
{
    QCborMap cbor = QCborMap::fromJsonObject(extras);
    cbor[QLatin1String("id")]             = id;
    cbor[QLatin1String("freesurfer_dir")] = freeSurferDir;

    QCborArray sessArr;
    for(const MnaSession& s : sessions) {
        sessArr.append(s.toCbor());
    }
    cbor[QLatin1String("sessions")] = sessArr;

    return cbor;
}

//=============================================================================================================

MnaSubject MnaSubject::fromCbor(const QCborMap& cbor)
{
    MnaSubject subj;
    subj.id            = cbor[QLatin1String("id")].toString();
    subj.freeSurferDir = cbor[QLatin1String("freesurfer_dir")].toString();

    const QCborArray sessArr = cbor[QLatin1String("sessions")].toArray();
    for(const QCborValue& v : sessArr) {
        subj.sessions.append(MnaSession::fromCbor(v.toMap()));
    }

    static const QSet<QString> knownKeys = {
        QStringLiteral("id"), QStringLiteral("freesurfer_dir"),
        QStringLiteral("sessions")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            subj.extras.insert(it.key(), it.value());
    }

    return subj;
}
