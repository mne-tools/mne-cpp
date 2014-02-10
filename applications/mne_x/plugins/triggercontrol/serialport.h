//=============================================================================================================
/**
* @file     serialport.h
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
* @brief    Contains the declaration of the SerialPort class.
*
*/

#ifndef SERIALPORT_H
#define SERIALPORT_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "triggercontrol.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSerialPort>
#include <QVector>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TriggerControlPlugin
//=============================================================================================================

namespace TriggerControlPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace MNEX;
//using namespace TriggerControlPlugin;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//class TriggerControlSetupWidget;
//class TriggerControl;
//class SettingsWidget;

//=============================================================================================================
/**
* DECLARE CLASS TriggerControl
*
* @brief The TriggerControl ....
*/
class SerialPort : public QObject
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs a SerialPort.
    */
    SerialPort();

    //=========================================================================================================
    /**
    * Destroys the SerialPort.
    */
    ~SerialPort();


    void initSettings();    /**< Initializes Settings as #data bits or baud rate.*/
    void initPort();        /**< Checks all available serial ports and initializes the one of the trigger box.*/

    bool open();            /**< Opens a communication channel to the serial port.*/
    void close();           /**< Closes a communication channel to the serial port.*/


    void encodedig();       /**< Encodes the chosen digital channels according to the data transfer protocol.*/
    void encodeana();       /**< Encodes the chosen analog channels and values according to the data transfer protocol.*/
    void encoderetr();      /**< Encodes a retrieve bytearray according to the data transfer protocol.*/

    void decodedig(QByteArray &t_incomingArray);       /**< Decodes the incoming digital information according to the data transfer protocol.*/
    void decodeana(QByteArray &t_incomingArray);       /**< Decodes the incoming analog information according to the data transfer protocol.*/

    void sendData(const QByteArray &data);    /**< Sends an array of bytes to the configured serial port.*/
    void readData();

 //   void writeData(const QByteArray &data);


   // void handleError(QSerialPort::SerialPortError error);

    QByteArray m_data;
    QVector<int> m_digchannel;      // current digital channels (OUT)
    int m_motor;                    // current selected analog channel (OUT)
    int m_analval;                  // current analog value (OUT)

    QVector<int> m_InAnChannelVal;        // contains the analog values coming from the MUC - channel specific position
    QVector<int> m_InActiveDig;

    int m_retrievetyp;
    int m_retrievechan;


    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
    };

    inline Settings& settings()
    {
        return m_currentSettings;
    }

signals:
    void dataAvailable(const QByteArray);
    void byteReceived();


protected:

private:
    Settings m_currentSettings;
    QSerialPort m_qSerialPort;
};

} // Namespace

#endif // SERIALPORT_H






