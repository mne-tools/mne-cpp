//=============================================================================================================
/**
 * @file     mna_project.cpp
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
 * @brief    MnaProject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_project.h"
#include "mna_io.h"

#include <QJsonArray>
#include <QCborArray>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

MnaProject::MnaProject()
    : mnaVersion(CURRENT_SCHEMA_VERSION)
    , created(QDateTime::currentDateTimeUtc())
    , modified(QDateTime::currentDateTimeUtc())
{
}

//=============================================================================================================

QJsonObject MnaProject::toJson() const
{
    QJsonObject json;
    json[QLatin1String("mna_version")] = mnaVersion;
    json[QLatin1String("name")]        = name;
    json[QLatin1String("description")] = description;
    json[QLatin1String("created")]     = created.toString(Qt::ISODate);
    json[QLatin1String("modified")]    = modified.toString(Qt::ISODate);

    QJsonArray subjArr;
    for(const MnaSubject& s : subjects) {
        subjArr.append(s.toJson());
    }
    json[QLatin1String("subjects")] = subjArr;

    QJsonArray pipeArr;
    for(const MnaNode& nd : pipeline) {
        pipeArr.append(nd.toJson());
    }
    json[QLatin1String("pipeline")] = pipeArr;

    return json;
}

//=============================================================================================================

MnaProject MnaProject::fromJson(const QJsonObject& json)
{
    MnaProject proj;
    proj.mnaVersion  = json[QLatin1String("mna_version")].toString();
    proj.name        = json[QLatin1String("name")].toString();
    proj.description = json[QLatin1String("description")].toString();
    proj.created     = QDateTime::fromString(json[QLatin1String("created")].toString(), Qt::ISODate);
    proj.modified    = QDateTime::fromString(json[QLatin1String("modified")].toString(), Qt::ISODate);

    const QJsonArray subjArr = json[QLatin1String("subjects")].toArray();
    for(const QJsonValue& v : subjArr) {
        proj.subjects.append(MnaSubject::fromJson(v.toObject()));
    }

    const QJsonArray pipeArr = json[QLatin1String("pipeline")].toArray();
    for(const QJsonValue& v : pipeArr) {
        proj.pipeline.append(MnaNode::fromJson(v.toObject()));
    }

    return proj;
}

//=============================================================================================================

QCborMap MnaProject::toCbor() const
{
    QCborMap cbor;
    cbor[QLatin1String("mna_version")] = mnaVersion;
    cbor[QLatin1String("name")]        = name;
    cbor[QLatin1String("description")] = description;
    cbor[QLatin1String("created")]     = QCborValue(created);
    cbor[QLatin1String("modified")]    = QCborValue(modified);

    QCborArray subjArr;
    for(const MnaSubject& s : subjects) {
        subjArr.append(s.toCbor());
    }
    cbor[QLatin1String("subjects")] = subjArr;

    QCborArray pipeArr;
    for(const MnaNode& nd : pipeline) {
        pipeArr.append(nd.toCbor());
    }
    cbor[QLatin1String("pipeline")] = pipeArr;

    return cbor;
}

//=============================================================================================================

MnaProject MnaProject::fromCbor(const QCborMap& cbor)
{
    MnaProject proj;
    proj.mnaVersion  = cbor[QLatin1String("mna_version")].toString();
    proj.name        = cbor[QLatin1String("name")].toString();
    proj.description = cbor[QLatin1String("description")].toString();
    proj.created     = cbor[QLatin1String("created")].toDateTime();
    proj.modified    = cbor[QLatin1String("modified")].toDateTime();

    const QCborArray subjArr = cbor[QLatin1String("subjects")].toArray();
    for(const QCborValue& v : subjArr) {
        proj.subjects.append(MnaSubject::fromCbor(v.toMap()));
    }

    const QCborArray pipeArr = cbor[QLatin1String("pipeline")].toArray();
    for(const QCborValue& v : pipeArr) {
        proj.pipeline.append(MnaNode::fromCbor(v.toMap()));
    }

    return proj;
}

//=============================================================================================================

MnaProject MnaProject::read(const QString& path)
{
    return MnaIO::read(path);
}

//=============================================================================================================

bool MnaProject::write(const MnaProject& project, const QString& path)
{
    return MnaIO::write(project, path);
}
