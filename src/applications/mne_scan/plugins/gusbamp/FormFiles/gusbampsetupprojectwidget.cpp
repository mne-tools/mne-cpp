//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     gusbampsetupprojectwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2016
 * @brief    Contains the implementation of the gUSBampSetupProjectWidget class.
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
                                                "../resources/mne_scan/plugins/gusbamp/loc_files",
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
