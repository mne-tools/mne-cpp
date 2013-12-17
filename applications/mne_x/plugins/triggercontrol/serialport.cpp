//=============================================================================================================
/**
* @file     serialport.cpp
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
* @brief    Contains the implementation of the SerialPort class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "serialport.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMessageBox>
#include <QDebug>
#include <QSerialPort>
#include <QSerialPortInfo>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TriggerControlPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SerialPort::SerialPort()
{
    initSettings();
    initPort();
}



//*************************************************************************************************************

SerialPort::~SerialPort()
{

}


//*************************************************************************************************************

void SerialPort::close()
{
    m_qSerialPort.close();
    qDebug() << "port geschlossen" << endl;
}


//*************************************************************************************************************

void SerialPort::initSettings()
{
    m_currentSettings.name = "";
    m_currentSettings.baudRate = QSerialPort::Baud115200;
    m_currentSettings.stringBaudRate = "115200";
    m_currentSettings.dataBits = QSerialPort::Data8;
    m_currentSettings.stringDataBits = "8";
    m_currentSettings.parity = QSerialPort::NoParity;
    m_currentSettings.stringParity = "None";
    m_currentSettings.stopBits = QSerialPort::OneStop;
    m_currentSettings.stringStopBits = "1";
    m_currentSettings.flowControl = QSerialPort::NoFlowControl;
    m_currentSettings.stringFlowControl = "None";
}

//*************************************************************************************************************

void SerialPort::initPort()
{
    QString sPortName;

    QList<QSerialPortInfo> t_qListPortInfo = QSerialPortInfo::availablePorts();

//    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
// qDebug() << "Port gefunden und geändert, "<< info.portName() << endl;
//        sPortName = info.portName();

//    }

    if(t_qListPortInfo.size() > 0)
    {
        m_currentSettings.name = t_qListPortInfo[0].portName();

        m_qSerialPort.setPortName( t_qListPortInfo[0].portName());
    }

}


//*************************************************************************************************************
void SerialPort::sendData(const QByteArray &data)
{
        m_qSerialPort.write(data);
}



//*************************************************************************************************************

bool SerialPort::open()
{
    bool success = false;

    // get current settings
    m_qSerialPort.setPortName(m_currentSettings.name);




 //   qDebug() << "noch nicht geöffnet" << endl;
    if (m_qSerialPort.open(QIODevice::ReadWrite))
    {    qDebug() << "geöffnet, ohne Konfigs" << endl;
        if (m_qSerialPort.setBaudRate(m_currentSettings.baudRate)
                && m_qSerialPort.setDataBits(m_currentSettings.dataBits)
                && m_qSerialPort.setParity(m_currentSettings.parity)
                && m_qSerialPort.setStopBits(m_currentSettings.stopBits)
                && m_qSerialPort.setFlowControl(m_currentSettings.flowControl))
        {    qDebug() << "geöffnet, mit:"
                      << "Name" << m_currentSettings.name
                      << "BaudRat" << m_currentSettings.stringBaudRate
                      << "Databits" << m_currentSettings.stringDataBits
                      << "Parity" << m_currentSettings.stringParity
                      << "FlowControl" << m_currentSettings.stringFlowControl  << endl;
//            ui->pushButton_connect->setEnabled(false);
//            ui->pushButton_disconnect->setEnabled(true);
//            ui->pushButton_opensettings->setEnabled(false);
//            ui->pushButton_send->setEnabled(true);
//            ui->statusBar->showMessage(tr("Verbunden mit %1 : %2, %3, %4, %5, %6")
//                                       .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
//                                       .arg(p.stringParity).arg(p.stringStopBits)
//                                       .arg(p.stringFlowControl));
            success = true;
        }
        else
        {    qDebug() << "nicht geöffnet, Fehler mit KOnfigurationszuweisung" << endl;
            m_qSerialPort.close();
         //   QMessageBox::critical(this,tr("Error"),serial->errorString());
          //  ui->statusBar->showMessage(tr("Fehler beim Öffnen"));
            success = false;
        }

    }
    else
    {    qDebug() << "nicht geöffnet, Fehler schon beim readwriteöffnen" << endl;
       // QMessageBox::critical(this,tr("Error"),serial->errorString());
      //  ui->statusBar->showMessage(tr("Konfigurationsfehler"));
        success = false;
    }

    return success;
}




