//=============================================================================================================
/**
 * @file     bids_coordinate_system.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
    cs.system                = json.value(QStringLiteral("iEEGCoordinateSystem")).toString();
    cs.units                 = json.value(QStringLiteral("iEEGCoordinateUnits")).toString();
    cs.description           = json.value(QStringLiteral("iEEGCoordinateSystemDescription")).toString();
    cs.processingDescription = json.value(QStringLiteral("iEEGCoordinateProcessingDescription")).toString();
    cs.associatedImagePath   = json.value(QStringLiteral("IntendedFor")).toString();

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

    QFile file(sFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[BidsCoordinateSystem::writeJson] Cannot open" << sFilePath << "for writing";
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
