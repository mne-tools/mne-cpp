//=============================================================================================================
/**
 * @file     fifffilesharer.h
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

namespace FIFFLIB
{
class FIFFSHARED_EXPORT FiffFileSharer : public QObject
{
    Q_OBJECT
public:

    //=========================================================================================================
    FiffFileSharer();

    //=========================================================================================================
    FiffFileSharer(const QString& sDirName);

    //=========================================================================================================
    void copyRealtimeFile(const QString &sSourcePath);

    //=========================================================================================================
    void initWatcher();

private:

    //=========================================================================================================
    bool initSharedDirectory();

    //=========================================================================================================
    void onDirectoryChanged(const QString &sPath);

    //=========================================================================================================
    void onFileChanged(const QString &sPath);



    QFileSystemWatcher      m_fileWatcher;

    QString                 m_sDirectory;
    int                     m_iFileIndex;

signals:
    void newFileAtPath(const QString& sPath);

};
} //namespace

#endif // FIFFFILESHARER_H
