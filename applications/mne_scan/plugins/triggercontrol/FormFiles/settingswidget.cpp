//=============================================================================================================
/**
* @file     triggercontrol.cpp
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
* @brief    Definition of the SettingsWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "settingswidget.h"
#include "ui_settingswidget.h"

#include "../triggercontrol.h"

#include "triggercontrolsetupwidget.h"

#include "serialport.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TriggerControlPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SettingsWidget::SettingsWidget(QWidget *parent)
: QDialog(parent)
, ui(new Ui::SettingsWidget)
{
    ui->setupUi(this);

    connect(ui->m_qPushButton_apply,SIGNAL(released()),this, SLOT(apply()));
//    connect(ui->m_qPushButton_apply, &QPushButton::released,  this, SettingsWidget::apply());

    connect(ui->m_qComboBox_port, SIGNAL(currentIndexChanged(int)),this, SLOT(showPortInfo(int)));
 //   connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)),
  //          this, SLOT(checkCustomBaudRatePolicy(int)));

    fillPortsParameters();
    fillPortsInfo();

   // initSettings();   --> eigentlich reicht es, wenn wir das hier weglassen, denn initial wurden die richtigen
    //                      Parameter ja schon gesetzt. Eine Änderung durch die comboboxen ruft weiterhin updatesettings auf
}


//*************************************************************************************************************

SettingsWidget::~SettingsWidget()
{
    delete ui;
}


//*************************************************************************************************************

void SettingsWidget::apply()
{
    updateSettings();
    close();
}


//*************************************************************************************************************

void SettingsWidget::fillPortsInfo()
{
    ui->m_qComboBox_port->clear();

    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {

        QStringList list;
        list << info.portName()
             << info.description()
             << info.manufacturer()
             << info.systemLocation();
//             << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString())
 //            << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : QString());

        ui->m_qComboBox_port->addItem(list.first(), list);
    }
}


//*************************************************************************************************************

void SettingsWidget::fillPortsParameters()
{
    ui->m_qComboBox_baudrate->addItem(QLatin1String("9600"), QSerialPort::Baud9600);
    ui->m_qComboBox_baudrate->addItem(QLatin1String("19200"), QSerialPort::Baud19200);
    ui->m_qComboBox_baudrate->addItem(QLatin1String("38400"), QSerialPort::Baud38400);
    ui->m_qComboBox_baudrate->addItem(QLatin1String("115200"), QSerialPort::Baud115200);
    ui->m_qComboBox_baudrate->setCurrentIndex(3);

    ui->m_qComboBox_databits->addItem(QLatin1String("7"),QSerialPort::Data7);
    ui->m_qComboBox_databits->addItem(QLatin1String("8"),QSerialPort::Data8);
    ui->m_qComboBox_databits->setCurrentIndex(1);

    ui->m_qComboBox_parity->addItem(QLatin1String("None"), QSerialPort::NoParity);
    ui->m_qComboBox_parity->addItem(QLatin1String("Even"), QSerialPort::EvenParity);
    ui->m_qComboBox_parity->addItem(QLatin1String("Odd"), QSerialPort::OddParity);
    ui->m_qComboBox_parity->addItem(QLatin1String("Mark"), QSerialPort::MarkParity);
    ui->m_qComboBox_parity->addItem(QLatin1String("Space"), QSerialPort::SpaceParity);

    ui->m_qComboBox_stopbits->addItem(QLatin1String("1"), QSerialPort::OneStop);
    ui->m_qComboBox_stopbits->addItem(QLatin1String("2"), QSerialPort::TwoStop);
    ui->m_qComboBox_stopbits->setCurrentIndex(1);

    ui->m_qComboBox_flowcontrol->addItem(QLatin1String("None"), QSerialPort::NoFlowControl);
    ui->m_qComboBox_flowcontrol->addItem(QLatin1String("RTS/CTS"), QSerialPort::HardwareControl);
    ui->m_qComboBox_flowcontrol->addItem(QLatin1String("XON/XOFF"), QSerialPort::SoftwareControl);
}

//*************************************************************************************************************

void SettingsWidget::showPortInfo(int idx)
{
    if (idx != -1) {
        QStringList list = ui->m_qComboBox_port->itemData(idx).toStringList();
        ui->m_qLabel_description->setText(tr("Description: %1").arg(list.at(1)));
        ui->m_qLabel_producer->setText(tr("Manufacturer: %1").arg(list.at(2)));
        ui->m_qLabel_place->setText(tr("Location: %1").arg(list.at(3)));

    }
}


//*************************************************************************************************************

void SettingsWidget::updateSettings()
{
    TriggerControlSetupWidget* tcsWidget = static_cast<TriggerControlSetupWidget*> (this->parentWidget()); //Upcast from QWidget to TriggerControlSetupWidget

    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().name = ui->m_qComboBox_port->currentText();

    // Baud Rate

    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().baudRate = static_cast<QSerialPort::BaudRate>(
                    ui->m_qComboBox_baudrate->itemData(ui->m_qComboBox_baudrate->currentIndex()).toInt());
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stringBaudRate = QString::number(tcsWidget->m_pTriggerControl->m_pSerialPort->settings().baudRate);

    // Data bits
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().dataBits = static_cast<QSerialPort::DataBits>(
                ui->m_qComboBox_databits->itemData(ui->m_qComboBox_databits->currentIndex()).toInt());
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stringDataBits = ui->m_qComboBox_databits->currentText();

    // Parity
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().parity = static_cast<QSerialPort::Parity>(
                ui->m_qComboBox_parity->itemData(ui->m_qComboBox_parity->currentIndex()).toInt());
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stringParity = ui->m_qComboBox_parity->currentText();

    // Stop bits
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stopBits = static_cast<QSerialPort::StopBits>(
                ui->m_qComboBox_stopbits->itemData(ui->m_qComboBox_stopbits->currentIndex()).toInt());
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stringStopBits = ui->m_qComboBox_stopbits->currentText();

    // Flow control
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().flowControl = static_cast<QSerialPort::FlowControl>(
                ui->m_qComboBox_flowcontrol->itemData(ui->m_qComboBox_flowcontrol->currentIndex()).toInt());
    tcsWidget->m_pTriggerControl->m_pSerialPort->settings().stringFlowControl = ui->m_qComboBox_flowcontrol->currentText();
}
