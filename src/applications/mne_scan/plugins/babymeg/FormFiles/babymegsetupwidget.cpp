//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     babymegsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    BabyMEGSetupWidget class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegsetupwidget.h"

#include "babymegsquidcontroldgl.h"

#include "../babymeg.h"
#include "../babymegclient.h"

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

BabyMEGSetupWidget::BabyMEGSetupWidget(BabyMEG* p_pBabyMEG,
                                       QWidget* parent)
: QWidget(parent)
, m_pBabyMEG(p_pBabyMEG)
{
    ui.setupUi(this);

    //rt server fiffInfo received
    connect(m_pBabyMEG, &BabyMEG::fiffInfoAvailable, this,
            &BabyMEGSetupWidget::setSamplingFrequency);
}

//=============================================================================================================

BabyMEGSetupWidget::~BabyMEGSetupWidget()
{
}

//=============================================================================================================

void BabyMEGSetupWidget::setSamplingFrequency()
{
    if(m_pBabyMEG->m_pFiffInfo) {
        this->ui.m_qLabel_sps->setText(QString("%1").arg(m_pBabyMEG->m_pFiffInfo->sfreq));
    }
}
