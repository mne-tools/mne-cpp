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

TMSISetupWidget::TMSISetupWidget(TMSI* simulator, QWidget* parent)
: QWidget(parent)
, m_pTMSI(simulator)
{
    ui.setupUi(this);

    connect(ui.m_qDoubleSpinBox_SamplingRate, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &TMSISetupWidget::setSamplingRate);
    connect(ui.m_qSpinBox_Downsampling, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &TMSISetupWidget::setDownsamplingRate);


    QString path(m_pTMSI->m_qStringResourcePath+"data/");

    QDir directory = QDir(path);
    QStringList files;
    QString fileName("*.txt");
    files = directory.entryList(QStringList(fileName),
                                QDir::Files | QDir::NoSymLinks);

    ui.m_qComboBox_Channel_1->addItems(files);
    ui.m_qComboBox_Channel_2->addItems(files);
    ui.m_qComboBox_Channel_3->addItems(files);

    connect(ui.m_qComboBox_Channel_1, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TMSISetupWidget::setFileOfChannel_I);
    connect(ui.m_qComboBox_Channel_2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TMSISetupWidget::setFileOfChannel_II);
    connect(ui.m_qComboBox_Channel_3, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &TMSISetupWidget::setFileOfChannel_III);

    connect(ui.m_qCheckBox_Channel_Enable_1, &QCheckBox::toggled, this, &TMSISetupWidget::setEnabledChannel_I);
    connect(ui.m_qCheckBox_Channel_Enable_2, &QCheckBox::toggled, this, &TMSISetupWidget::setEnabledChannel_II);
    connect(ui.m_qCheckBox_Channel_Enable_3, &QCheckBox::toggled, this, &TMSISetupWidget::setEnabledChannel_III);

    connect(ui.m_qCheckBox_Channel_Visible_1, &QCheckBox::toggled, this, &TMSISetupWidget::setVisibleChannel_I);
    connect(ui.m_qCheckBox_Channel_Visible_2, &QCheckBox::toggled, this, &TMSISetupWidget::setVisibleChannel_II);
    connect(ui.m_qCheckBox_Channel_Visible_3, &QCheckBox::toggled, this, &TMSISetupWidget::setVisibleChannel_III);

    connect(ui.m_qPushButton_About, &QPushButton::released, this, &TMSISetupWidget::showAboutDialog);


    QFile file(m_pTMSI->m_qStringResourcePath+"readme.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd()) {
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

void TMSISetupWidget::initSamplingFactors()
{
    ui.m_qDoubleSpinBox_SamplingRate->setValue(m_pTMSI->m_fSamplingRate);
    ui.m_qSpinBox_Downsampling->setValue(m_pTMSI->m_iDownsamplingFactor);
}


//*************************************************************************************************************

void TMSISetupWidget::initSelectedChannelFile()
{
    int idx;
    idx = ui.m_qComboBox_Channel_1->findText(m_pTMSI->m_pTMSIChannel_TMSI_I->getChannelFile());
    if(idx >= 0)
        ui.m_qComboBox_Channel_1->setCurrentIndex(idx);

    idx = ui.m_qComboBox_Channel_2->findText(m_pTMSI->m_pTMSIChannel_TMSI_II->getChannelFile());
    if(idx >= 0)
        ui.m_qComboBox_Channel_2->setCurrentIndex(idx);

    idx = ui.m_qComboBox_Channel_3->findText(m_pTMSI->m_pTMSIChannel_TMSI_III->getChannelFile());
    if(idx >= 0)
        ui.m_qComboBox_Channel_3->setCurrentIndex(idx);

}


//*************************************************************************************************************

void TMSISetupWidget::initChannelStates()
{
    setEnabledChannel_I(m_pTMSI->m_pTMSIChannel_TMSI_I->isEnabled());
    setVisibleChannel_I(m_pTMSI->m_pTMSIChannel_TMSI_I->isVisible());

    setEnabledChannel_II(m_pTMSI->m_pTMSIChannel_TMSI_II->isEnabled());
    setVisibleChannel_II(m_pTMSI->m_pTMSIChannel_TMSI_II->isVisible());

    setEnabledChannel_III(m_pTMSI->m_pTMSIChannel_TMSI_III->isEnabled());
    setVisibleChannel_III(m_pTMSI->m_pTMSIChannel_TMSI_III->isVisible());
}



//*************************************************************************************************************

void TMSISetupWidget::setSamplingRate(double value)
{
    m_pTMSI->m_fSamplingRate = value;
}


//*************************************************************************************************************

void TMSISetupWidget::setDownsamplingRate(int value)
{
    m_pTMSI->m_iDownsamplingFactor = value;
}

//*************************************************************************************************************

void TMSISetupWidget::setEnabledChannel_I(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_I->setEnabled(state);
    ui.m_qCheckBox_Channel_Enable_1->setChecked(state);
    ui.m_qLabel_Channel_1->setEnabled(state);
    ui.m_qCheckBox_Channel_Visible_1->setEnabled(state);
    ui.m_qComboBox_Channel_1->setEnabled(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setEnabledChannel_II(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_II->setEnabled(state);
    ui.m_qCheckBox_Channel_Enable_2->setChecked(state);
    ui.m_qLabel_Channel_2->setEnabled(state);
    ui.m_qCheckBox_Channel_Visible_2->setEnabled(state);
    ui.m_qComboBox_Channel_2->setEnabled(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setEnabledChannel_III(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_III->setEnabled(state);
    ui.m_qCheckBox_Channel_Enable_3->setChecked(state);
    ui.m_qLabel_Channel_3->setEnabled(state);
    ui.m_qCheckBox_Channel_Visible_3->setEnabled(state);
    ui.m_qComboBox_Channel_3->setEnabled(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setVisibleChannel_I(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_I->setVisible(state);
    ui.m_qCheckBox_Channel_Visible_1->setChecked(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setVisibleChannel_II(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_II->setVisible(state);
    ui.m_qCheckBox_Channel_Visible_2->setChecked(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setVisibleChannel_III(bool state)
{
    m_pTMSI->m_pTMSIChannel_TMSI_III->setVisible(state);
    ui.m_qCheckBox_Channel_Visible_3->setChecked(state);
}


//*************************************************************************************************************

void TMSISetupWidget::setFileOfChannel_I(qint32)
{
    m_pTMSI->m_pTMSIChannel_TMSI_I->setChannelFile(ui.m_qComboBox_Channel_1->currentText());
}


//*************************************************************************************************************

void TMSISetupWidget::setFileOfChannel_II(qint32)
{
    m_pTMSI->m_pTMSIChannel_TMSI_II->setChannelFile(ui.m_qComboBox_Channel_2->currentText());
}


//*************************************************************************************************************

void TMSISetupWidget::setFileOfChannel_III(qint32)
{
    m_pTMSI->m_pTMSIChannel_TMSI_III->setChannelFile(ui.m_qComboBox_Channel_3->currentText());
}


//*************************************************************************************************************

void TMSISetupWidget::showAboutDialog()
{
    TMSIAboutWidget aboutDialog(this);
    aboutDialog.exec();
}
