//=============================================================================================================
/**
* @file     triggercontrolsetupwidget.cpp
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>
*           Luise Lang <luise.lang@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze, Luise Lang and Christoph Dinh. All rights reserved.
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
* @brief    Definition of the TriggerControlSetupWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggercontrolsetupwidget.h"
#include "triggercontrolaboutwidget.h"
#include "settingswidget.h"

#include "serialport.h"

#include "../triggercontrol.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TRIGGERCONTROLPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerControlSetupWidget::TriggerControlSetupWidget(TriggerControl* toolbox, QWidget *parent)
: QWidget(parent)
, m_pTriggerControl(toolbox)
{
    ui.setupUi(this);
    connect(ui.m_qPushButton_Settings, &QPushButton::released, this, &TriggerControlSetupWidget::showSettings);

    connect(ui.m_qPushButton_About, &QPushButton::released, this, &TriggerControlSetupWidget::showAboutDialog);
    // initialize Buttons
    ui.m_qPushButton_Connect->setEnabled(true);
    ui.m_qPushButton_Disconnect->setEnabled(false);
    ui.m_qPushButton_Send->setEnabled(false);
    ui.m_qPushButton_Sendanalog->setEnabled(false);

    ui.m_qComboBox_AnalogSelect->addItem(QLatin1String("Kanal 1"));
    ui.m_qComboBox_AnalogSelect->addItem(QLatin1String("Kanal 2"));

    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 1"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 2"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 3"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 4"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 5"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 6"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 7"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 8"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 9"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 10"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 11"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 12"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 13"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 14"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 15"));
    ui.m_qComboBox_ChannelList->addItem(QLatin1String("Kanal 16"));

}

//*************************************************************************************************************

TriggerControlSetupWidget::~TriggerControlSetupWidget()
{

}


//*************************************************************************************************************

void TriggerControlSetupWidget::showAboutDialog()
{
    TriggerControlAboutWidget aboutDialog(this);
    aboutDialog.exec();
}


//*************************************************************************************************************

void TriggerControlSetupWidget::showSettings()
{
    SettingsWidget settingsWidget(this);
    settingsWidget.exec();
}


//*************************************************************************************************************

void TriggerControlSetupWidget::on_m_qPushButton_Connect_released()
{
    if (m_pTriggerControl->m_pSerialPort->open())
    {
        std::cout << "Port opened" << std::endl;
        ui.m_qPushButton_Connect->setEnabled(false);
        ui.m_qPushButton_Disconnect->setEnabled(true);
        ui.m_qPushButton_Send->setEnabled(true);
        ui.m_qPushButton_Sendanalog->setEnabled(true);
        ui.m_qPushButton_Settings->setEnabled(false);
    }
    else
    {
        std::cout << "Port cannot be opened" << std::endl;
    }
}


//*************************************************************************************************************

void TriggerControlSetupWidget::on_m_qPushButton_Disconnect_released()
{

    m_pTriggerControl->m_pSerialPort->close();
    std::cout << "Port closed" << std::endl;
    ui.m_qPushButton_Connect->setEnabled(true);
    ui.m_qPushButton_Disconnect->setEnabled(false);
    ui.m_qPushButton_Send->setEnabled(false);
    ui.m_qPushButton_Sendanalog->setEnabled(false);
    ui.m_qPushButton_Settings->setEnabled(true);


}


//*************************************************************************************************************

void TRIGGERCONTROLPLUGIN::TriggerControlSetupWidget::on_m_qPushButton_Send_released()
{

    // retrieve current configuration of digital channels
    if (ui.m_qRadioButton_1->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(0,1);     // 0000 0100
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(0,0);
    if (ui.m_qRadioButton_2->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(1,1);     // 0000 1000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(1,0);
    if (ui.m_qRadioButton_3->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(2,1);     // 0001 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(2,0);
    if (ui.m_qRadioButton_4->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(3,1);     // 0010 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(3,0);
    if (ui.m_qRadioButton_5->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(4,1);     // 0100 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(4,0);
    if (ui.m_qRadioButton_6->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(5,1);     // 1000 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(5,0);

    // 7 - 12
    if (ui.m_qRadioButton_7->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(6,1);     // 0000 0100
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(6,0);
    if (ui.m_qRadioButton_8->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(7,1);     // 0000 1000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(7,0);
    if (ui.m_qRadioButton_9->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(8,1);     // 0001 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(8,0);
    if (ui.m_qRadioButton_10->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(9,1);     // 0010 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(9,0);
    if (ui.m_qRadioButton_11->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(10,1);     // 0100 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(10,0);
    if (ui.m_qRadioButton_12->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(11,1);     // 1000 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(11,0);

    // 13 - 18
    if (ui.m_qRadioButton_13->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(12,1);     // 0000 0100
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(12,0);
    if (ui.m_qRadioButton_14->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(13,1);     // 0000 1000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(13,0);
    if (ui.m_qRadioButton_15->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(14,1);     // 0001 0000
        else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(14,0);
    if (ui.m_qRadioButton_16->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(15,1);     // 0010 0000
        else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(15,0);

   // encode information according to data transfer protocol
    m_pTriggerControl->m_pSerialPort->encodedig();

    // send data
    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);

    std::cout << "Digital data sent" << std::endl;
}


//*************************************************************************************************************

void TRIGGERCONTROLPLUGIN::TriggerControlSetupWidget::on_m_qPushButton_Sendanalog_released()
{

    // retrieve current configuration
    // retrieve motor selection
    if(ui.m_qRadioButton_motor1->isChecked()) m_pTriggerControl->m_pSerialPort->m_motor = 1;
    if(ui.m_qRadioButton_motor2->isChecked()) m_pTriggerControl->m_pSerialPort->m_motor = 2;
    if(ui.m_qRadioButton_motor3->isChecked()) m_pTriggerControl->m_pSerialPort->m_motor = 3;
    if(ui.m_qRadioButton_motor4->isChecked()) m_pTriggerControl->m_pSerialPort->m_motor = 4;

    // retrieve analog value
    m_pTriggerControl->m_pSerialPort->m_analval = ui.m_qAnalogDisp->intValue();

    // encode information according to data transfer protocol
    m_pTriggerControl->m_pSerialPort->encodeana();

    // send data
    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);

    std::cout << "Analog data sent" << std::endl;

}

void TRIGGERCONTROLPLUGIN::TriggerControlSetupWidget::on_m_qPushButton_RetrieveDigitalInfo_released()
{
    m_pTriggerControl->m_pSerialPort->m_retrievetyp = 0;    // digital channel desired
    m_pTriggerControl->m_pSerialPort->encoderetr();
    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);
}


//*************************************************************************************************************

void TRIGGERCONTROLPLUGIN::TriggerControlSetupWidget::on_m_qPushButton_RetrieveAnalogInfo_released()
{
    m_pTriggerControl->m_pSerialPort->m_retrievetyp = 1;    // analog channel desired

    if (ui.m_qComboBox_AnalogSelect->currentIndex() == 0)
        m_pTriggerControl->m_pSerialPort->m_retrievechan = 1;
    else
        std::cout << "check desired channel" << std::endl;

    if (ui.m_qComboBox_AnalogSelect->currentIndex() == 1)
        m_pTriggerControl->m_pSerialPort->m_retrievechan = 2;
    else
        std::cout << "check desired channel" << std::endl;

    m_pTriggerControl->m_pSerialPort->encoderetr();

    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);
}


//*************************************************************************************************************

void TRIGGERCONTROLPLUGIN::TriggerControlSetupWidget::on_m_qPushButton_ConnectChannel_released()
{
    m_pTriggerControl->m_pSerialPort->m_wiredChannel = ui.m_qComboBox_ChannelList->currentIndex();
}
