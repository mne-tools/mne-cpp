//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_coordinate_system.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref BIDSLIB::BidsCoordinateSystem — JSON I/O and @c FiffCoordTrans bridge for ``_coordsystem.json``.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_coordinate_system.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;

//=============================================================================================================
// STATIC METHODS
//=============================================================================================================

BidsCoordinateSystem BidsCoordinateSystem::readJson(const QString& sFilePath)
{
    BidsCoordinateSystem cs;
    cs.transform = Eigen::Matrix4d::Identity();

    QFile file(sFilePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BidsCoordinateSystem::readJson] Cannot open" << sFilePath;
        return cs;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if(error.error != QJsonParseError::NoError) {
        qWarning() << "[BidsCoordinateSystem::readJson] Parse error in" << sFilePath
                    << ":" << error.errorString();
        return cs;
    }

    QJsonObject json = doc.object();

    // Try iEEG fields first, fall back to EEG fields
    cs.system = json.value(QStringLiteral("iEEGCoordinateSystem")).toString();
    if(cs.system.isEmpty())
        cs.system = json.value(QStringLiteral("EEGCoordinateSystem")).toString();

    cs.units = json.value(QStringLiteral("iEEGCoordinateUnits")).toString();
    if(cs.units.isEmpty())
        cs.units = json.value(QStringLiteral("EEGCoordinateUnits")).toString();

    cs.description = json.value(QStringLiteral("iEEGCoordinateSystemDescription")).toString();
    if(cs.description.isEmpty())
        cs.description = json.value(QStringLiteral("EEGCoordinateSystemDescription")).toString();

    cs.processingDescription = json.value(QStringLiteral("iEEGCoordinateProcessingDescription")).toString();
    if(cs.processingDescription.isEmpty())
        cs.processingDescription = json.value(QStringLiteral("EEGCoordinateProcessingDescription")).toString();

    cs.associatedImagePath = json.value(QStringLiteral("IntendedFor")).toString();

    // Parse 4x4 transform if provided as "iEEGCoordinateProcessingTransform" or "Transform"
    QString transformKey;
    if(json.contains(QStringLiteral("iEEGCoordinateProcessingTransform")))
        transformKey = QStringLiteral("iEEGCoordinateProcessingTransform");
    else if(json.contains(QStringLiteral("Transform")))
        transformKey = QStringLiteral("Transform");

    if(!transformKey.isEmpty()) {
        QJsonArray rows = json.value(transformKey).toArray();
        if(rows.size() == 4) {
            for(int r = 0; r < 4; ++r) {
                QJsonArray cols = rows[r].toArray();
                if(cols.size() == 4) {
                    for(int c = 0; c < 4; ++c)
                        cs.transform(r, c) = cols[c].toDouble();
                }
            }
        }
    }

    return cs;
}

//=============================================================================================================

bool BidsCoordinateSystem::writeJson(const QString& sFilePath,
                                     const BidsCoordinateSystem& cs)
{
    QJsonObject json;

    json[QStringLiteral("iEEGCoordinateSystem")] = cs.system;
    json[QStringLiteral("iEEGCoordinateUnits")]  = cs.units;

    if(!cs.description.isEmpty())
        json[QStringLiteral("iEEGCoordinateSystemDescription")] = cs.description;
    if(!cs.processingDescription.isEmpty())
        json[QStringLiteral("iEEGCoordinateProcessingDescription")] = cs.processingDescription;
    if(!cs.associatedImagePath.isEmpty())
        json[QStringLiteral("IntendedFor")] = cs.associatedImagePath;

    // Serialize the 4x4 transform matrix
    if(!cs.transform.isIdentity(1e-15)) {
        QJsonArray rows;
        for(int r = 0; r < 4; ++r) {
            QJsonArray cols;
            for(int c = 0; c < 4; ++c)
                cols.append(cs.transform(r, c));
            rows.append(cols);
        }
        json[QStringLiteral("iEEGCoordinateProcessingTransform")] = rows;
    }

    QFile file(sFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BidsCoordinateSystem::writeJson] Cannot open" << sFilePath << "for writing";
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

//=============================================================================================================

FIFFLIB::FiffCoordTrans BidsCoordinateSystem::toFiffCoordTrans(int fromFrame, int toFrame) const
{
    Eigen::Matrix4f matTrans = transform.cast<float>();
    return FIFFLIB::FiffCoordTrans(fromFrame, toFrame, matTrans);
}
