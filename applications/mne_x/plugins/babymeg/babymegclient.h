//=============================================================================================================
/**
* @file     babymegclient.h
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the BabyMEGClient Class.
*
*/

#ifndef BABYMEGCLIENT_H
#define BABYMEGCLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QTcpSocket>
#include <QMutex>
#include <QThread>
#include <QDataStream>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeginfo.h"


class QTcpSocket;
class QNetworkSession;


//=============================================================================================================
/**
* DECLARE CLASS BabyMEGClient
*
* @brief The BabyMEGClient class provides a TCP/IP communication between Qt and Labview.
*/
class BabyMEGClient : public QThread
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a BabyMEG.
    */
    explicit BabyMEGClient(int myPort, QObject *parent = 0);

    ~BabyMEGClient();

    //=========================================================================================================
    /**
    * Convert an integer (4 bytes) to a 4-byte array
    *
    * @param[in] a -- <int>.
    * @param[out] Byte array
    */
    QByteArray MGH_LM_Int2Byte(int a);

    //=========================================================================================================
    /**
    * Convert a 4-byte array to an integer
    *
    * @param[in] InByte -- Byte array
    * @param[out] <int>.
    */
    int MGH_LM_Byte2Int(QByteArray InByte);

    //=========================================================================================================
    /**
    * Convert one 8-byte array to a double
    *
    * @param[in] InByte -- Byte array
    * @param[out] <double>.
    */
    double MGH_LM_Byte2Double(QByteArray InByte);

    //=========================================================================================================
    /**
    * Hex display
    *
    * @param[in] a -- double number
    */
    void HexDisplay(double a);

    //=========================================================================================================
    /**
    * Set Head Info
    *
    * @param[in] pInfo -- struct of header information
    */
    void SetInfo(QSharedPointer<BabyMEGInfo> pInfo);

    //=========================================================================================================
    /**
    * Dispatch the data package
    *
    * @param[in] tmp -- block size
    */
    void DispatchDataPackage(int tmp);

    //=========================================================================================================
    /**
    * Read next data block
    *
    * @param[in] tmp -- block size
    */
    void ReadNextBlock(int tmp);

    //=========================================================================================================
    /**
    * Send command with command format as string
    *
    * @param[in] s -- string
    */
    void SendCommand(QString s);

    //=========================================================================================================
    /**
    * Handle the data buffer connecting to the TCP socket
    *
    * @param[in] void
    */
    void handleBuffer();

    inline bool isConnected() const;

signals:
    void DataAcq();
    void error(int socketError, const QString &message);

public slots:
    //=========================================================================================================
    /**
    * Connect to BabyMEG server
    *
    * @param[in] void.
    */
    void ConnectToBabyMEG();

    //=========================================================================================================
    /**
    * DisConnect to BabyMEG server
    *
    * @param[in] void.
    */
    void DisConnectBabyMEG();

    //=========================================================================================================
    /**
    * Send Command to BabyMEG server
    *
    * @param[in] void.
    */
    void SendCommandToBabyMEG();
    void DisplayError(int socketError, const QString &message);

    //=========================================================================================================
    /**
    * Read data from socket to a buffer
    *
    * @param[in] void.
    */
    void ReadToBuffer();
    void run();

    //=========================================================================================================
    /**
    * Send Command to BabyMEG command server with short sync connection
    *
    * @param[in] String s - the string will be sent to server.
    */
    void SendCommandToBabyMEGShortConnection(QByteArray s);

public:
    QString name;
    int port;
    bool SkipLoop;
    bool DataAcqStartFlag;
    QSharedPointer<BabyMEGInfo> myBabyMEGInfo;

    QByteArray buffer;
    int numBlock;
    bool DataACK;

private:
    bool m_bSocketIsConnected;
    QTcpSocket *tcpSocket;
    QMutex m_qMutex;

};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool BabyMEGClient::isConnected() const
{
    return m_bSocketIsConnected;
}

#endif // BABYMEGCLIENT_H
