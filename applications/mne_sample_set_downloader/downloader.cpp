//=============================================================================================================
/**
* @file     Downloader.cpp
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
* @brief    Downloader class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "Downloader.h"
#include "Extract.h"
#include "ui_downloader.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

#ifdef _WIN32

void Downloader::on_extractButton_clicked()
{
    QString zipPath = "\"" + QString(QDir::toNativeSeparators(ui->filePath->text())) + "\"";
    readyToExtract(boolDownloadStatus, zipPath, m_qCurrentPath);
    ui->extractButton->setEnabled(false);

}

void Downloader::checkZipperPath()
{
    ui->label->setText("7zip installed? Please check the path");
    ui->extractButton->setVisible(true);
    ui->extractButton->setEnabled(true);
    ui->downloadButton->setVisible(false);
    ui->downloadButton->setEnabled(false);
    ui->progressBar->setVisible(false);
    ui->filePath->setVisible(true);
    ui->toolButton->setVisible(true);
    ui->toolButton->setEnabled(true);
}

void Downloader::on_toolButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home",QFileDialog::DontResolveSymlinks) + "\\7z.exe";
    ui->filePath->setText(QDir::toNativeSeparators(dir));
}

void Downloader::readyToExtract(bool stat, QString zip, QString current)
{
    if (stat == true)
    {
        ui->label->setText("EXTRACTING...");
        ui->progressBar->setVisible(false);
        QObject::connect(&m_extractor, SIGNAL(zipperError()), this, SLOT(checkZipperPath()));
        connect(&m_extractor, SIGNAL(extractionDone()), this, SLOT(done()));
        m_extractor.beginExtraction(zip, current);
    }
}

void Downloader::downloadFinished()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    m_qFile.close();
    boolDownloadStatus = true;
    QString zipPath ="\"" + QString (ui->filePath->text()) + "\"";
    ui->progressBar->setEnabled(false);
    readyToExtract(boolDownloadStatus, zipPath, m_qCurrentPath);
    return;
}
#elif __linux__

void Downloader::readyToExtract(bool stat)
{
    if (stat == true)
    {
        ui->label->setText("EXTRACTING...");
        m_extractor.beginExtraction();
    }
}

void Downloader::downloadFinished()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    boolDownloadStatus = true;
    ui->progressBar->setEnabled(false);
    readyToExtract(boolDownloadStatus);
    return;
}

#endif

void Downloader::done()
{
    ui->label->setText("done");
}

//placeholder downloadFinished OSX

void Downloader::dataReady()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    return;
}

void Downloader::downloadProgress(qint64 recieved,qint64 total)
{
    ui->progressBar->setMaximum(total);
    ui->progressBar->setValue(recieved);
}


void Downloader::on_downloadButton_clicked()
{
    ui->downloadButton->setEnabled(false);
    ui->progressBar->show();
    ui->label->setText("DOWNLOADING FILES...");

    QNetworkAccessManager *manager = new QNetworkAccessManager;
    QString url = "ftp://surfer.nmr.mgh.harvard.edu/pub/data/MNE-sample-data-processed.tar.gz"; //"ftp://surfer.nmr.mgh.harvard.edu/pub/data/test_data.tar.gz";
    m_qReply = QSharedPointer<QNetworkReply> (manager->get(QNetworkRequest(url)));

    connect(m_qReply.data(),SIGNAL(readyRead()),this,SLOT(dataReady()));
    connect(m_qReply.data(),SIGNAL(finished()), this,SLOT(downloadFinished()));

    if (m_qFile.fileName() != "sample.tar.gz")
        m_qFile.setFileName("sample.tar.gz");
    m_qFile.open(QIODevice::Append);

    connect(m_qReply.data(), SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    QEventLoop loop;
    connect(m_qReply.data(), SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    return;
}

void Downloader::on_exitButton_clicked()
{
    this->close();
    m_qFile.remove();
    m_qFile.close();
    return;
}


Downloader::Downloader(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Downloader)
{
    ui->setupUi(this);
    ui->progressBar->hide();
    ui->progressBar->setValue(0);
    ui->filePath->setVisible(false);
    ui->extractButton->setVisible(false);
    ui->extractButton->setEnabled(false);
    ui->filePath->setText(QDir::toNativeSeparators("C:/Program\ Files/7-Zip/7z.exe"));
    m_qCurrentPath = QDir::toNativeSeparators(QDir::currentPath());
    ui->toolButton->setVisible(false);
    ui->toolButton->setEnabled(false);
    if (!QDir("MNE-sample-data").exists())
    {
        ui->downloadButton->setEnabled(false);
        ui->label->setText("Unknown error, please recompile");
    }
    int numberOfFiles = QDir("MNE-sample-data/MEG/sample").count();
    if ((QDir("MNE-sample-data/MEG/sample").exists()) && (numberOfFiles > 60))
        ui->label->setText("Sample data seems to exist\nReplace data?");

}

Downloader::~Downloader()
{
    delete ui;
    m_qFile.remove();
}
