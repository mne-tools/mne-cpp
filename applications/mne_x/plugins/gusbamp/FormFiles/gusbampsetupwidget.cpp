//=============================================================================================================
/**
* @file     gusbampsetupwidget.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the GUSBAmpSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampsetupwidget.h"
#include "gusbampaboutwidget.h"
#include "../gusbamp.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAmpPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpSetupWidget::GUSBAmpSetupWidget(GUSBAmp* pGUSBAmp, QWidget* parent)
: QWidget(parent)
, m_pGUSBAmp(pGUSBAmp)
{
    ui.setupUi(this);

    //Connect device sampling properties
    connect(ui.m_spinBox_SamplingFreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_spinBox_NumberOfChannels, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_spinBox_SamplesPerBlock, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);

    //Connect channel corrections
    connect(ui.m_checkBox_UseChExponent, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_checkBox_UseUnitGain, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);
    connect(ui.m_checkBox_UseUnitOffset, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setDeviceSamplingProperties);

    //Connect debug file
    connect(ui.m_checkBox_WriteDriverDebugToFile, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setWriteToFile);

    //Connect trigger properties
    connect(ui.m_spinBox_BeepLength, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GUSBAmpSetupWidget::setTriggerProperties);
    connect(ui.m_checkBox_EnableBeep, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setTriggerProperties);

    //Connect about button
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &GUSBAmpSetupWidget::showAboutDialog);

    //Connect split file options
    connect(ui.m_checkBox_splitFiles, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &GUSBAmpSetupWidget::setSplitFile);
    connect(ui.m_spinBox_splitFileSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &GUSBAmpSetupWidget::setSplitFileSize);

    //Fill info box
    QFile file(m_pGUSBAmp->m_qStringResourcePath+"readme.txt");
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

GUSBAmpSetupWidget::~GUSBAmpSetupWidget()
{

}


//*************************************************************************************************************

void GUSBAmpSetupWidget::initGui()
{
    //Init device sampling properties
    ui.m_spinBox_SamplingFreq->setValue(m_pGUSBAmp->m_iSamplingFreq);
    ui.m_spinBox_NumberOfChannels->setValue(m_pGUSBAmp->m_iNumberOfChannels);
    ui.m_spinBox_SamplesPerBlock->setValue(m_pGUSBAmp->m_iSamplesPerBlock);

    //Init channel corrections
    ui.m_checkBox_UseChExponent->setChecked(m_pGUSBAmp->m_bUseChExponent);
    ui.m_checkBox_UseUnitGain->setChecked(m_pGUSBAmp->m_bUseUnitGain);
    ui.m_checkBox_UseUnitOffset->setChecked(m_pGUSBAmp->m_bUseUnitOffset);

    //Init write to file
    ui.m_checkBox_WriteDriverDebugToFile->setChecked(m_pGUSBAmp->m_bWriteDriverDebugToFile);

    //Init trigger properties
    ui.m_spinBox_BeepLength->setValue(m_pGUSBAmp->m_iTriggerInterval);
    ui.m_checkBox_EnableBeep->setChecked(m_pGUSBAmp->m_bBeepTrigger);
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::setDeviceSamplingProperties()
{
    cout<<"changing "<<endl;
    m_pGUSBAmp->m_iSamplingFreq = ui.m_spinBox_SamplingFreq->value();
    m_pGUSBAmp->m_iNumberOfChannels = ui.m_spinBox_NumberOfChannels->value();
    m_pGUSBAmp->m_iSamplesPerBlock = ui.m_spinBox_SamplesPerBlock->value();

    m_pGUSBAmp->m_bUseChExponent = ui.m_checkBox_UseChExponent->isChecked();
    m_pGUSBAmp->m_bUseUnitGain = ui.m_checkBox_UseUnitGain->isChecked();
    m_pGUSBAmp->m_bUseUnitOffset = ui.m_checkBox_UseUnitOffset->isChecked();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::setWriteToFile()
{
    m_pGUSBAmp->m_bWriteDriverDebugToFile = ui.m_checkBox_WriteDriverDebugToFile->isChecked();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::setSplitFile(bool state)
{
    m_pGUSBAmp->m_bSplitFile = state;
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::setSplitFileSize(qint32 value)
{
    m_pGUSBAmp->m_iSplitFileSizeMs = value;
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::setTriggerProperties()
{
    m_pGUSBAmp->m_iTriggerInterval = ui.m_spinBox_BeepLength->value();
    m_pGUSBAmp->m_bBeepTrigger = ui.m_checkBox_EnableBeep->isChecked();
}


//*************************************************************************************************************

void GUSBAmpSetupWidget::showAboutDialog()
{
    GUSBAmpAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
