//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsisetupprojectwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2014
 * @brief    Contains the implementation of the TMSISetupProjectWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsisetupprojectwidget.h"
#include "ui_tmsisetupprojectwidget.h"
#include "../tmsi.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSISetupProjectWidget::TMSISetupProjectWidget(TMSI* pTMSI, QWidget *parent)
: QWidget(parent)
, ui(new Ui::TMSISetupProjectWidget)
, m_pTMSI(pTMSI)
{
    ui->setupUi(this);

    //Connect EEG hat
    connect(ui->m_qPushButton_EEGCap, &QPushButton::released, this, &TMSISetupProjectWidget::changeCap);

    // Connect QLineEdit's
    connect(ui->m_qLineEdit_EEGCap, static_cast<void (QLineEdit::*)(const QString &)>(&QLineEdit::textEdited),
            this, &TMSISetupProjectWidget::changeQLineEdits);
}

//=============================================================================================================

TMSISetupProjectWidget::~TMSISetupProjectWidget()
{
    delete ui;
}

//=============================================================================================================

void TMSISetupProjectWidget::initGui()
{
    // Init location of layout file
    ui->m_qLineEdit_EEGCap->setText(m_pTMSI->m_sElcFilePath);
}

//=============================================================================================================

void TMSISetupProjectWidget::changeCap()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                "Change EEG cap layout",
                                                "../resources/mne_scan/plugins/tmsi/loc_files",
                                                 tr("Electrode location files (*.elc)"));

    if(path==NULL)
        path = ui->m_qLineEdit_EEGCap->text();

    ui->m_qLineEdit_EEGCap->setText(path);
    m_pTMSI->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}

//=============================================================================================================

void TMSISetupProjectWidget::changeQLineEdits()
{
    m_pTMSI->m_sElcFilePath = ui->m_qLineEdit_EEGCap->text();
}
