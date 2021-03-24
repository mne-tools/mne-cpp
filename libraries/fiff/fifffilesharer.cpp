//=============================================================================================================
/**
 * @file     fifffilesharer.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     MArch, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief     FiffFileSharer class definition.
 *
 */


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fifffilesharer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

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
