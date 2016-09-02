//=============================================================================================================
/**
* @file     Extract.h
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
* @brief     Extract class declaration.
*
*/

#ifndef EXTRACT_H
#define EXTRACT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QMainWindow>
#include <QString>
#include <QtCore>
#include <QFile>
#include <QObject>
#include <QDebug>
#include <QIODevice>
#include <QProcess>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE Extract
//=============================================================================================================

using namespace std;

namespace Ui {
class Extract;
}

//*************************************************************************************************************
//=============================================================================================================
// Extract FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================

/**
* Extracts the sample data set from .tar.gz. On Windows it utilizes 7zip, as .tar.gz is not a supported format.
* If the user hasn't installed 7zip or has installed it onto a different location then the standard path, it sends
* an error signal which prompts the Downloader class to change it's GUI. On Linux, it uses a system call and thus
* extracts the data set with the Linux native method for extraction of .tar.gz. No further software is needed.
* Once the extraction is completed, it emits the extractionDone signal which prompts Downloader to display "Done".
* OSX is currently unsupported.
*
* @brief Extracts the sample data set.
*/

class Extract : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<Extract> SPtr;            /**< Shared pointer type for mne_sample_set_downloader. */
    typedef QSharedPointer<const Extract> ConstSPtr; /**< Const shared pointer type for mne_sample_set_downloader. */

    //=========================================================================================================
    /**
    * Constructs a Extract object.
    */
    explicit Extract(QWidget *parent = 0);
    ~Extract();

private:
    Ui::Extract *ui;
    QStringList m_qArguments;
    QString m_qCurrentPath;
    QString m_q7zipPath;

signals:
    void extractionDone();


#ifdef _WIN32
public:
    void beginExtraction(QString, QString);

signals:
    void zipperError();

private:
    void extractGz(QString);

private slots:
    void extractTar();

#endif

#ifdef __linux__
public:
    void beginExtraction();

#endif

};

#endif // EXTRACT_H
