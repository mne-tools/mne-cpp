//=============================================================================================================
/**
 * @file     gusbampsetupprojectwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the gUSBampSetupProjectWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampsetupprojectwidget.h"
#include "ui_gusbampsetupprojectwidget.h"
#include "../gusbamp.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAMPPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupProjectWidget::GUSBAmpSetupProjectWidget(GUSBAmp* pGUSBAmp, QWidget *parent)
: QWidget(parent)
, ui(new Ui::GUSBAmpSetupProjectWidget)
, m_pGUSBAmp(pGUSBAmp)
{
    ui->setupUi(this);

    //Connect EEG hat
    connect(ui->m_qPushButton_EEGCap, &QPushButton::released, this, &GUSBAmpSetupProjectWidget::changeCap);

    // Connect QLineEdit's
    connect(ui->m_qLineEdit_EEGCap, static_cast<void (QLineEdit::*)(const QString &)>(&QLineEdit::textEdited),
            this, &GUSBAmpSetupProjectWidget::changeQLineEdits);
}

//=============================================================================================================

GUSBAmpSetupProjectWidget::~GUSBAmpSetupProjectWidget()
{
    delete ui;
}

//=============================================================================================================

void GUSBAmpSetupProjectWidget::initGui()
{
}

//=============================================================================================================

void GUSBAmpSetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                ../resources/mne_scan/plugins/gusbamp/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL){
        path = ui->m_qLineEdit_EEGCap->text();
    }

    ui->m_qLineEdit_EEGCap->setText(path);
    //m_pGUSBAmp->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}

//=============================================================================================================

void GUSBAmpSetupProjectWidget::changeQLineEdits()
{
    //m_pGUSBAmp->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}
