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
    ui.m_qPushButton_Settings->setEnabled(true);


}


//*************************************************************************************************************

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_Send_released()
{
    // create Data byte

/*

    QByteArray data;
//    int length = data.size();


    // Auswerten der LED Steuerung

    unsigned char steuerwert = 0;
    if (ui.m_qRadioButton_8->isChecked()) // Auswertung HSB
    {
        steuerwert = steuerwert + 128;
    }
    if (ui.m_qRadioButton_7->isChecked())
    {
        steuerwert = steuerwert + 64;
    }
    if (ui.m_qRadioButton_6->isChecked())
    {
        steuerwert = steuerwert + 32;
    }
    if (ui.m_qRadioButton_5->isChecked())
    {
        steuerwert = steuerwert + 16;
    }
    if (ui.m_qRadioButton_4->isChecked())
    {
        steuerwert = steuerwert + 8;
    }
    if ( ui.m_qRadioButton_3->isChecked())
    {
        steuerwert = steuerwert + 4;
    }
    if (ui.m_qRadioButton_2->isChecked())
    {
        steuerwert = steuerwert + 2;
    }
    if ( ui.m_qRadioButton_1->isChecked())    // Auswertung LSB
    {
        steuerwert = steuerwert + 1;
    }

     std::cout << "LEDS ausgelesen" << steuerwert << std::endl;
    data[0] = steuerwert;*/

    m_pTriggerControl->m_pSerialPort->encodeana();

    m_pTriggerControl->m_pSerialPort->sendData(m_pTriggerControl->m_pSerialPort->m_data);
}

void TriggerControlPlugin::TriggerControlSetupWidget::on_m_qPushButton_nullen_released()
{
    QByteArray t_data;
    //t_data = m_pTriggerControl->m_pSerialPort->m_data;

    t_data.resize(3);
    int value = 1;
    t_data.append(value);

    std::cout << t_data.size() << std::endl;

    m_pTriggerControl->m_pSerialPort->sendData(t_data);
}
