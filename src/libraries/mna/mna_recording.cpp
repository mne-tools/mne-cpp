//=============================================================================================================
/**
 * @file     mna_recording.cpp
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
 * @brief    MnaRecording class definition.
 *
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
