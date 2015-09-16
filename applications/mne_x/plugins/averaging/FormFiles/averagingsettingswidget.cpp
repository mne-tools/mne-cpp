//=============================================================================================================
/**
* @file     averagingsettingswidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the AveragingSettingsWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsettingswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AveragingPlugin;


AveragingSettingsWidget::AveragingSettingsWidget(Averaging *toolbox, QWidget *parent)
: QWidget(parent)
, m_pAveragingToolbox(toolbox)
{
    ui.setupUi(this);

    this->setWindowTitle("Averaging Settings");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);

    if(m_pAveragingToolbox->m_pFiffInfo && m_pAveragingToolbox->m_qListStimChs.size() > 0) {
        for(qint32 i = 0; i < m_pAveragingToolbox->m_qListStimChs.size(); ++i) {
            if(m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ] != QString("STI 014")) {
                //qDebug() << "Insert" << i << m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ];
                ui.m_pComboBoxChSelection->insertItem(i,m_pAveragingToolbox->m_pFiffInfo->ch_names[ m_pAveragingToolbox->m_qListStimChs[i] ],QVariant(i));
            }
        }

        ui.m_pComboBoxChSelection->setCurrentIndex(m_pAveragingToolbox->m_iStimChan);

        connect(ui.m_pComboBoxChSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                m_pAveragingToolbox, &Averaging::changeStimChannel);
    }

    if(m_pAveragingToolbox->m_pRtAve) {
        ui.m_pSpinBoxNumAverages->setValue(m_pAveragingToolbox->m_iNumAverages);
        connect(ui.m_pSpinBoxNumAverages, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                m_pAveragingToolbox, &Averaging::changeNumAverages);
    }

    //Pre Post stimulus
    ui.m_pSpinBoxPreStimSamples->setValue(m_pAveragingToolbox->m_iPreStimSeconds);
    connect(ui.m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &AveragingSettingsWidget::changePreStim);

    ui.m_pSpinBoxPostStimSamples->setValue(m_pAveragingToolbox->m_iPostStimSeconds);
    connect(ui.m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &AveragingSettingsWidget::changePostStim);

    //Baseline Correction
    connect(ui.m_pcheckBoxBaselineCorrection, &QCheckBox::clicked,
            m_pAveragingToolbox, &Averaging::changeBaselineActive);

    ui.m_pSpinBoxBaselineFrom->setMinimum(ui.m_pSpinBoxPreStimSamples->value()*-1);
    ui.m_pSpinBoxBaselineFrom->setMaximum(ui.m_pSpinBoxPostStimSamples->value());
    ui.m_pSpinBoxBaselineFrom->setValue(ui.m_pSpinBoxPreStimSamples->value());
    connect(ui.m_pSpinBoxBaselineFrom, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &AveragingSettingsWidget::changeBaselineFrom);

    ui.m_pSpinBoxBaselineTo->setMinimum(ui.m_pSpinBoxPreStimSamples->value()*-1);
    ui.m_pSpinBoxBaselineTo->setMaximum(ui.m_pSpinBoxPostStimSamples->value());
    ui.m_pSpinBoxBaselineTo->setValue(ui.m_pSpinBoxPreStimSamples->value());
    connect(ui.m_pSpinBoxBaselineTo, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &AveragingSettingsWidget::changeBaselineTo);
}


//*************************************************************************************************************

int AveragingSettingsWidget::getStimChannelIdx()
{
    return ui.m_pComboBoxChSelection->currentData().toInt();
}


//*************************************************************************************************************

void AveragingSettingsWidget::changePreStim(qint32 mSeconds)
{
    ui.m_pSpinBoxBaselineTo->setMinimum(ui.m_pSpinBoxPreStimSamples->value()*-1);
    ui.m_pSpinBoxBaselineFrom->setMinimum(ui.m_pSpinBoxPreStimSamples->value()*-1);

    m_pAveragingToolbox->changePreStim(mSeconds);
}


//*************************************************************************************************************

void AveragingSettingsWidget::changePostStim(qint32 mSeconds)
{
    ui.m_pSpinBoxBaselineTo->setMaximum(ui.m_pSpinBoxPostStimSamples->value());
    ui.m_pSpinBoxBaselineFrom->setMaximum(ui.m_pSpinBoxPostStimSamples->value());

    m_pAveragingToolbox->changePostStim(mSeconds);
}

//*************************************************************************************************************

void AveragingSettingsWidget::changeBaselineFrom(qint32 mSeconds)
{
    ui.m_pSpinBoxBaselineTo->setMinimum(mSeconds);

    m_pAveragingToolbox->changeBaselineFrom(mSeconds/*+m_pSpinBoxPreStimSamples->value()*/);
}


//*************************************************************************************************************

void AveragingSettingsWidget::changeBaselineTo(qint32 mSeconds)
{
    ui.m_pSpinBoxBaselineFrom->setMaximum(mSeconds);

    m_pAveragingToolbox->changeBaselineTo(mSeconds/*+m_pSpinBoxPreStimSamples->value()*/);
}

