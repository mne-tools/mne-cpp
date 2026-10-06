//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_file_sharer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref FiffFileSharer: refcounted memory-mapped view of a FIFF file shared across consumers.
 *
 * Backs zero-copy access for the realtime pipeline, the GUI viewer and
 * the recording dumper without duplicating I/O.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_file_sharer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE STATIC MEMBERS
//=============================================================================================================

const static char m_sDefaultDirectory[]("realtime_shared_files");
const static char m_sDefaultFileName[]("realtime_file");

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffFileSharer::FiffFileSharer()
: FiffFileSharer(m_sDefaultDirectory)
{
}

//=============================================================================================================

FiffFileSharer::FiffFileSharer(const QString& sDirName)
: m_sDirectory(QDir::currentPath() + "/" + sDirName)
, m_iFileIndex(0)
{
}

//=============================================================================================================

void FiffFileSharer::copyRealtimeFile(const QString& sSourcePath)
{
    if (!initSharedDirectory())
        return;

    // Complete the copy under a temporary name and rename it, so the watcher never sees a half-written file.
    const QString sFilePath(m_sDirectory + "/" + m_sDefaultFileName + QString::number(m_iFileIndex++) + "_raw.fif");
    const QString sPartPath(sFilePath + ".part");
    QFile::remove(sPartPath);
    if (!QFile::copy(sSourcePath, sPartPath))
        return;

    QFile newFile(sPartPath);
    if (newFile.open(QIODevice::ReadWrite)) {
        FIFFLIB::FiffStream stream(&newFile);
        stream.skipRawData(newFile.bytesAvailable());
        stream.finish_writing_raw();
        newFile.close();
    }
    QFile::remove(sFilePath);
    QFile::rename(sPartPath, sFilePath);
}

//=============================================================================================================

void FiffFileSharer::initWatcher()
{
    if (initSharedDirectory()) {
        clearSharedDirectory();
        m_fileWatcher.addPath(m_sDirectory);
        connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged,
                this, &FiffFileSharer::onDirectoryChanged, Qt::UniqueConnection);
    } else {
        qWarning() << "[FiffFileSharer::initWatcher] Unable to initilaize shared directory";
    }
}

//=============================================================================================================

void FiffFileSharer::clearSharedDirectory()
{
    QDir directory(m_sDirectory);
    directory.setNameFilters(QStringList("*.*"));
    directory.setFilter(QDir::Files);
    for (auto& file : directory.entryList()) {
        directory.remove(file);
    }
}

//=============================================================================================================

void FiffFileSharer::onDirectoryChanged(const QString& sPath)
{
    // Files appear complete (renamed into place), so every new one can be reported as soon as it is listed.
    QString filePath(sPath + "/" + m_sDefaultFileName + QString::number(m_iFileIndex) + "_raw.fif");
    while (QFile::exists(filePath)) {
        emit newFileAtPath(filePath);
        filePath = sPath + "/" + m_sDefaultFileName + QString::number(++m_iFileIndex) + "_raw.fif";
    }
}

//=============================================================================================================

bool FiffFileSharer::initSharedDirectory()
{
    QDir sharedDirectory(m_sDirectory);
    if (!sharedDirectory.exists()) {
        sharedDirectory.mkpath(".");
    }

    return sharedDirectory.exists();
}
