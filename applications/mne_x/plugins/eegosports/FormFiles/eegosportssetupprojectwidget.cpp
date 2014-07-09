//=============================================================================================================
/**
* @file     eegosportssetupprojectwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the EEGoSportsSetupProjectWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportssetupprojectwidget.h"
#include "ui_eegosportssetupprojectwidget.h"
#include "../eegosports.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGoSportsPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsSetupProjectWidget::EEGoSportsSetupProjectWidget(EEGoSports* pEEGoSports, QWidget *parent)
: QWidget(parent)
, ui(new Ui::EEGoSportsSetupProjectWidget)
, m_pEEGoSports(pEEGoSports)
{
    ui->setupUi(this);

    // Connect write to file options
    connect(ui->m_qPushButton_NewProject, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::addProject);
    connect(ui->m_qPushButton_NewSubject, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::addSubject);
    connect(ui->m_qPushButton_FiffRecordFile, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::changeOutputFile);

    // Connect drop down menus
    connect(ui->m_qComboBox_SubjectSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupProjectWidget::generateFilePath);
    connect(ui->m_qComboBox_ProjectSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &EEGoSportsSetupProjectWidget::generateFilePath);

    //Connect EEG hat
    connect(ui->m_qPushButton_EEGCap, &QPushButton::released, this, &EEGoSportsSetupProjectWidget::changeCap);
}


//*************************************************************************************************************

EEGoSportsSetupProjectWidget::~EEGoSportsSetupProjectWidget()
{
    delete ui;
}

//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::initGui()
{
    // Init output file path
    ui->m_qLineEdit_FiffRecordFile->setText(m_pEEGoSports->m_sOutputFilePath);

    // Init location of layout file
    ui->m_qLineEdit_EEGCap->setText(m_pEEGoSports->m_sElcFilePath);

    // Init project and subject menus
    ui->m_qComboBox_ProjectSelection->addItem("Sequence_01");
    ui->m_qComboBox_SubjectSelection->addItem("Subject_01");
    ui->m_qComboBox_ProjectSelection->addItem("Sequence_02");
    ui->m_qComboBox_SubjectSelection->addItem("Subject_02");

    // Init file name
    generateFilePath();
}


//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::addProject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Project Directory"),
                                                     m_pEEGoSports->m_qStringResourcePath,
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    // Split string to get created or existing target dir with the name of the project
    QStringList list = path.split("/");

    // Add to combo box
    ui->m_qComboBox_ProjectSelection->addItem(list.at(list.size()-1));
    ui->m_qComboBox_ProjectSelection->setCurrentIndex(ui->m_qComboBox_ProjectSelection->count()-1);
}


//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::addSubject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Subject Directory"),
                                                     m_pEEGoSports->m_qStringResourcePath,
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    // Split string to get created or existing target dir with the name of the project
    QStringList list = path.split("/");

    // Add to combo box
    ui->m_qComboBox_SubjectSelection->addItem(list.at(list.size()-1));
    ui->m_qComboBox_SubjectSelection->setCurrentIndex(ui->m_qComboBox_SubjectSelection->count()-1);
}


//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::changeOutputFile()
{
    QString path = QFileDialog::getSaveFileName(
                this,
                "Save to fif file",
                "mne_x_plugins/resources/eegosports/EEG_data_001_raw.fif",
                 tr("Fif files (*.fif)"));

    if(path==NULL)
        path = ui->m_qLineEdit_FiffRecordFile->text();

    ui->m_qLineEdit_FiffRecordFile->setText(path);
    m_pEEGoSports->m_sOutputFilePath = ui->m_qLineEdit_FiffRecordFile->text();
}


//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                "mne_x_plugins/resources/tmsi/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL)
        path = ui->m_qLineEdit_EEGCap->text();

    ui->m_qLineEdit_EEGCap->setText(path);
    m_pEEGoSports->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}


//*************************************************************************************************************

void EEGoSportsSetupProjectWidget::generateFilePath(int index)
{
    Q_UNUSED(index);

    // Generate file name with timestamp
    QDate date;
    QString fileName = QString ("%1_%2_%3_EEG_001_raw.fif").arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day());

    // Append new file name, subject and project
    QString resourcePath = m_pEEGoSports->m_qStringResourcePath;
    resourcePath.append(ui->m_qComboBox_ProjectSelection->currentText());
    resourcePath.append("/");
    resourcePath.append(ui->m_qComboBox_SubjectSelection->currentText());
    resourcePath.append("/");
    resourcePath.append(fileName);

    ui->m_qLineEdit_FiffRecordFile->setText(resourcePath);
    m_pEEGoSports->m_sOutputFilePath = resourcePath;
}
