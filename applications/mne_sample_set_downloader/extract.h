//=============================================================================================================
/**
* @file     extract.h
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
* @brief     extract class declaration.
*
*/

#ifndef EXTRACT_H
#define EXTRACT_H

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
// DEFINE NAMESPACE EXTRACT
//=============================================================================================================

using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// EXTRACT FORWARD DECLARATIONS
//=============================================================================================================

/**
*
* @brief Extracts the sample data set.
*/

class Extract : public QMainWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<Extract> SPtr;              /**< Shared pointer type for Extract. */
    typedef QSharedPointer<const Extract> ConstSPtr;   /**< Const shared pointer type for Extract. */

    //=========================================================================================================
    /**
    * Constructs a extract object.
    */
    explicit Extract(QWidget *parent = 0);
    ~Extract();

signals:
    //=========================================================================================================
    /**
    * Emitted if the extraction has finished.
    */
    void extractionDone();

private:
    QStringList     m_qArguments;                       /**< List of extractionarguments for 7zip. */
    QString         m_qCurrentPath;                     /**< Temporary filepath of the sample data set. */
    QString         m_q7zipPath;                        /**< Location of 7z.exe, */

#ifdef _WIN32
public:
    //=========================================================================================================
    /**
    * Looks for 7zip at the given filepath and extracts the file at the other filepath.
    *
    * @param[in] zip            Path to 7z.exe
    *
    * @param[in] current        Path to the .tar.gz
    */
    void beginExtraction(QString zip, QString current);

signals:
    //=========================================================================================================
    /**
    * Emitted if 7zip cannot be opened.
    */
    void zipperError();

private:
    //=========================================================================================================
    /**
    * Extracts from .tar.gz to .tar using 7zip
    *
    * @param[in] archivePath    Path to the .tar.gz
    */
    void extractGz(QString archivePath);

    //=========================================================================================================
    /**
    * Extracts the sample set from .tar using 7zip
    */
    void extractTar();

#else //Linux & OSX
public:
    //=========================================================================================================
    /**
    * Extracts the .tar.gz using a systemcall.
    */
    void beginExtraction();

#endif

};

#endif // EXTRACT_H
