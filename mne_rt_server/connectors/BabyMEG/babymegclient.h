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
#include <QtNetWork/QTcpSocket>
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
public:
    QString name;
    int port;
    bool SocketIsConnected;
    bool SkipLoop;
    bool DataAcqStartFlag;
    BabyMEGInfo *myBabyMEGInfo;

    QByteArray buffer;
    int numBlock;
    bool DataACK;

private:
    QTcpSocket *tcpSocket;
    QMutex m_qMutex;
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
    * @param[in] void.
    */
    void SendCommandToBabyMEGShortConnection();

// public function
public:
    QByteArray MGH_LM_Int2Byte(int a);
    int MGH_LM_Byte2Int(QByteArray InByte);
    double MGH_LM_Byte2Double(QByteArray InByte);
    void HexDisplay(double a);
    void SetInfo(BabyMEGInfo *pInfo);

    void DispatchDataPackage(int tmp);
    void ReadNextBlock(int tmp);
    void SendCommand(QString s);
    void handleBuffer();
};

#endif // BABYMEGCLIENT_H
