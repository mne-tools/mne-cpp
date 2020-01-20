//=============================================================================================================
/**
 * @file     extract.cpp
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
 * @brief    extract class definition.
 *
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "extract.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Extract::Extract(QWidget *parent)
: QMainWindow(parent)
{

}


//*************************************************************************************************************

#ifdef _WIN32

void Extract::extractTar()
{

    QProcess *m_qp7zipTar = new QProcess(this);
    m_qArguments.removeLast();
    m_qArguments << (m_qCurrentPath + "\\sample.tar");
    connect(m_qp7zipTar, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), m_qp7zipTar, &QProcess::deleteLater);
    m_qp7zipTar->start(m_q7zipPath, m_qArguments);
    m_qp7zipTar->waitForFinished(-1);

    QFile tarFile("sample.tar");
    tarFile.remove();
    tarFile.close();

    emit extractionDone();
}


//*************************************************************************************************************

void Extract::extractGz(QString archivePath)
{
    m_qArguments.clear();
    QFile oldData("sample.tar");
    oldData.remove();
    oldData.close();
    m_qCurrentPath = archivePath;
    m_qArguments << "x" << "-aoa" << "-y" << (m_qCurrentPath + "\\sample.tar.gz");
    QProcess *m_qp7zipGz = new QProcess(this);
    connect(m_qp7zipGz, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), m_qp7zipGz, &QProcess::deleteLater);
    connect(m_qp7zipGz, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &Extract::extractTar);
    connect(m_qp7zipGz, &QProcess::errorOccurred, this, &Extract::zipperError);
    m_qp7zipGz->start(m_q7zipPath, m_qArguments);
    qDebug() << m_qp7zipGz->error();
}


//*************************************************************************************************************

void Extract::beginExtraction(QString zip, QString current)
{
    if (zip == NULL){m_q7zipPath = QDir::toNativeSeparators("\"C:/Program\ Files/7-Zip/7z.exe\"");}
    else{m_q7zipPath = zip;}
    extractGz(current);
}


//*************************************************************************************************************

#else //Linux & OSX

void Extract::beginExtraction()
{
    system("tar -zxf sample.tar.gz");
    emit extractionDone();
}

//#else
//#   error "Unknown compiler" // we don't want to have a compilation error on an unknown os
#endif

//*************************************************************************************************************

Extract::~Extract()
{

}
