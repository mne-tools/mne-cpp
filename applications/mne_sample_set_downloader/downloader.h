//=============================================================================================================
/**
* @file     mne_sample_set_downloader.h
* @author   Louis Eichhorst <louis.eichhorst@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Louis Eichhorst and Matti Hamalainen. All rights reserved.
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
* @brief     Downloader class declaration.
*
*/

#ifndef DOWNLOADER_H
#define DOWNLOADER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "Extract.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QString>
#include <QtCore>
#include <QFile>
#include <QSharedPointer>
#include <QDebug>
#include <QIODevice>
#include <QFileDialog>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DOWNLOADER
//=============================================================================================================

using namespace std;

namespace Ui {
class Downloader;
}

/**
* This class builds a Downloader Object an GUI. It downloads the sample data set and stores it in a temporary file
* for the Extract class to extract once it recieves the download_finished signal.
* The temporary file is deleted on leaving the GUI. The class also handles error and finalisation signals
* from the extract class and adjusts the GUI accordingly (i.e. displays "Done" on finalization and ask if/where 7zip is
* installed if the Extract class can't find it).
*
* @brief Downloads the sample data set and manages the GUI
*/

class Downloader : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<Downloader> SPtr;            /**< Shared pointer type for Downloader. */
    typedef QSharedPointer<const Downloader> ConstSPtr; /**< Const shared pointer type for Downloader. */

    //=========================================================================================================
    /**
    * Constructs a Downloader object.
    */
    explicit Downloader(QWidget *parent = 0);
    bool boolDownloadStatus = false;
    ~Downloader();

public slots:
    void done();

private slots:
    void downloadFinished();
    void on_downloadButton_clicked();
    void on_exitButton_clicked();
    void dataReady();
    void downloadProgress(qint64,qint64);

private:
    Ui::Downloader *ui;
    QSharedPointer<QNetworkReply> m_qReply;
    QFile m_qFile;
    Extract m_extractor;
    QString m_qCurrentPath;

#ifdef _WIN32
public slots:
    void checkZipperPath();

private slots:
    void on_extractButton_clicked();
    void on_toolButton_clicked();

private:
    void readyToExtract(bool, QString, QString);

#endif

#ifdef __linux__
private:
    void readyToExtract(bool);

#endif


};

#endif // DOWNLOADER_H
