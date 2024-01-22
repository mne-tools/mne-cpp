//=============================================================================================================
/**
 * @file     fifffilesharer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
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
 * @brief     FiffFileSharer class declaration.
 *
 */

#ifndef FIFFFILESHARER_H
#define FIFFFILESHARER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_io.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileSystemWatcher>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * FiffFileSharer provides a way to share fiff files while they are being saved in real time
 */
class FIFFSHARED_EXPORT FiffFileSharer : public QObject
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
     * Constructs a FiffFileSharer with default paramaters (based on m_sDefaultDirectory and m_sDefaultFileName)
     */
    FiffFileSharer();

    //=========================================================================================================
    /**
     * Constructs a FiffFileSharer to watch/save to sDirName
     *
     * @param[in] sDirName      Directory to/from which data will saved/read
     */
    FiffFileSharer(const QString& sDirName);

    //=========================================================================================================
    /**
     * Copies unfinished fiff file to set directory and appends relevant end tags to copy
     *
     * @param[in] sSourcePath   source file to ber copied
     */
    void copyRealtimeFile(const QString &sSourcePath);

    //=========================================================================================================
    /**
     * Sets member QFileSystemWatcher to watch set directory
     */
    void initWatcher();

private:

    //=========================================================================================================
    /**
     * Creates specified shared direcotry if it does not exist.
     *
     * @return Retruns whether shared direcotry exists in file structure.
     */
    bool initSharedDirectory();

    //=========================================================================================================
    /**
     * Clears shared directory of old shared files.
     */
    void clearSharedDirectory();

    //=========================================================================================================
    /**
     * Called when directory QFileSystemWatcher m_fileWatcher is watching gets updated/changed.
     * Adds next realtime file to be watched to m_fileWatcher
     *
     * @param[in] sPath     Path of directory that was changed.
     */
    void onDirectoryChanged(const QString &sPath);

    //=========================================================================================================
    /**
     * Called when a file QFileSystemWatcher m_fileWatcher is watching gets updated.
     * Emits path of new file and stops watching it.
     *
     * @param sPath
     */
    void onFileChanged(const QString &sPath);

    QFileSystemWatcher      m_fileWatcher;          /**< Watches m_sDirectory for new files. */

    QString                 m_sDirectory;           /**< Directory where files will be saved to / read from. */
    int                     m_iFileIndex;           /**< File counter to give files unique name */

signals:

    //=========================================================================================================
    /**
     * Emits path of new shared fiff file.
     *
     * @param[in] sPath     Path of new shared fiff file.
     */
    void newFileAtPath(const QString& sPath);

};
} //namespace

#endif // FIFFFILESHARER_H
