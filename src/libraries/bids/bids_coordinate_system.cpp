//=============================================================================================================
/**
 * @file     bids_coordinate_system.cpp
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
 * @brief    BidsCoordinateSystem struct definition — *_coordsystem.json I/O.
 *
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
