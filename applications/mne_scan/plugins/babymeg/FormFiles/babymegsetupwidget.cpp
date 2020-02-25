//=============================================================================================================
/**
 * @file     babymegsetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    BabyMEGSetupWidget class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegsetupwidget.h"
#include "babymegaboutwidget.h"

#include "babymegsquidcontroldgl.h"

#include "../babymeg.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>
#include <QComboBox>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGSetupWidget::BabyMEGSetupWidget(BabyMEG* p_pBabyMEG, QWidget* parent)
: QWidget(parent)
, m_pBabyMEG(p_pBabyMEG)
, m_bIsInit(false)
{
    ui.setupUi(this);

    connect(m_pBabyMEG, &BabyMEG::cmdConnectionChanged, this,
            &BabyMEGSetupWidget::cmdConnectionChanged);

    //rt server fiffInfo received
    connect(m_pBabyMEG, &BabyMEG::fiffInfoAvailable, this,
            &BabyMEGSetupWidget::fiffInfoReceived);

    //About
    connect(ui.m_qPushButton_About, &QPushButton::released,
            this, &BabyMEGSetupWidget::showAboutDialog);

    //SQUID Control
    connect(ui.m_qPushButtonSqdCtrl, &QPushButton::released,
            this, &BabyMEGSetupWidget::showSqdCtrlDialog);

    ui.m_qPushButtonSqdCtrl->setVisible(false);

    this->init();
}

//=============================================================================================================

BabyMEGSetupWidget::~BabyMEGSetupWidget()
{
}

//=============================================================================================================

void BabyMEGSetupWidget::init()
{
}

//=============================================================================================================

void BabyMEGSetupWidget::cmdConnectionChanged(bool p_bConnectionStatus)
{
    Q_UNUSED(p_bConnectionStatus)
}

//=============================================================================================================

void BabyMEGSetupWidget::fiffInfoReceived()
{
    if(m_pBabyMEG->m_pFiffInfo)
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMEG->m_pFiffInfo->sfreq));
}

//=============================================================================================================

void BabyMEGSetupWidget::showAboutDialog()
{
    BabyMEGAboutWidget aboutDialog(this);
    aboutDialog.exec();
}

//=============================================================================================================

void BabyMEGSetupWidget::showSqdCtrlDialog()
{
//    BabyMEGSQUIDControlDgl m_pSQUIDCtrlDlg(m_pBabyMEG,this);
//    m_pSQUIDCtrlDlg.exec();
}
