//=============================================================================================================
/**
 * @file     serialport.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     November, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
// DEFINE NAMESPACE TRIGGERCONTROLPLUGIN
//=============================================================================================================

namespace TRIGGERCONTROLPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================




//=============================================================================================================
/**

 * DECLARE CLASS SerialPort
 *

 * @brief The SerialPort is a class which holds all properties and methods necesarry to open, communicate and
 * close a serial port. In most cases you want to open the port, encode your output information (digital, analog
 * or retrieve) and decode input information (digital or analog). When you are done, close the serial port.
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

    //=========================================================================================================
    /**
    * Initializes Settings as data bits or baud rate, parity, stop bits and flow control.
    *
    */
    void initSettings();

    //=========================================================================================================
    /**
    * Checks all available serial ports for the one desired and initializes to that.
    *
    */
    void initPort();

    //=========================================================================================================
    /**
    * Opens a channel to the serial port.
    *
    */
    bool open();

    //=========================================================================================================
    /**
    * Closes the channel to the serial port.
    *
    */    
	void close();

    //=========================================================================================================
    /**
    * Encodes the selected digital output channels according to a data transfer protocol (see manual).
    *
    */
    void encodedig();

    //=========================================================================================================
    /**
    * Encodes the selected analog output channel according to a data transfer protocol (see manual).
    *
    */
    void encodeana();

    //=========================================================================================================
    /**
    * Encodes a retrieve byte array according to a data transfer protocol (see manual).
    *
    */
    void encoderetr();

    //=========================================================================================================
    /**
    * Decodes the digital input information according to a data transfer protocol (see manual).
    * @param [in] QByteArray input byte array according to transfer protocol (see manual).    *
    */
    void decodedig(QByteArray &t_incomingArray);

    //=========================================================================================================
    /**
    * Decodes the analog input information according to a data transfer protocol (see manual).
    * @param [in] QByteArray input byte array according to transfer protocol (see manual).

    */
    void decodeana(QByteArray &t_incomingArray);

    //=========================================================================================================
    /**
    * Sends a byte array to the configured serial port
    * @param [in] QByteArray output byte array according to transfer protocol (see manual).
    */
    void sendData(const QByteArray &data);

    //=========================================================================================================
    /**
    * Reads the input information after checking whether it is formally correct.
    *
    */
    void readData();



    QByteArray          m_data;                     /**< Holds the byte array. */
    QVector<int>        m_digchannel;               /**< Holds the currently selected digital output channel. */
    int                 m_motor;                    /**< Holds the currently selected analog output channel. */
    int                 m_analval;                  /**< Holds the current analog output value. */

    QVector<int>        m_InAnChannelVal;           /**< Lists the analog values of the input channels .*/
    QVector<int>        m_InActiveDig;              /**< Lists the digital states of the input channels. */
 
    int                 m_retrievetyp;              /**< Holds the desired input mode (analog or digital). */
    int                 m_retrievechan;             /**< Holds the desired analog input channel. */

    int                 m_wiredChannel;             /**< Holds the channel which is connected to the TriggerControl Run Method. */

    //=========================================================================================================
    /**
    * Holds the information which are necessery to define the serial port.
    * @param [in] name name of the port.
    * @param [in] baudRate desired baud rate.
    * @param [in] stringBaudRate desired baud rate as string.
    * @param [in] dataBits desired data bits.
    * @param [in] stringDataBits string of desired data bits.
    * @param [in] parity desired parity.
    * @param [in] stringParity string of desired parity.
    * @param [in] stopBits desired stop bits.
    * @param [in] stringStopBits string of desired stop bits.
    * @param [in] flowControl desired flow control.
    * @param [in] stringFlowControl string of desired flow control.
    */
    struct Settings {
        QString             name;
        qint32              baudRate;
        QString             stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString             stringDataBits;
        QSerialPort::Parity parity;
        QString             stringParity;
        QSerialPort::StopBits stopBits;
        QString             stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString             stringFlowControl;
    };


//=========================================================================================================
/**
 * returns the current settings
 *
 */
    inline Settings& settings()
    {
        return m_currentSettings;
    }

//=========================================================================================================

signals:
    void dataAvailable(const QByteArray);       /**< [...] .*/

    void byteReceived();                        /**< [...] .*/


protected:

private:
    Settings m_currentSettings;
    QSerialPort m_qSerialPort;
};

} // Namespace

#endif // SERIALPORT_H






