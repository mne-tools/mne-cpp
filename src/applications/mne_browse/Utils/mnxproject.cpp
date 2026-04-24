//=============================================================================================================
/**
 * @file     mnxproject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
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
 * @brief    MnxProject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnxproject.h"

#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDirIterator>
#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

static constexpr int kManifestVersion = 1;
static const QString kMegSubdir = QStringLiteral("meg");
static const QString kManifestName = QStringLiteral("manifest.json");

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnxProject::MnxProject() = default;

MnxProject::~MnxProject()
{
    close();
}

MnxProject::MnxProject(MnxProject&&) noexcept = default;
MnxProject& MnxProject::operator=(MnxProject&&) noexcept = default;

//=============================================================================================================

bool MnxProject::open(const QString& mnxPath)
{
    close();

    QZipReader zip(mnxPath);
    if (!zip.isReadable()) {
        qWarning() << "MnxProject: Cannot read" << mnxPath;
        return false;
    }

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (!m_tempDir->isValid()) {
        qWarning() << "MnxProject: Failed to create temp directory";
        m_tempDir.reset();
        return false;
    }

    const QString destDir = m_tempDir->path();
    if (!zip.extractAll(destDir)) {
        qWarning() << "MnxProject: Extraction failed for" << mnxPath;
        m_tempDir.reset();
        return false;
    }

    m_mnxFilePath = mnxPath;

    // Parse manifest if present
    const QString manifestPath = destDir + QDir::separator() + kManifestName;
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            m_settings = doc.object();
        } else {
            qWarning() << "MnxProject: manifest.json parse error:" << err.errorString();
        }
    }

    discoverFiles();
    qInfo() << "MnxProject: Opened" << mnxPath << "→" << destDir;
    return true;
}

//=============================================================================================================

bool MnxProject::create(const QString& rawPath,
                          const QString& eventPath,
                          const QString& annotationPath,
                          const QString& virtualChanPath,
                          const QString& evokedPath,
                          const QString& covPath,
                          const QString& inversePath)
{
    close();

    if (rawPath.isEmpty() || !QFileInfo::exists(rawPath)) {
        qWarning() << "MnxProject::create: raw file does not exist:" << rawPath;
        return false;
    }

    m_tempDir = std::make_unique<QTemporaryDir>();
    if (!m_tempDir->isValid()) {
        qWarning() << "MnxProject: Failed to create temp directory";
        m_tempDir.reset();
        return false;
    }

    // Create BIDS-like meg/ subdirectory
    QDir(m_tempDir->path()).mkpath(kMegSubdir);

    // Derive a BIDS-like base name from the raw file name
    // e.g. "sample_audvis_raw.fif" → "sample_audvis_raw"
    QString rawBaseName = QFileInfo(rawPath).completeBaseName();

    // Copy raw file
    if (!copyIntoProject(rawPath, kMegSubdir + QDir::separator() + rawBaseName + QStringLiteral(".fif"))) {
        m_tempDir.reset();
        return false;
    }

    // Copy optional sidecars
    auto copyOptional = [&](const QString& src, const QString& suffix) {
        if (!src.isEmpty() && QFileInfo::exists(src)) {
            copyIntoProject(src, kMegSubdir + QDir::separator() + rawBaseName + suffix);
        }
    };

    copyOptional(eventPath,       QStringLiteral("-eve.fif"));
    copyOptional(annotationPath,  QStringLiteral("-annot.json"));
    copyOptional(virtualChanPath, QStringLiteral("-virtchan.json"));
    copyOptional(evokedPath,      QStringLiteral("-ave.fif"));
    copyOptional(covPath,         QStringLiteral("-cov.fif"));
    copyOptional(inversePath,     QStringLiteral("-inv.fif"));

    discoverFiles();
    qInfo() << "MnxProject: Created from" << rawPath << "→" << m_tempDir->path();
    return true;
}

//=============================================================================================================

bool MnxProject::save(const QString& mnxPath, const QJsonObject& settings)
{
    if (!isOpen()) {
        qWarning() << "MnxProject::save: No project is open";
        return false;
    }

    // Write manifest.json into the temp directory
    QJsonObject manifest = settings;
    manifest[QStringLiteral("version")] = kManifestVersion;
    manifest[QStringLiteral("format")] = QStringLiteral("mnx");

    const QString manifestPath = m_tempDir->path() + QDir::separator() + kManifestName;
    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        manifestFile.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
        manifestFile.close();
    } else {
        qWarning() << "MnxProject: Failed to write manifest";
        return false;
    }

    // Pack everything into a ZIP
    QZipWriter zip(mnxPath);
    if (!zip.isWritable()) {
        qWarning() << "MnxProject: Cannot write to" << mnxPath;
        return false;
    }

    const QString basePath = m_tempDir->path();
    QDirIterator it(basePath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QString absPath = it.filePath();
        const QString relPath = QDir(basePath).relativeFilePath(absPath);

        QFile f(absPath);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "MnxProject: Cannot read" << absPath << "for packing";
            continue;
        }

        // Store large FIF files uncompressed (they're already dense binary)
        const bool isFif = relPath.endsWith(QStringLiteral(".fif"), Qt::CaseInsensitive);
        zip.setCompressionPolicy(isFif ? QZipWriter::NeverCompress : QZipWriter::AutoCompress);
        zip.addFile(relPath, f.readAll());
    }

    zip.close();
    m_mnxFilePath = mnxPath;
    m_settings = manifest;

    qInfo() << "MnxProject: Saved to" << mnxPath;
    return true;
}

//=============================================================================================================

bool MnxProject::isOpen() const
{
    return m_tempDir && m_tempDir->isValid();
}

//=============================================================================================================

void MnxProject::close()
{
    m_tempDir.reset();     // QTemporaryDir destructor removes the directory
    m_mnxFilePath.clear();
    m_settings = {};
    m_rawPath.clear();
    m_eventPath.clear();
    m_annotationPath.clear();
    m_virtualChanPath.clear();
    m_evokedPath.clear();
    m_covPath.clear();
    m_inversePath.clear();
}

//=============================================================================================================

QString MnxProject::rawPath() const         { return m_rawPath; }
QString MnxProject::eventPath() const       { return m_eventPath; }
QString MnxProject::annotationPath() const  { return m_annotationPath; }
QString MnxProject::virtualChannelPath() const { return m_virtualChanPath; }
QString MnxProject::evokedPath() const      { return m_evokedPath; }
QString MnxProject::covariancePath() const  { return m_covPath; }
QString MnxProject::inversePath() const     { return m_inversePath; }

//=============================================================================================================

QString MnxProject::tempDir() const
{
    return m_tempDir ? m_tempDir->path() : QString();
}

//=============================================================================================================

void MnxProject::discoverFiles()
{
    if (!isOpen()) return;

    const QString base = m_tempDir->path();

    // Scan for files matching known patterns
    QDirIterator it(base, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QString path = it.filePath();
        const QString name = it.fileName();

        // Raw FIF: any .fif that is NOT an event/evoked/cov/inv file
        if (name.endsWith(QStringLiteral(".fif"), Qt::CaseInsensitive)) {
            if (name.endsWith(QStringLiteral("-eve.fif"), Qt::CaseInsensitive)) {
                m_eventPath = path;
            } else if (name.endsWith(QStringLiteral("-ave.fif"), Qt::CaseInsensitive)) {
                m_evokedPath = path;
            } else if (name.endsWith(QStringLiteral("-cov.fif"), Qt::CaseInsensitive)) {
                m_covPath = path;
            } else if (name.endsWith(QStringLiteral("-inv.fif"), Qt::CaseInsensitive)) {
                m_inversePath = path;
            } else if (name.endsWith(QStringLiteral("_annot.fif"), Qt::CaseInsensitive)
                       || name.endsWith(QStringLiteral("-annot.fif"), Qt::CaseInsensitive)) {
                m_annotationPath = path;
            } else if (m_rawPath.isEmpty()) {
                // First unclassified .fif is the raw file
                m_rawPath = path;
            }
        } else if (name.endsWith(QStringLiteral("-annot.json"), Qt::CaseInsensitive)
                   || name.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            if (name.contains(QStringLiteral("annot"), Qt::CaseInsensitive)
                && !name.contains(QStringLiteral("virtchan"), Qt::CaseInsensitive)
                && !name.contains(QStringLiteral("manifest"), Qt::CaseInsensitive)) {
                m_annotationPath = path;
            } else if (name.contains(QStringLiteral("virtchan"), Qt::CaseInsensitive)) {
                m_virtualChanPath = path;
            }
        }
    }
}

//=============================================================================================================

bool MnxProject::copyIntoProject(const QString& srcPath, const QString& targetName)
{
    const QString destPath = m_tempDir->path() + QDir::separator() + targetName;

    // Ensure parent directory exists
    QDir().mkpath(QFileInfo(destPath).absolutePath());

    if (!QFile::copy(srcPath, destPath)) {
        qWarning() << "MnxProject: Failed to copy" << srcPath << "→" << destPath;
        return false;
    }
    return true;
}
