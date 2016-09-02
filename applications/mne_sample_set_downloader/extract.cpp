//=============================================================================================================
/**
* @file     Extract.cpp
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
* @brief    Extract class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "Extract.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Extract::Extract(QWidget *parent) :
    QMainWindow(parent)
{

}

#ifdef _WIN32


void Extract::extractTar()
{

    QProcess *m_qp7zipTar = new QProcess(this);
    m_qArguments.removeLast();
    m_qArguments << (m_qCurrentPath + "\\MNE-Sample-Data.tar");
    connect(m_qp7zipTar, SIGNAL(finished(int,QProcess::ExitStatus)), m_qp7zipTar, SLOT(deleteLater()));
    m_qp7zipTar->start(m_q7zipPath, m_qArguments);
    m_qp7zipTar->waitForFinished(-1);

    QFile tarFile("MNE-Sample-Data.tar");
    tarFile.remove();
    tarFile.close();

    emit extractionDone();

}

void Extract::extractGz(QString archivePath)
{
    m_qArguments.clear();
    QFile oldData("MNE-Sample-Data.tar");
    oldData.remove();
    oldData.close();
    m_qCurrentPath = archivePath;
    m_qArguments << "x" << "-aoa" << (m_qCurrentPath + "\\sample.tar.gz");
    QProcess *m_qp7zipGz = new QProcess(this);
    connect(m_qp7zipGz, SIGNAL(finished(int,QProcess::ExitStatus)), m_qp7zipGz, SLOT(deleteLater()));
    connect(m_qp7zipGz, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(extractTar()));
    connect(m_qp7zipGz, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SIGNAL(zipperError()));
    m_qp7zipGz->start(m_q7zipPath, m_qArguments);
    qDebug() << m_qp7zipGz->error();

}

void Extract::beginExtraction(QString zip, QString current)
{
    if (zip == NULL)
        m_q7zipPath = QDir::toNativeSeparators("\"C:/Program\ Files/7-Zip/7z.exe\"");
    else
        m_q7zipPath = zip;
    extractGz(current);
}

#elif __linux__

void Extract::beginExtraction()
{
    system("tar -zxf sample.tar.gz");
    emit extractionDone();
}

#else
#   error "Unknown compiler"
#endif


Extract::~Extract()
{

}
