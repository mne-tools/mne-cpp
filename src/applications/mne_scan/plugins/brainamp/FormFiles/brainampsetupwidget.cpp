//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the implementation of the BrainAMPSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampsetupwidget.h"
#include "brainampaboutwidget.h"
#include "../brainamp.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMPSetupWidget::BrainAMPSetupWidget(BrainAMP* pBrainAMP, QWidget* parent)
: QWidget(parent)
, m_pBrainAMP(pBrainAMP)
{
    ui.setupUi(this);

    //Connect device sampling properties
    connect(ui.m_comboBox_SamplingFreq, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BrainAMPSetupWidget::setSamplingFreq);
    connect(ui.m_spinBox_BlockSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &BrainAMPSetupWidget::setSamplesPerBlock);
}

//=============================================================================================================

BrainAMPSetupWidget::~BrainAMPSetupWidget()
{
}

//=============================================================================================================

void BrainAMPSetupWidget::initGui()
{
    //Init device sampling properties
    ui.m_comboBox_SamplingFreq->setCurrentText(QString::number(m_pBrainAMP->m_iSamplingFreq));
    ui.m_spinBox_BlockSize->setValue(m_pBrainAMP->m_iSamplesPerBlock);
}

//=============================================================================================================

void BrainAMPSetupWidget::setSamplingFreq()
{
    m_pBrainAMP->m_iSamplingFreq = ui.m_comboBox_SamplingFreq->currentText().toInt();
}

//=============================================================================================================

void BrainAMPSetupWidget::setSamplesPerBlock()
{
    m_pBrainAMP->m_iSamplesPerBlock = ui.m_spinBox_BlockSize->value();
}

//=============================================================================================================

void BrainAMPSetupWidget::showAboutDialog()
{
    BrainAMPAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
