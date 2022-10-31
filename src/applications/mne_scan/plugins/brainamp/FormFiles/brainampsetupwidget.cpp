//=============================================================================================================
/**
 * @file     brainampsetupwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
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
 * @brief    Contains the implementation of the BrainAMPSetupWidget class.
 *
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
