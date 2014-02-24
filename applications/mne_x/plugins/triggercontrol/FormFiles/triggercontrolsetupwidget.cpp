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
* @brief    Contains the implementation of the TriggerControlSetupWidget class.
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

using namespace TriggerControlPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerControlSetupWidget::TriggerControlSetupWidget(TriggerControl* toolbox, QWidget *parent)
: QWidget(parent)
, m_pTriggerControl(toolbox)
{
    ui.setupUi(this);

//    connect(ui.m_qPushButton_About, &QPushButton::released, this, &TriggerControlSetupWidget::showAboutDialog);



    connect(ui.m_qPushButton_Settings, &QPushButton::released, this, &TriggerControlSetupWidget::showSettings);



    // initialize Buttons
    ui.m_qPushButton_Connect->setEnabled(true);
    ui.m_qPushButton_Disconnect->setEnabled(false);
    ui.m_qPushButton_Send->setEnabled(false);
    ui.m_qPushButton_Sendanalog->setEnabled(false);

    ui.m_qComboBox_RetrieveType->addItem(QLatin1String("Digital"));
    ui.m_qComboBox_RetrieveType->addItem(QLatin1String("Analog"));


//    connect(ui.m_qPushButton_Connect, SIGNAL(released()), this, SLOT( m_port_tsw->openSerialPort(TriggerControl::Settings m_pTriggerControl->m_currentSettings)));
//    connect(ui.m_qPushButton_Disconnect, SIGNAL(released()), this, SLOT(closeSerialPort()));
//    connect(ui->pushButton_send,SIGNAL(released()),this, SLOT(sendData()));




    //Bsp Parameter

//    m_pTriggerControl->m_bBspBool = true;
}

//*************************************************************************************************************

TriggerControlSetupWidget::~TriggerControlSetupWidget()
{

}


//*************************************************************************************************************

void TriggerControlSetupWidget::showAboutDialog()
{
//    TriggerControlAboutWidget aboutDialog(this);
//    aboutDialog.exec();
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
        std::cout << "Port geöffnet" << std::endl;
        ui.m_qPushButton_Connect->setEnabled(false);
        ui.m_qPushButton_Disconnect->setEnabled(true);
        ui.m_qPushButton_Send->setEnabled(true);
        ui.m_qPushButton_Sendanalog->setEnabled(true);
        ui.m_qPushButton_Settings->setEnabled(false);
    }
    else
    {
        std::cout << "Port konnte nicht geöffnet werden" << std::endl;
    }
}


//*************************************************************************************************************

void TriggerControlSetupWidget::on_m_qPushButton_Disconnect_released()
{

    m_pTriggerControl->m_pSerialPort->close();
    std::cout << "Port geschlossen" << std::endl;
    ui.m_qPushButton_Connect->setEnabled(true);
    ui.m_qPushButton_Disconnect->setEnabled(false);
    ui.m_qPushButton_Send->setEnabled(false);
    ui.m_qPushButton_Sendanalog->setEnabled(false);
    ui.m_qPushButton_Settings->setEnabled(true);


}


//*************************************************************************************************************

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_Send_released()
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
    if (ui.m_qRadioButton_17->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(16,1);     // 0100 0000
        else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(16,0);
    if (ui.m_qRadioButton_18->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(17,1);     // 1000 0000
        else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(17,0);

    // 19 - 22
    if (ui.m_qRadioButton_19->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(18,1);     // 0000 0100
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(18,0);
    if (ui.m_qRadioButton_20->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(19,1);     // 0000 1000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(19,0);
    if (ui.m_qRadioButton_21->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(20,1);     // 0001 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(20,0);
    if (ui.m_qRadioButton_22->isChecked()) m_pTriggerControl->m_pSerialPort->m_digchannel.replace(21,1);     // 0010 0000
    else m_pTriggerControl->m_pSerialPort->m_digchannel.replace(21,0);


   // encode information according to data transfer protocol
    m_pTriggerControl->m_pSerialPort->encodedig();


    // send data
    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);

    std::cout << "Digitale Daten gesendet" << std::endl;
}


//*************************************************************************************************************

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_Sendanalog_released()
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

    std::cout << "Analoge Daten gesendet" << std::endl;

  //  m_pTriggerControl->m_pSerialPort->readData();

}

//*************************************************************************************************************

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_RetrieveInfo_released()
{
    if ( ui.m_qComboBox_RetrieveType->currentIndex() == 0)      // digital selected
    {
        m_pTriggerControl->m_pSerialPort->m_retrievetyp = 0;    // digital channel desired

    }

    else if (ui.m_qComboBox_RetrieveType->currentIndex() == 1)
    {
        m_pTriggerControl->m_pSerialPort->m_retrievetyp = 1;    // analog channel desired
        int t_channel = ui.m_qLineEdit_RetrieveChannel->text().toInt();
        if ((t_channel <= 2) && (t_channel > 0))
                m_pTriggerControl->m_pSerialPort->m_retrievechan = t_channel;   // xth channel desired
        else
            std::cout << "check desired channel" << std::endl;
    }
    m_pTriggerControl->m_pSerialPort->encoderetr();

    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);

}
