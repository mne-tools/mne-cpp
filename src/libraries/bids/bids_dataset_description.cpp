//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_dataset_description.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BidsDatasetDescription — JSON I/O for the root ``dataset_description.json`` sidecar.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_dataset_description.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

BidsDatasetDescription BidsDatasetDescription::read(const QString& sFilePath)
{
    BidsDatasetDescription desc;

    QFile file(sFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BidsDatasetDescription::read] Cannot open" << sFilePath;
        return desc;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if(error.error != QJsonParseError::NoError) {
        qWarning() << "[BidsDatasetDescription::read] Parse error in" << sFilePath
                    << ":" << error.errorString();
        return desc;
    }

    QJsonObject json = doc.object();
    desc.name        = json.value(QStringLiteral("Name")).toString();
    desc.bidsVersion = json.value(QStringLiteral("BIDSVersion")).toString();
    desc.datasetType = json.value(QStringLiteral("DatasetType")).toString();
    desc.license     = json.value(QStringLiteral("License")).toString();

    return desc;
}

//=============================================================================================================

bool BidsDatasetDescription::write(const QString& sFilePath,
                                    const BidsDatasetDescription& desc)
{
    QJsonObject json;

    json[QStringLiteral("Name")] = desc.name;
    json[QStringLiteral("BIDSVersion")] = desc.bidsVersion;

    if(!desc.datasetType.isEmpty())
        json[QStringLiteral("DatasetType")] = desc.datasetType;
    if(!desc.license.isEmpty())
        json[QStringLiteral("License")] = desc.license;

    QFile file(sFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BidsDatasetDescription::write] Cannot open" << sFilePath << "for writing";
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
