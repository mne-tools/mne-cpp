//=============================================================================================================
/**
 * @file     babymegclient.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Limin Sun, Lorenz Esch. All rights reserved.
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
 * @brief     BabyMEGClient class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymegclient.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtNetwork>
#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEGClient::BabyMEGClient(int myPort, QObject *parent)
: QThread(parent)
{
    connect(this, &BabyMEGClient::DataAcq,
            this, &BabyMEGClient::run);

    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, &QTcpSocket::readyRead,
            this, &BabyMEGClient::ReadToBuffer);

    connect(this, &BabyMEGClient::error,
            this, &BabyMEGClient::DisplayError);     //find out name of this machine

    name = QHostInfo::localHostName();
    if(!name.isEmpty())
    {
        QString domain = QHostInfo::localDomainName();
        if (!domain.isEmpty())
            name = name + QChar('.') + domain;
    }
    if (name!=QString("localhost"))
        name = QString("localhost");
    qDebug()<< "- " + name;
    port = myPort;//6340;
    m_bSocketIsConnected = false;
    SkipLoop = false;
    DataAcqStartFlag = false;
    numBlock = 0;
    DataACK = false;
}

//=============================================================================================================

BabyMEGClient::~BabyMEGClient()
{
    delete tcpSocket;   // added 5.31.2013
}

//=============================================================================================================

void BabyMEGClient::SetInfo(QSharedPointer<BabyMEGInfo> pInfo)
{
    myBabyMEGInfo = pInfo;
}

//=============================================================================================================

void BabyMEGClient::DisplayError(int socketError, const QString &message)
{
    switch (socketError){
    case QAbstractSocket::RemoteHostClosedError:
            break;
    case QAbstractSocket::HostNotFoundError:
        qDebug()<< "The host was not found. Please check the host name and the port number";
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qDebug()<< "The connection was refused by the peer. Make sure the server is running?";
        break;
    default:
        qDebug()<< "Error: " << message;
    }
}

//=============================================================================================================

int BabyMEGClient::MGH_LM_Byte2Int(QByteArray b)
{
    int value= 0;
    for (int i=0;i<2;i++)
    {
        QByteArray t;
        t[0] = b[i];
        b[i] = b[3-i];
        b[3-i] = t[0];
    }
    memcpy((char *)&value,b,4);
    return value;
}

//=============================================================================================================

QByteArray BabyMEGClient::MGH_LM_Int2Byte(int a)
{
    QByteArray b = QByteArray::fromRawData((char *)&a,4);

    for (int i=0;i<2;i++)
    {
        QByteArray t;
        t[0] = b[i];
        b[i] = b[3-i];
        b[3-i] = t[0];
    }
    return b;
}

//=============================================================================================================

double BabyMEGClient::MGH_LM_Byte2Double(QByteArray b)
{
    double value= 1.0;
    // reverse the byte order
    for (int i=0;i<4;i++)
    {
        QByteArray t;
        t[0] = b[i];
        b[i] = b[7-i];
        b[7-i] = t[0];
    }
    memcpy((char *)&value,b,8);

    return value;
}

//=============================================================================================================

void BabyMEGClient::HescDisplay(double a)
{
    QByteArray data = QByteArray::fromRawData((char *)&a,8);
    qDebug() << data.toHex();
}

//=============================================================================================================

void BabyMEGClient::ConnectToBabyMEG()
{
    m_bSocketIsConnected = false;

    // Connect to the server of babyMEG [labview]
    qDebug()<< "Client is started!";

    for(int i = 0; i < 10; i++) {
        tcpSocket->connectToHost(name,port,QIODevice::ReadWrite);

        if (tcpSocket->waitForConnected(5000)) {
            m_bSocketIsConnected = true;
            qDebug("Connect to BabyMEG Server ... Ok");

            //download parameters
            qDebug()<< "Send the initial parameter request";

            if (tcpSocket->state()==QAbstractSocket::ConnectedState)  {
                buffer.clear();
//                SendCommand("INFO");
                SendCommand("DATA");
            }

            return;
        } else{
            qDebug("Connection to BabyMEG server failed");
            qDebug("Retry...");
        }

        qDebug("Please check the babyMEG server: if started");
    }

    return;
}

//=============================================================================================================

void BabyMEGClient::DisconnectBabyMEG()
{
    if(m_bSocketIsConnected && tcpSocket->state()==QAbstractSocket::ConnectedState)
        SendCommand("QUIT");
}

//=============================================================================================================

void BabyMEGClient::SendCommandToBabyMEGShortConnection(QByteArray s)
{
    qDebug() << "SendCommandToBabyMEGShortConnection";

    bool connected = (tcpSocket->state() == QTcpSocket::ConnectedState);
    if(connected)
    {
        tcpSocket->disconnectFromHost();
        if(tcpSocket->state() != QAbstractSocket::UnconnectedState)
                    tcpSocket->waitForDisconnected();
    }

    tcpSocket->connectToHost(name,port,QIODevice::ReadWrite);
    if (tcpSocket->waitForConnected(10000))
    {

        qDebug()<<"Connection is built.";

        if(tcpSocket->state()==QAbstractSocket::ConnectedState)
        {
            qDebug()<<"Send String [" << s << "]\n"<<"length["<<s.size()<<"]\n";
            tcpSocket->write(s);
//            int strlen = s.size();
//            QByteArray Scmd = MGH_LM_Int2Byte(strlen);

//            tcpSocket->write(Scmd);
//            tcpSocket->write("SLM");
            tcpSocket->waitForBytesWritten();
        }
        else
        {
            qDebug()<<"Connect state is abnormal:"<<tcpSocket->state();
        }
    }
}

//=============================================================================================================

void BabyMEGClient::SendCommandToBabyMEG()
{
    qDebug()<<"Send Command";
    if(m_bSocketIsConnected && tcpSocket->state()==QAbstractSocket::ConnectedState)
    {
        m_qMutex.lock();
        tcpSocket->write("COMD");
        tcpSocket->waitForBytesWritten();

        int strlen = 3;
        QByteArray Scmd = MGH_LM_Int2Byte(strlen);

        tcpSocket->write(Scmd);
        tcpSocket->write("SLM");
        tcpSocket->waitForBytesWritten();
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void BabyMEGClient::ReadToBuffer()
{

    QByteArray dat;

    int numBytes = tcpSocket->bytesAvailable();
//    qDebug() << "1.byte available: " << numBytes;
    if (numBytes > 0){
        dat = tcpSocket->read(numBytes); // read all pending data
//        qDebug()<<"[dat Size]"<<dat.size();
        if (!dat.isEmpty()){
            buffer.append(dat); // and append it to your own buffer
//            qDebug()<<"[ReadToBuffer: Buffer Size]"<<buffer.size();
        }
        else
        {
            qDebug()<<"[Empty dat: error]"<<tcpSocket->errorString();
        }
    }
//    qDebug()<<"read buffer is done!";

    handleBuffer();
    return;
}

//=============================================================================================================

void BabyMEGClient::handleBuffer()
{
    if(buffer.size()>= 8){
        QByteArray CMD = buffer.left(4);
        QByteArray DLEN = buffer.mid(4,4);
        int tmp = MGH_LM_Byte2Int(DLEN);
//        qDebug() << "First 4 bytes + length" << CMD << "["<<CMD.toHex()<<"]";
//        qDebug() << "Command[" << CMD <<"]";
//        qDebug() << "Body Length[" << tmp << "]";

        if (tmp <= (buffer.size() - 8))
        {
            buffer.remove(0,8);

            int OPT = 0;

            if (CMD == "INFO")
                OPT = 1;
            else if (CMD == "DATR")
                OPT = 2;
            else if (CMD == "COMD")
                OPT = 3;
            else if (CMD == "QUIT")
                OPT = 4;
            else if (CMD == "COMS")
                OPT = 5;
            else if (CMD == "QUIS")
                OPT = 6;
            else if (CMD == "INFG")
                OPT = 7;

            switch (OPT){
            case 1:
                // from buffer get data package
                {
                QByteArray PARA = buffer.left(tmp);
                qDebug()<<"[INFO]"<<PARA;
                //Parse parameters from PARA string
                myBabyMEGInfo->MGH_LM_Parse_Para(PARA);
                buffer.remove(0,tmp);
                qDebug()<<"INFO has been received!!!!";
//                qDebug()<<"ACQ Start";
//                SendCommand("DATA");
                }
                break;
            case 2:
                // read data package from buffer
                // Ask for the next data block

                SendCommand("DATA");
                DispatchDataPackage(tmp);

                break;
            case 3:
                {
                QByteArray RESP = buffer.left(tmp);
                qDebug()<< "5.Readbytes:"<<RESP.size();
                qDebug() << RESP;
                }
                buffer.remove(0,tmp);

                break;
            case 4:  //quit
                qDebug()<<"Quit";

                SendCommand("QREL");
                tcpSocket->disconnectFromHost();
                if(tcpSocket->state() != QAbstractSocket::UnconnectedState)
                            tcpSocket->waitForDisconnected();
                m_bSocketIsConnected = false;
                qDebug()<< "Disconnect Server";
                qDebug()<< "Client is End!";
                qDebug()<< "You can close this application or restart to connect Server.";

                break;
            case 5://command short connection
                {
                QByteArray RESP = buffer.left(tmp);
                qDebug()<< "5.Readbytes:"<<RESP.size();
                qDebug() << RESP;
                myBabyMEGInfo->MGH_LM_Send_CMDPackage(RESP);
                }
                buffer.remove(0,tmp);
                SendCommand("QUIT");
                break;
            case 6:  //quit
                qDebug()<<"Quit";

                SendCommand("QREL");
                tcpSocket->disconnectFromHost();
                if(tcpSocket->state() != QAbstractSocket::UnconnectedState)
                            tcpSocket->waitForDisconnected();
                m_bSocketIsConnected = false;
                qDebug()<< "Disconnect Server";
                break;
            case 7: //INFG
                {
                QByteArray PARA = buffer.left(tmp);
                qDebug()<<"[INFG]"<<PARA;
                //Parse parameters from PARA string
                myBabyMEGInfo->MGH_LM_Parse_Para_Infg(PARA);
                buffer.remove(0,tmp);
                qDebug()<<"INFG has been received!!!!";
                }
                break;

            default:
                qDebug()<< "Unknow Type";
                break;
            }
        }
    }// buffer is not empty and larger than 8 bytes
}

//=============================================================================================================

void BabyMEGClient::DispatchDataPackage(int tmp)
{

//    qDebug()<<"Acq data from buffer  [buffer size() =" << buffer.size()<<"]";
    QByteArray DATA = buffer.left(tmp);
    qDebug()<< "5.Readbytes:"<<DATA.size();
    myBabyMEGInfo->MGH_LM_Send_DataPackage(DATA);
//    myBabyMEGInfo->EnQueue(DATA);
    buffer.remove(0,tmp);
//    qDebug()<<"Rest buffer  [buffer size() =" << buffer.size()<<"]";
    numBlock ++;
    qDebug()<< "Next Block ..." << numBlock;
//    DATA.clear();

    ReadNextBlock(tmp);
}

//=============================================================================================================

void BabyMEGClient::ReadNextBlock(int tmp)
{
    QByteArray CMD1;
    QByteArray DLEN1;
    QByteArray DATA1;
    int tmp1;
    while (buffer.size()>=(tmp+8))
    { // process the extra data block to reduce the load of data buffer
        CMD1 = buffer.left(4);
        qDebug()<<"CMD"<< CMD1;
        if (CMD1 == "DATR")
        {
            DLEN1 = buffer.mid(4,4);
            tmp1 = MGH_LM_Byte2Int(DLEN1);
            qDebug() << "[2]First 4 bytes + length" << CMD1 << "["<<CMD1.toHex()<<"]";
            qDebug() << "[2]Command[" << CMD1 <<"]";
            qDebug() << "[2]Body Length[" << tmp1 << "]";

            buffer.remove(0,8);
            DATA1 = buffer.left(tmp1);
            myBabyMEGInfo->MGH_LM_Send_DataPackage(DATA1);
//            myBabyMEGInfo->EnQueue(DATA1);
            buffer.remove(0,tmp1);
            qDebug()<<"End of DataPackeage" << buffer.left(3);
            qDebug()<<"[2]Rest buffer  [buffer size() =" << buffer.size()<<"]";
            numBlock ++;
            qDebug()<< "[2]Next Block ..." << numBlock;
        }
        else
        {
            qDebug()<<"[CMD1]"<<CMD1.toHex();
            break;
        }
        qDebug()<<"[ReadNextBlock:buffer size]"<<buffer.size();
    }
    DATA1.clear();
    CMD1.clear();
    DLEN1.clear();
}

//=============================================================================================================

void BabyMEGClient::SendCommand(QString s)
{
    QByteArray array;
    array.append(s);

    qint64 WrtNum;

    if (tcpSocket->state()==QAbstractSocket::ConnectedState)
    {
        m_qMutex.lock();

//    qDebug()<<"[Send Command]"<<array;
        WrtNum = tcpSocket->write(array,4);
        if(WrtNum==-1)
        {
            qDebug()<<"Error for sending a command";
        }
        if(WrtNum != array.size())
        {
            qDebug()<<"Uncorrectly sending";
        }
        tcpSocket->flush();
        tcpSocket->waitForBytesWritten();
        m_qMutex.unlock();
        qDebug()<<"[Done: Send Command]"<<array<<"[Send bytes]"<<WrtNum;

        }
        else
        {
            qDebug()<<"Not in Connected state";
            //re-connect to server
            ConnectToBabyMEG();
            buffer.clear();
            SendCommand("DATA");
        }
//    sleep(1);
}

//=============================================================================================================

void BabyMEGClient::run()
{
}
