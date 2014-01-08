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
    QByteArray t_data;
    t_data.resize(4);
    t_data.clear();

    //denote control bytes
    t_data[0] = t_data[0]|0x40;
    t_data[1] = t_data[1]|0x01;
    t_data[2] = t_data[2]|0x02;
    t_data[3] = t_data[3]|0x03;
    // 1 - 6
    if (ui.m_qRadioButton_1->isChecked()) t_data[3] = t_data[3]|0x04;     // 0000 0100
    if (ui.m_qRadioButton_2->isChecked()) t_data[3] = t_data[3]|0x08;     // 0000 1000
    if (ui.m_qRadioButton_3->isChecked()) t_data[3] = t_data[3]|0x10;     // 0001 0000
    if (ui.m_qRadioButton_4->isChecked()) t_data[3] = t_data[3]|0x20;     // 0010 0000
    if (ui.m_qRadioButton_5->isChecked()) t_data[3] = t_data[3]|0x40;     // 0100 0000
    if (ui.m_qRadioButton_6->isChecked()) t_data[3] = t_data[3]|0x80;     // 1000 0000

    // 7 - 12
    if (ui.m_qRadioButton_7->isChecked()) t_data[2] = t_data[2]|0x04;     // 0000 0100
    if (ui.m_qRadioButton_8->isChecked()) t_data[2] = t_data[2]|0x08;     // 0000 1000
    if (ui.m_qRadioButton_9->isChecked()) t_data[2] = t_data[2]|0x10;     // 0001 0000
    if (ui.m_qRadioButton_10->isChecked()) t_data[2] = t_data[2]|0x20;     // 0010 0000
    if (ui.m_qRadioButton_11->isChecked()) t_data[2] = t_data[2]|0x40;     // 0100 0000
    if (ui.m_qRadioButton_12->isChecked()) t_data[2] = t_data[2]|0x80;     // 1000 0000

    // 13 - 18
    if (ui.m_qRadioButton_13->isChecked()) t_data[1] = t_data[1]|0x04;     // 0000 0100
    if (ui.m_qRadioButton_14->isChecked()) t_data[1] = t_data[1]|0x08;     // 0000 1000
    if (ui.m_qRadioButton_15->isChecked()) t_data[1] = t_data[1]|0x10;     // 0001 0000
    if (ui.m_qRadioButton_16->isChecked()) t_data[1] = t_data[1]|0x20;     // 0010 0000
    if (ui.m_qRadioButton_17->isChecked()) t_data[1] = t_data[1]|0x40;     // 0100 0000
    if (ui.m_qRadioButton_18->isChecked()) t_data[1] = t_data[1]|0x80;     // 1000 0000

    // 19 - 22
    if (ui.m_qRadioButton_19->isChecked()) t_data[0] = t_data[0]|0x04;     // 0000 0100
    if (ui.m_qRadioButton_20->isChecked()) t_data[0] = t_data[0]|0x08;     // 0000 1000
    if (ui.m_qRadioButton_21->isChecked()) t_data[0] = t_data[0]|0x10;     // 0001 0000
    if (ui.m_qRadioButton_22->isChecked()) t_data[0] = t_data[0]|0x20;     // 0010 0000

  //  m_pTriggerControl->m_pSerialPort->encodedig();

    m_pTriggerControl->m_pSerialPort->sendData(t_data);
    std::cout << "Digitale Daten gesendet" << std::endl;
}

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_nullen_released()
{
    QByteArray t_data;
    //t_data = m_pTriggerControl->m_pSerialPort->m_data;

    t_data.resize(3);
    int value = 0;
    t_data.append(value);

    std::cout << t_data.size() << std::endl;

    m_pTriggerControl->m_pSerialPort->sendData(t_data);
}

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_Sendanalog_released()
{
    // create Data byte
    int t_motor;
    if(ui.m_qRadioButton_motor1->isChecked()) t_motor = 1;
    if(ui.m_qRadioButton_motor2->isChecked()) t_motor = 2;
    if(ui.m_qRadioButton_motor3->isChecked()) t_motor = 3;
    if(ui.m_qRadioButton_motor4->isChecked()) t_motor = 4;

    int t_analval = ui.m_qAnalogDisp->intValue();

    m_pTriggerControl->m_pSerialPort->encodeana(t_analval,t_motor);

    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);

    std::cout << "Analoge Daten gesendet" << std::endl;
}
