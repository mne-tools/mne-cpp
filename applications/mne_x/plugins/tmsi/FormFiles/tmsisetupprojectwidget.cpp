//=============================================================================================================
/**
* @file     tmsisetupprojectwidget.cpp
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
* @brief    Contains the implementation of the TMSISetupProjectWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsisetupprojectwidget.h"
#include "ui_tmsisetupprojectwidget.h"
#include "../tmsi.h"

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSISetupProjectWidget::TMSISetupProjectWidget(TMSI* pTMSI, QWidget *parent)
: QWidget(parent)
, ui(new Ui::TMSISetupProjectWidget)
, m_pTMSI(pTMSI)
{
    ui->setupUi(this);

    // Connect write to file options
    connect(ui->m_qPushButton_NewProject, &QPushButton::released, this, &TMSISetupProjectWidget::changeProject);
    connect(ui->m_qPushButton_NewSubject, &QPushButton::released, this, &TMSISetupProjectWidget::changeSubject);
    connect(ui->m_qPushButton_FiffRecordFile, &QPushButton::released, this, &TMSISetupProjectWidget::changeOutputFile);

    //Connect EEG hat
    connect(ui->m_qPushButton_EEGCap, &QPushButton::released, this, &TMSISetupProjectWidget::changeCap);
}


//*************************************************************************************************************

TMSISetupProjectWidget::~TMSISetupProjectWidget()
{
    delete ui;
}


//*************************************************************************************************************

void TMSISetupProjectWidget::changeProject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Project Directory"),
                                                     "/home",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

//    if(path==NULL)
//        path = ui.m_lineEdit_outputDir->text();

//    ui.m_lineEdit_outputDir->setText(path);
//    m_pTMSI->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void TMSISetupProjectWidget::changeSubject()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Subject Directory"),
                                                     "/home",
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

//    if(path==NULL)
//        path = ui.m_lineEdit_outputDir->text();

//    ui.m_lineEdit_outputDir->setText(path);
//    m_pTMSI->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void TMSISetupProjectWidget::changeOutputFile()
{
    QString path = QFileDialog::getSaveFileName(
                this,
                "Save to fif file",
                "mne_x_plugins/resources/tmsi/EEG_data_001_raw.fif",
                 tr("Fif files (*.fif)"));

//    if(path==NULL)
//        path = ui.m_lineEdit_outputDir->text();

//    ui.m_lineEdit_outputDir->setText(path);
//    m_pTMSI->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void TMSISetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                "mne_x_plugins/resources/tmsi/loc_files",
                                                 tr("Electrode location files (*.elc)"));

//    if(path==NULL)
//        path = ui.m_lineEdit_CurrentEEGHat->text();

//    ui.m_lineEdit_CurrentEEGHat->setText(path);
//    m_pTMSI->m_sElcFilePath = ui.m_lineEdit_CurrentEEGHat->text();
}
