//=============================================================================================================
/**
* @file     tmsisetupwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSISetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsisetupwidget.h"
#include "tmsiaboutwidget.h"
#include "../tmsi.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSISetupWidget::TMSISetupWidget(TMSI* pTMSI, QWidget* parent)
: QWidget(parent)
, m_pTMSI(pTMSI)
{
    ui.setupUi(this);

    //Connect properties
    connect(ui.m_spinBox_SamplingFreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setSamplingFreq);
    connect(ui.m_spinBox_NumberOfChannels, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setNumberOfChannels);
    connect(ui.m_spinBox_SamplesPerBlock, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setSamplesPerBlock);

    //Connect start stop buttons
    connect(ui.m_pushButton_StartAcquisition, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &TMSISetupWidget::startAcquisition);
    connect(ui.m_pushButton_StopAcquisition, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &TMSISetupWidget::stopAcquisition);

    //Connect about button
    connect(ui.m_qPushButton_About, &QPushButton::released, this, &TMSISetupWidget::showAboutDialog);

    //Fill info box
    QFile file(m_pTMSI->m_qStringResourcePath+"readme.txt");
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

TMSISetupWidget::~TMSISetupWidget()
{

}


//*************************************************************************************************************

void TMSISetupWidget::initSamplingProperties()
{
    ui.m_spinBox_SamplingFreq->setValue(m_pTMSI->m_iSamplingFreq);
    ui.m_spinBox_NumberOfChannels->setValue(m_pTMSI->m_iNumberOfChannels);
    ui.m_spinBox_SamplesPerBlock->setValue(m_pTMSI->m_iSamplesPerBlock);
}


//*************************************************************************************************************

void TMSISetupWidget::setSamplingFreq(int value)
{
    m_pTMSI->m_iSamplingFreq = value;
}


//*************************************************************************************************************

void TMSISetupWidget::setNumberOfChannels(int value)
{
    m_pTMSI->m_iNumberOfChannels = value;
}


//*************************************************************************************************************

void TMSISetupWidget::setSamplesPerBlock(int value)
{
    m_pTMSI->m_iSamplesPerBlock = value;
}


//*************************************************************************************************************

void TMSISetupWidget::startAcquisition()
{
    m_pTMSI->start();
}


//*************************************************************************************************************

void TMSISetupWidget::stopAcquisition()
{
    m_pTMSI->stop();
}


//*************************************************************************************************************

void TMSISetupWidget::showAboutDialog()
{
    TMSIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
