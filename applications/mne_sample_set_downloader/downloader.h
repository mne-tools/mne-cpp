//=============================================================================================================
/**
 * @file     downloader.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>
 * @version  dev
 * @date     August, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Louis Eichhorst. All rights reserved.
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
 * @brief     downloader class declaration.
 *
 */

#ifndef DOWNLOADER_H
#define DOWNLOADER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "extract.h"

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


//*************************************************************************************************************
//=============================================================================================================
// DOWNLOADER FORWARD DECLARATIONS
//=============================================================================================================

/**
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
    ~Downloader();

    //=========================================================================================================
    /**
     * Called once the extraction is done. Changes GUI and sets m_bExtraction = true.
     */
    void done();

private:
    //=========================================================================================================
    /**
     * Starts the Download and changes GUI.
     */
    void onDownloadButtonClicked();

    //=========================================================================================================
    /**
     * Checks if extraction is finished.
     *
     * Closes GUI and deletes temporary files if m_bExtraction = true, triggers warning if not.
     */
    void onExitButtonClicked();

    //=========================================================================================================
    /**
     * Calls the extract class once the download is finished.
     */
    void downloadFinished();

    //=========================================================================================================
    /**
     * Writes data onto harddrive once it arrives.
     */
    void dataReady();

    //=========================================================================================================
    /**
     * Displays downloadprogress on the GUI.
     *
     * @param[in] recieved   Size of recieved data
     *
     * @param[in] total      Total size of the sample data set
     */
    void downloadProgress(qint64 recieved, qint64 total);

    Ui::Downloader *ui;                                     /**< Sets up the GUI. */
    QSharedPointer<QNetworkReply>   m_qReply;               /**< Networkreply */
    QFile                           m_qFile;                /**< Temporary file for the dataset. */
    Extract                         m_extractor;            /**< Extractor for the data set. */
    QString                         m_qCurrentPath;         /**< Location of the temporary file. */
    bool                            m_bDownloadStatus;      /**< True if download was completed. */
    bool                            m_bExtractionDone;      /**< True if extraction was completed. */


#ifdef _WIN32
public:
    //=========================================================================================================
    /**
     * Changes GUI for filepathinput if 7zip isn't found automatically.
     */
    void checkZipperPath();

private:
    //=========================================================================================================
    /**
     * Tries to extract again using the new filepath if the extract button is pressed.
     */
    void onExtractButtonClicked();

    //=========================================================================================================
    /**
     * Opens filepath GUI for easy input.
     */
    void onToolButtonClicked();

    //=========================================================================================================
    /**
     * Starts Extraction if stat=true. Hands the extractor the path of the temporary file and of 7zip.
     *
     * @param[in] stat       Downloadstatus. stat = true if download has finished
     *
     * @param[in] zip        Path to 7z.exe
     *
     * @param[in] current    Path to the temporary .tar.gz file
     */
    void readyToExtract(bool stat, QString zip, QString current);


#else //Linux & OSX

private:
    //=========================================================================================================
    /**
     * Starts Extraction if stat=true.
     *
     * @param[in] stat       Downloadstatus. stat = true if download has finished
     */
    void readyToExtract(bool stat);

#endif


};

#endif // DOWNLOADER_H
