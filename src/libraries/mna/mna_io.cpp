//=============================================================================================================
/**
 * @file     mna_io.cpp
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
 * @brief    MnaIO class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_io.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCborValue>
#include <QCborMap>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNALIB;

//=============================================================================================================
// STATIC CONSTANTS
//=============================================================================================================

static const QByteArray MNX_MAGIC = QByteArrayLiteral("MNX1");

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

MnaProject MnaIO::read(const QString& path)
{
    const QFileInfo fi(path);
    const QString suffix = fi.suffix().toLower();

    if(suffix == QLatin1String("mna")) {
        return readJson(path);
    } else if(suffix == QLatin1String("mnx")) {
        return readCbor(path);
    }

    qWarning() << "[MnaIO::read] Unknown extension:" << suffix;
    return MnaProject();
}

//=============================================================================================================

bool MnaIO::write(const MnaProject& project, const QString& path)
{
    const QFileInfo fi(path);
    const QString suffix = fi.suffix().toLower();

    if(suffix == QLatin1String("mna")) {
        return writeJson(project, path);
    } else if(suffix == QLatin1String("mnx")) {
        return writeCbor(project, path);
    }

    qWarning() << "[MnaIO::write] Unknown extension:" << suffix;
    return false;
}

//=============================================================================================================

MnaProject MnaIO::readJson(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[MnaIO::readJson] Cannot open file:" << path;
        return MnaProject();
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if(parseError.error != QJsonParseError::NoError) {
        qWarning() << "[MnaIO::readJson] Parse error:" << parseError.errorString();
        return MnaProject();
    }

    const QJsonObject root = doc.object();

    // Version check: reject if major version differs
    const QString fileVersion = root[QLatin1String("mna_version")].toString();
    const int fileMajor = fileVersion.section(QLatin1Char('.'), 0, 0).toInt();
    const int currentMajor = QString::fromLatin1(MnaProject::CURRENT_SCHEMA_VERSION).section(QLatin1Char('.'), 0, 0).toInt();
    if(fileMajor != currentMajor) {
        qWarning() << "[MnaIO::readJson] Incompatible major version:" << fileVersion
                    << "(expected major" << currentMajor << ")";
        return MnaProject();
    }

    return MnaProject::fromJson(root);
}

//=============================================================================================================

bool MnaIO::writeJson(const MnaProject& project, const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[MnaIO::writeJson] Cannot open file for writing:" << path;
        return false;
    }

    const QJsonDocument doc(project.toJson());
    const QByteArray data = doc.toJson(QJsonDocument::Indented);
    const qint64 written = file.write(data);
    file.close();

    return (written == data.size());
}

//=============================================================================================================

MnaProject MnaIO::readCbor(const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[MnaIO::readCbor] Cannot open file:" << path;
        return MnaProject();
    }

    // Verify magic header
    const QByteArray magic = file.read(MNX_MAGIC.size());
    if(magic != MNX_MAGIC) {
        qWarning() << "[MnaIO::readCbor] Invalid magic header in:" << path;
        file.close();
        return MnaProject();
    }

    const QByteArray cborData = file.readAll();
    file.close();

    const QCborValue cborVal = QCborValue::fromCbor(cborData);
    if(!cborVal.isMap()) {
        qWarning() << "[MnaIO::readCbor] Root CBOR value is not a map";
        return MnaProject();
    }

    const QCborMap root = cborVal.toMap();

    // Version check: reject if major version differs
    const QString fileVersion = root[QLatin1String("mna_version")].toString();
    const int fileMajor = fileVersion.section(QLatin1Char('.'), 0, 0).toInt();
    const int currentMajor = QString::fromLatin1(MnaProject::CURRENT_SCHEMA_VERSION).section(QLatin1Char('.'), 0, 0).toInt();
    if(fileMajor != currentMajor) {
        qWarning() << "[MnaIO::readCbor] Incompatible major version:" << fileVersion
                    << "(expected major" << currentMajor << ")";
        return MnaProject();
    }

    return MnaProject::fromCbor(root);
}

//=============================================================================================================

bool MnaIO::writeCbor(const MnaProject& project, const QString& path)
{
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "[MnaIO::writeCbor] Cannot open file for writing:" << path;
        return false;
    }

    // Write magic header
    if(file.write(MNX_MAGIC) != MNX_MAGIC.size()) {
        qWarning() << "[MnaIO::writeCbor] Failed to write magic header";
        file.close();
        return false;
    }

    const QCborValue cborVal(project.toCbor());
    const QByteArray cborData = cborVal.toCbor();
    const qint64 written = file.write(cborData);
    file.close();

    return (written == cborData.size());
}
