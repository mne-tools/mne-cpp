//=============================================================================================================
/**
 * @file     tmsisetupwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September 2013
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
 * @brief    Contains the implementation of the TMSISetupWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsisetupwidget.h"
#include "../tmsi.h"
#include "ui_tmsisetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSISetupWidget::TMSISetupWidget(TMSI* pTMSI, QWidget* parent)
: QWidget(parent)
, m_pTMSI(pTMSI)
, m_pUi(new Ui::TMSISetupClass)
{
    m_pUi->setupUi(this);

    //Connect device sampling properties
    connect(m_pUi->m_spinBox_SamplingFreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_spinBox_NumberOfChannels, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_spinBox_SamplesPerBlock, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_checkBox_UseCommonAverage, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setDeviceSamplingProperties);

    //Connect channel corrections
    connect(m_pUi->m_checkBox_UseChExponent, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_checkBox_UseUnitGain, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setDeviceSamplingProperties);
    connect(m_pUi->m_checkBox_UseUnitOffset, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setDeviceSamplingProperties);

    //Connect debug file
    connect(m_pUi->m_checkBox_WriteDriverDebugToFile, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setWriteToDebugFile);

    //Connect trigger properties
    connect(m_pUi->m_spinBox_BeepLength, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setTriggerProperties);
    connect(m_pUi->m_checkBox_EnableBeep, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setTriggerProperties);
    connect(m_pUi->m_checkBox_EnableKeyboardTrigger, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setTriggerProperties);
}

//=============================================================================================================

TMSISetupWidget::~TMSISetupWidget()
{
}

//=============================================================================================================

void TMSISetupWidget::initGui()
{
    //Init device sampling properties
    m_pUi->m_spinBox_SamplingFreq->setValue(m_pTMSI->m_iSamplingFreq);
    m_pUi->m_spinBox_NumberOfChannels->setValue(m_pTMSI->m_iNumberOfChannels);
    m_pUi->m_spinBox_SamplesPerBlock->setValue(m_pTMSI->m_iSamplesPerBlock);
    m_pUi->m_checkBox_UseCommonAverage->setChecked(m_pTMSI->m_bUseCommonAverage);

    //Init channel corrections
    m_pUi->m_checkBox_UseChExponent->setChecked(m_pTMSI->m_bUseChExponent);
    m_pUi->m_checkBox_UseUnitGain->setChecked(m_pTMSI->m_bUseUnitGain);
    m_pUi->m_checkBox_UseUnitOffset->setChecked(m_pTMSI->m_bUseUnitOffset);

    //Init write to file
    m_pUi->m_checkBox_WriteDriverDebugToFile->setChecked(m_pTMSI->m_bWriteDriverDebugToFile);

    //Init trigger properties
    m_pUi->m_spinBox_BeepLength->setValue(m_pTMSI->m_iTriggerInterval);
    m_pUi->m_checkBox_EnableBeep->setChecked(m_pTMSI->m_bBeepTrigger);

    m_pUi->m_checkBox_EnableKeyboardTrigger->setChecked(m_pTMSI->m_bUseKeyboardTrigger);
}

//=============================================================================================================

void TMSISetupWidget::setDeviceSamplingProperties()
{
    m_pTMSI->m_iSamplingFreq = m_pUi->m_spinBox_SamplingFreq->value();
    m_pTMSI->m_iNumberOfChannels = m_pUi->m_spinBox_NumberOfChannels->value();
    m_pTMSI->m_iSamplesPerBlock = m_pUi->m_spinBox_SamplesPerBlock->value();

    m_pTMSI->m_bUseChExponent = m_pUi->m_checkBox_UseChExponent->isChecked();
    m_pTMSI->m_bUseUnitGain = m_pUi->m_checkBox_UseUnitGain->isChecked();
    m_pTMSI->m_bUseUnitOffset = m_pUi->m_checkBox_UseUnitOffset->isChecked();

    m_pTMSI->m_bUseCommonAverage = m_pUi->m_checkBox_UseCommonAverage->isChecked();
}

//=============================================================================================================

void TMSISetupWidget::setWriteToDebugFile()
{
    m_pTMSI->m_bWriteDriverDebugToFile = m_pUi->m_checkBox_WriteDriverDebugToFile->isChecked();
}

//=============================================================================================================

void TMSISetupWidget::setTriggerProperties()
{
    m_pTMSI->m_iTriggerInterval = m_pUi->m_spinBox_BeepLength->value();
    m_pTMSI->m_bBeepTrigger = m_pUi->m_checkBox_EnableBeep->isChecked();
    m_pTMSI->m_bUseKeyboardTrigger = m_pUi->m_checkBox_EnableKeyboardTrigger->isChecked();
}

//=============================================================================================================
