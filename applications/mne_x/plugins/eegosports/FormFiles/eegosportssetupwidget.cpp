//=============================================================================================================
/**
* @file     eegosportssetupwidget.cpp
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
* @brief    Contains the implementation of the EEGoSportsSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportssetupwidget.h"
#include "eegosportsaboutwidget.h"
#include "../eegosports.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGoSportsPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsSetupWidget::EEGoSportsSetupWidget(EEGoSports* pEEGoSports, QWidget* parent)
: QWidget(parent)
, m_pEEGoSports(pEEGoSports)
{
    ui.setupUi(this);

    //Connect device sampling properties
    connect(ui.m_spinBox_SamplingFreq, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_spinBox_NumberOfChannels, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_spinBox_SamplesPerBlock, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);

    //Connect channel corrections
    connect(ui.m_checkBox_UseChExponent, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setDeviceSamplingProperties);

    //Connect preprocessing
    connect(ui.m_checkBox_UseFiltering, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setPreprocessing);

    //Connect debug file
    connect(ui.m_checkBox_WriteDriverDebugToFile, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setWriteToFile);

    //Connect trigger properties
    connect(ui.m_spinBox_BeepLength, static_cast<void (QSpinBox::*)()>(&QSpinBox::editingFinished),
            this, &EEGoSportsSetupWidget::setTriggerProperties);
    connect(ui.m_checkBox_EnableBeep, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setTriggerProperties);
    connect(ui.m_checkBox_EnableKeyboardTrigger, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &EEGoSportsSetupWidget::setTriggerProperties);

    //Connect about button
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &EEGoSportsSetupWidget::showAboutDialog);

    //Fill info box
    QFile file(m_pEEGoSports->m_qStringResourcePath+"readme.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        ui.m_qTextBrowser_Information->insertHtml(line);
        ui.m_qTextBrowser_Information->insertHtml("<br>");
    }
}


//*************************************************************************************************************

EEGoSportsSetupWidget::~EEGoSportsSetupWidget()
{

}


//*************************************************************************************************************

void EEGoSportsSetupWidget::initGui()
{
    //Init device sampling properties
    ui.m_spinBox_SamplingFreq->setValue(m_pEEGoSports->m_iSamplingFreq);
    ui.m_spinBox_NumberOfChannels->setValue(m_pEEGoSports->m_iNumberOfChannels);
    ui.m_spinBox_SamplesPerBlock->setValue(m_pEEGoSports->m_iSamplesPerBlock);

    //Init channel corrections
    ui.m_checkBox_UseChExponent->setChecked(m_pEEGoSports->m_bUseChExponent);

    //Init preprocessing
    ui.m_checkBox_UseFiltering->setChecked(m_pEEGoSports->m_bUseFiltering);

    //Init write to file
    ui.m_checkBox_WriteDriverDebugToFile->setChecked(m_pEEGoSports->m_bWriteDriverDebugToFile);

    //Init trigger properties
    ui.m_spinBox_BeepLength->setValue(m_pEEGoSports->m_iTriggerInterval);
    ui.m_checkBox_EnableBeep->setChecked(m_pEEGoSports->m_bBeepTrigger);
}


//*************************************************************************************************************

void EEGoSportsSetupWidget::setDeviceSamplingProperties()
{
    m_pEEGoSports->m_iSamplingFreq = ui.m_spinBox_SamplingFreq->value();
    m_pEEGoSports->m_iNumberOfChannels = ui.m_spinBox_NumberOfChannels->value();
    m_pEEGoSports->m_iSamplesPerBlock = ui.m_spinBox_SamplesPerBlock->value();

    m_pEEGoSports->m_bUseChExponent = ui.m_checkBox_UseChExponent->isChecked();
}


//*************************************************************************************************************

void EEGoSportsSetupWidget::setPreprocessing()
{
    m_pEEGoSports->m_bUseFiltering = ui.m_checkBox_UseFiltering->isChecked();
}


//*************************************************************************************************************

void EEGoSportsSetupWidget::setPostprocessing()
{
}


//*************************************************************************************************************

void EEGoSportsSetupWidget::setWriteToFile()
{
    m_pEEGoSports->m_bWriteDriverDebugToFile = ui.m_checkBox_WriteDriverDebugToFile->isChecked();
}

//*************************************************************************************************************

void EEGoSportsSetupWidget::setTriggerProperties()
{
    m_pEEGoSports->m_iTriggerInterval = ui.m_spinBox_BeepLength->value();
    m_pEEGoSports->m_bBeepTrigger = ui.m_checkBox_EnableBeep->isChecked();
}


//*************************************************************************************************************

void EEGoSportsSetupWidget::showAboutDialog()
{
    EEGoSportsAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
