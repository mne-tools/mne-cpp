//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_project.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaProject plus @ref read / @ref write thin façades that delegate to @ref MnaIO.
 *
 * The implementation serialises the project metadata
 * (name, description, @c mnaVersion, creation/modification
 * timestamps) followed by the @ref MnaSubject list and the
 * @ref MnaNode pipeline. The @c mnaVersion field is written from
 * @ref MnaProject::CURRENT_SCHEMA_VERSION so older readers can
 * refuse projects from a future schema, while @c extras carries
 * forward-compatible additions through the round-trip without
 * loss.
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
    // Start from extras to preserve unknown keys
    QJsonObject json = extras;
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

    // Capture unknown keys for lossless round-trip
    static const QSet<QString> knownKeys = {
        QStringLiteral("mna_version"), QStringLiteral("name"),
        QStringLiteral("description"), QStringLiteral("created"),
        QStringLiteral("modified"), QStringLiteral("subjects"),
        QStringLiteral("pipeline")
    };
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            proj.extras.insert(it.key(), it.value());
    }

    return proj;
}

//=============================================================================================================

QCborMap MnaProject::toCbor() const
{
    // Start from extras to preserve unknown keys
    QCborMap cbor = QCborMap::fromJsonObject(extras);
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

    // Capture unknown keys for lossless round-trip
    static const QSet<QString> knownKeys = {
        QStringLiteral("mna_version"), QStringLiteral("name"),
        QStringLiteral("description"), QStringLiteral("created"),
        QStringLiteral("modified"), QStringLiteral("subjects"),
        QStringLiteral("pipeline")
    };
    QJsonObject cborJson = cbor.toJsonObject();
    for (auto it = cborJson.constBegin(); it != cborJson.constEnd(); ++it) {
        if (!knownKeys.contains(it.key()))
            proj.extras.insert(it.key(), it.value());
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
