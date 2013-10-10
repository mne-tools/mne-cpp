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
    m_bAcquisitionIsRunning = false;

    ui.setupUi(this);

    //Connect properties
    connect(ui.m_spinBox_SamplingFreq, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setSamplingFreq);
    connect(ui.m_spinBox_NumberOfChannels, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setNumberOfChannels);
    connect(ui.m_spinBox_SamplesPerBlock, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setSamplesPerBlock);

    //Connect channel corrections
    connect(ui.m_checkBox_ConvertToVolt, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setChannelCorrections);
    connect(ui.m_checkBox_UseChExponent, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setChannelCorrections);
    connect(ui.m_checkBox_UseUnitGain, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setChannelCorrections);
    connect(ui.m_checkBox_UseUnitOffset, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setChannelCorrections);

    //Connect write to file
    connect(ui.m_checkBox_WriteToFile, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &TMSISetupWidget::setWriteToFile);
    connect(ui.m_pushButton_ChangeDir, &QPushButton::released, this, &TMSISetupWidget::changeOutputFileDir);

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
    //Init device sampling properties
    ui.m_spinBox_SamplingFreq->setValue(m_pTMSI->m_iSamplingFreq);
    ui.m_spinBox_NumberOfChannels->setValue(m_pTMSI->m_iNumberOfChannels);
    ui.m_spinBox_SamplesPerBlock->setValue(m_pTMSI->m_iSamplesPerBlock);

    //Init channel corrections
    ui.m_checkBox_ConvertToVolt->setChecked(m_pTMSI->m_bConvertToVolt);
    ui.m_checkBox_UseChExponent->setChecked(m_pTMSI->m_bUseChExponent);
    ui.m_checkBox_UseUnitGain->setChecked(m_pTMSI->m_bUseUnitGain);
    ui.m_checkBox_UseUnitOffset->setChecked(m_pTMSI->m_bUseUnitOffset);

    //Init write to file
    ui.m_checkBox_WriteToFile->setChecked(m_pTMSI->m_bWriteToFile);
    ui.m_lineEdit_outputDir->setText(m_pTMSI->m_sOutputFilePath);
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

void TMSISetupWidget::setChannelCorrections()
{
    m_pTMSI->m_bConvertToVolt = ui.m_checkBox_ConvertToVolt->isChecked();
    m_pTMSI->m_bUseChExponent = ui.m_checkBox_UseChExponent->isChecked();
    m_pTMSI->m_bUseUnitGain = ui.m_checkBox_UseUnitGain->isChecked();
    m_pTMSI->m_bUseUnitOffset = ui.m_checkBox_UseUnitOffset->isChecked();
}


//*************************************************************************************************************

void TMSISetupWidget::setWriteToFile()
{
    m_pTMSI->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
    m_pTMSI->m_bWriteToFile = ui.m_checkBox_WriteToFile->isChecked();
}


//*************************************************************************************************************

void TMSISetupWidget::changeOutputFileDir()
{
    QString path = QFileDialog::getExistingDirectory(
                this,
                "Change output directory",
                "mne_x_plugins/resources/tmsi",
                 QFileDialog::ShowDirsOnly);

    if(path==NULL)
        path = ui.m_lineEdit_outputDir->text();

    ui.m_lineEdit_outputDir->setText(path);
    m_pTMSI->m_sOutputFilePath = ui.m_lineEdit_outputDir->text();
}


//*************************************************************************************************************

void TMSISetupWidget::showAboutDialog()
{
    TMSIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
