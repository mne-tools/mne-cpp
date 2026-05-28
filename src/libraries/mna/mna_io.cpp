//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_io.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of the @ref MnaIO codecs — UTF-8 JSON for @c .mna files and CBOR (prefixed with the @c "MNX1" four-byte magic) for @c .mnx files.
 *
 * The @ref read / @ref write entry points dispatch on the file
 * extension and delegate to the four private helpers. The CBOR
 * writer emits the @c MNX1 magic up-front so the reader can
 * fast-fail on a mismatched binary before attempting to decode
 * the payload; the JSON writer pretty-prints with indentation so
 * @c .mna files remain diff- and human-friendly under version
 * control. All four helpers share a strict error-handling policy:
 * file or parse errors surface as empty projects with a warning
 * on @c qWarning so callers cannot silently confuse load failures
 * with empty input.
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
