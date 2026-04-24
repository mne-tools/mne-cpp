//=============================================================================================================
/**
 * @file     mna_subject.cpp
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
 * @brief    MnaSubject class definition.
 *
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
