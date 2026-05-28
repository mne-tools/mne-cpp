//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_file_sharer.cpp
 * @since March 2026
 * @brief Implementation of @ref FiffFileSharer: refcounted memory-mapped view of a FIFF file shared across consumers.
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

void FiffFileSharer::copyRealtimeFile(const QString &sSourcePath)
{
    if(initSharedDirectory()){
        QString sFilePath(m_sDirectory + "/" + m_sDefaultFileName + QString::number(m_iFileIndex++) + "_raw.fif");

        if(QFile::copy(sSourcePath, sFilePath)){
            QFile newFile(sFilePath);
            if(newFile.open(QIODevice::ReadWrite)){
                FIFFLIB::FiffStream stream(&newFile);
                stream.skipRawData(newFile.bytesAvailable());
                stream.finish_writing_raw();

                newFile.close();
            }
        }
    }
}

//=============================================================================================================

void FiffFileSharer::initWatcher()
{
    if(initSharedDirectory()){
        clearSharedDirectory();
        m_fileWatcher.addPath(m_sDirectory);
        connect(&m_fileWatcher, &QFileSystemWatcher::directoryChanged,
                this, &FiffFileSharer::onDirectoryChanged, Qt::UniqueConnection);
        connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this, &FiffFileSharer::onFileChanged, Qt::UniqueConnection);
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
    for(auto& file : directory.entryList()){
        directory.remove(file);
    }
}

//=============================================================================================================

void FiffFileSharer::onDirectoryChanged(const QString &sPath)
{
    QString filePath(sPath + "/" + m_sDefaultFileName + QString::number(m_iFileIndex) + "_raw.fif");
    m_fileWatcher.addPath(filePath);
}

//=============================================================================================================

void FiffFileSharer::onFileChanged(const QString &sPath)
{
    emit newFileAtPath(sPath);
    m_fileWatcher.removePath(sPath);

    m_iFileIndex++;
}

//=============================================================================================================

bool FiffFileSharer::initSharedDirectory()
{
    QDir sharedDirectory(m_sDirectory);
    if(!sharedDirectory.exists()){
        sharedDirectory.mkpath(".");
    }

    return sharedDirectory.exists();
}
