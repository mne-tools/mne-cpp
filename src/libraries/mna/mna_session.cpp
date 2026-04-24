//=============================================================================================================
/**
 * @file     mna_session.cpp
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
 * @brief    MnaSession class definition.
 *
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
