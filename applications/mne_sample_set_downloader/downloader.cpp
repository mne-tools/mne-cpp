//=============================================================================================================
/**
 * @file     downloader.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>
 * @version  dev
 * @date     August, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch, Louis Eichhorst. All rights reserved.
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
 * @brief    downloader class definition.
 *
 */


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "downloader.h"
#include "extract.h"
#include "ui_downloader.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>
#include <QMessageBox>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Downloader::Downloader(QWidget *parent)
: QMainWindow(parent),
    ui(new Ui::Downloader)
{
    ui->setupUi(this);
    ui->m_progressBar->hide();
    ui->m_progressBar->setValue(0);
    ui->m_filePath->setVisible(false);
    ui->m_extractButton->setVisible(false);
    ui->m_extractButton->setEnabled(false);
    ui->m_filePath->setText(QDir::toNativeSeparators("C:/Program\ Files/7-Zip/7z.exe"));
    m_qCurrentPath = QDir::toNativeSeparators(QDir::currentPath());
    ui->m_toolButton->setVisible(false);
    ui->m_toolButton->setEnabled(false);
    m_bDownloadStatus = false;
    m_bExtractionDone = false;
    if (!QDir("MNE-sample-data").exists())
    {
        ui->m_downloadButton->setEnabled(false);
        ui->m_label->setText("Unknown error, please recompile");
        m_bExtractionDone = true;
    }
    int numberOfFiles = QDir("MNE-sample-data/MEG/sample").count();
    if ((QDir("MNE-sample-data/MEG/sample").exists()) && (numberOfFiles > 60))
        ui->m_label->setText("Sample data seems to exist\nReplace data?");
    connect(ui->m_downloadButton, &QPushButton::clicked, this, &Downloader::onDownloadButtonClicked);
    connect(ui->m_exitButton, &QPushButton::clicked, this, &Downloader::onExitButtonClicked);

#ifdef _WIN32
    connect(ui->m_extractButton, &QPushButton::clicked, this, &Downloader::onExtractButtonClicked);
    connect(ui->m_toolButton, &QToolButton::clicked, this, &Downloader::onToolButtonClicked);
#endif


}


//*************************************************************************************************************

Downloader::~Downloader()
{
    delete ui;
    m_qFile.remove();
}

//*************************************************************************************************************

void Downloader::done()
{
    m_bExtractionDone = true;
    ui->m_label->setText("done");
}


//*************************************************************************************************************

void Downloader::onDownloadButtonClicked()
{
    ui->m_downloadButton->setEnabled(false);
    ui->m_progressBar->show();
    ui->m_label->setText("DOWNLOADING FILES...");

    QNetworkAccessManager *manager = new QNetworkAccessManager;
    QString url = "ftp://surfer.nmr.mgh.harvard.edu/pub/data/MNE-sample-data-processed.tar.gz"; //"ftp://surfer.nmr.mgh.harvard.edu/pub/data/test_data.tar.gz";
    m_qReply = QSharedPointer<QNetworkReply> (manager->get(QNetworkRequest(url)));

    connect(m_qReply.data(), &QNetworkReply::readyRead, this, &Downloader::dataReady);
    connect(m_qReply.data(), &QNetworkReply::finished, this, &Downloader::downloadFinished);

    if (m_qFile.fileName() != "sample.tar.gz") {
        m_qFile.setFileName("sample.tar.gz");
    }

    if(!m_qFile.open(QIODevice::Append)) {
        qDebug() << "Downloader::onDownloadButtonClicked - Cannot open file";
    }

    connect(m_qReply.data(), &QNetworkReply::downloadProgress, this, &Downloader::downloadProgress);
    QEventLoop loop;
    connect(m_qReply.data(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    return;
}

//*************************************************************************************************************

void Downloader::onExitButtonClicked()
{
    if (m_bExtractionDone == true)
    {
        this->close();
        m_qFile.remove();
        m_qFile.close();
    }
    if (m_bExtractionDone == false)
    {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setText("Extraction not finished. Do you really want to quit?");
        warning.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        int ret = warning.exec();
        if (ret == QMessageBox::Yes)
        {
            this->close();
            m_qFile.remove();
            m_qFile.close();
        }
        warning.close();
    }
}

//*************************************************************************************************************

void Downloader::dataReady()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    return;
}

void Downloader::downloadProgress(qint64 recieved, qint64 total)
{
    ui->m_progressBar->setMaximum(total);
    ui->m_progressBar->setValue(recieved);
}

//*************************************************************************************************************

#ifdef _WIN32

void Downloader::downloadFinished()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    m_qFile.close();
    m_bDownloadStatus = true;
    QString zipPath ="\"" + QString (ui->m_filePath->text()) + "\"";
    ui->m_progressBar->setEnabled(false);
    readyToExtract(m_bDownloadStatus, zipPath, m_qCurrentPath);
    return;
}

//*************************************************************************************************************

void Downloader::checkZipperPath()
{
    ui->m_label->setText("7zip installed? Please check the path");
    ui->m_extractButton->setVisible(true);
    ui->m_extractButton->setEnabled(true);
    ui->m_downloadButton->setVisible(false);
    ui->m_downloadButton->setEnabled(false);
    ui->m_progressBar->setVisible(false);
    ui->m_filePath->setEnabled(true);
    ui->m_filePath->setVisible(true);
    ui->m_toolButton->setVisible(true);
    ui->m_toolButton->setEnabled(true);
}

//*************************************************************************************************************

void Downloader::onExtractButtonClicked()
{
    QString zipPath = "\"" + QString(QDir::toNativeSeparators(ui->m_filePath->text())) + "\"";
    ui->m_extractButton->setEnabled(false);
    ui->m_filePath->setEnabled(false);
    ui->m_toolButton->setEnabled(false);
    readyToExtract(m_bDownloadStatus, zipPath, m_qCurrentPath);

}

//*************************************************************************************************************

void Downloader::onToolButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home",QFileDialog::DontResolveSymlinks) + "\\7z.exe";
    ui->m_filePath->setText(QDir::toNativeSeparators(dir));
}


//*************************************************************************************************************

void Downloader::readyToExtract(bool stat, QString zip, QString current)
{
    if (stat == true)
    {
        ui->m_label->setText("EXTRACTING...");
        ui->m_progressBar->setVisible(false);
        QObject::connect(&m_extractor, &Extract::zipperError, this, &Downloader::checkZipperPath);
        connect(&m_extractor, &Extract::extractionDone, this, &Downloader::done);
        m_extractor.beginExtraction(zip, current);
    }
}

//*************************************************************************************************************


#else //Linux & OSX

void Downloader::readyToExtract(bool stat)
{
    if (stat == true)
    {
        ui->m_label->setText("EXTRACTING...");
        m_extractor.beginExtraction();
    }
}

//*************************************************************************************************************

void Downloader::downloadFinished()
{
    if (m_qReply != NULL)
        m_qFile.write(m_qReply->readAll());
    m_bDownloadStatus = true;
    ui->m_progressBar->setEnabled(false);
    readyToExtract(m_bDownloadStatus);
    return;
}

#endif

//TODO: Add OSX support
