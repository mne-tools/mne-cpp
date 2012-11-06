//=============================================================================================================
/**
* @file     commandthread.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the CommandThread Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_rt_server.h"
#include "commandthread.h"
#include "fiffstreamserver.h"
#include "fiffstreamthread.h"

#include <QtNetwork>
#include <QStringList>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandThread::CommandThread(int socketDescriptor, QObject *parent)
: QThread(parent)
, socketDescriptor(socketDescriptor)
{

}


//*************************************************************************************************************

QByteArray CommandThread::availableCommands()
{
    QByteArray t_blockCmdInfoList;
    t_blockCmdInfoList.append("\tclist\t\t\tprints and sends all available fiff stream clients\r\n");
    t_blockCmdInfoList.append("\tmeasinfo [ID/Alias]\tsends the measurement info to the specified fiff\r\n\t\t\t\tstream client\r\n");
    t_blockCmdInfoList.append("\tmeas     [ID/Alias]\tadds specified fiff stream client to raw data\r\n\t\t\t\tbuffer receivers. If acquisition is not already strated, it is triggered.\r\n");
    t_blockCmdInfoList.append("\tstop     [ID/Alias]\tremoves specified fiff stream client from raw\r\n\t\t\t\tdata buffer receivers.\r\n");
    t_blockCmdInfoList.append("\tstop-all\t\tstops the whole acquisition process.\r\n");
    t_blockCmdInfoList.append("\tbufsize  [samples]\tsets the buffer size of the fiff stream client\r\n\t\t\t\traw data buffers\r\n");

    t_blockCmdInfoList.append("\n\tconnectors\t\tprints and sends all available connectors\r\n");

    t_blockCmdInfoList.append("\n\thelp\t\t\tprints and sends this list\r\n\n");

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

bool CommandThread::parseCommand(QTcpSocket& p_qTcpSocket, QString& p_sCommand)
{
    QStringList t_qCommandList = p_sCommand.split(" ");


    bool success = false;
    QByteArray t_blockClientList;
    if(t_qCommandList[0].compare("clist",Qt::CaseInsensitive) == 0)
    {
        //
        // Print & send client list
        //
        FiffStreamServer* t_pFiffStreamServer = qobject_cast<MNERTServer*>(this->parent()->parent())->m_pFiffStreamServer;
        if(t_pFiffStreamServer)
        {
            t_blockClientList.append("\tID\tAlias\r\n");
            QMap<qint32, FiffStreamThread*>::iterator i;
            for (i = t_pFiffStreamServer->m_qClientList.begin(); i != t_pFiffStreamServer->m_qClientList.end(); ++i)
            {
                QString str = QString("\t%1\t%2\r\n").arg(i.key()).arg(i.value()->getAlias());
                t_blockClientList.append(str);
            }
            success = true;
        }
        else
        {
            t_blockClientList.append("Error: fiff stream not available.\r\n");
            success = false;
        }
    }
    else if(t_qCommandList[0].compare("measinfo",Qt::CaseInsensitive) == 0)
    {
        //
        // Measurement Info
        //
        qint32 t_id = -1;

        t_blockClientList.append(parseToId(t_qCommandList[1],t_id));

        if(t_id != -1)
        {
            emit requestMeasInfo(t_id);

            QString str = QString("\tsend measurement info to fiff stream client (ID: %1)\r\n\n").arg(t_id);
            t_blockClientList.append(str);
        }
    }
    else if(t_qCommandList[0].compare("help",Qt::CaseInsensitive) == 0)
    {
        //
        // Help
        //
        t_blockClientList.append(CommandThread::availableCommands());
    }
    else
    {
        //
        // Unknown command
        //
        t_blockClientList.append("command unknown\r\n");
        t_blockClientList.append(CommandThread::availableCommands());
    }

    //print
    std::cout << t_blockClientList.data();
    //send
    p_qTcpSocket.write(t_blockClientList);
    p_qTcpSocket.waitForBytesWritten();

    return success;
}


//*************************************************************************************************************

QByteArray CommandThread::parseToId(QString& p_sRawId, qint32& p_iParsedId)
{
    p_iParsedId = -1;
    QByteArray t_blockCmdIdInfo;
    if(!p_sRawId.isEmpty())
    {
        FiffStreamServer* t_pFiffStreamServer = qobject_cast<MNERTServer*>(this->parent()->parent())->m_pFiffStreamServer;
        if(t_pFiffStreamServer)
        {
            bool t_isInt;
            qint32 t_id = p_sRawId.toInt(&t_isInt);

            if(t_isInt && t_pFiffStreamServer->m_qClientList.contains(t_id))
            {
                p_iParsedId = t_id;
            }
            else
            {
                QMap<qint32, FiffStreamThread*>::iterator i;
                for (i = t_pFiffStreamServer->m_qClientList.begin(); i != t_pFiffStreamServer->m_qClientList.end(); ++i)
                {
                    if(i.value()->getAlias().compare(p_sRawId) == 0)
                    {
                        p_iParsedId = i.key();
                        QString str = QString("\tconvert alias '%1' => %2\r\n").arg(i.value()->getAlias()).arg(i.key());
                        t_blockCmdIdInfo.append(str);
                        break;
                    }
                }
            }
         }
    }

    if(p_iParsedId != -1)
    {
        QString str = QString("\tselect fiff stream client %1\r\n").arg(p_iParsedId);
        t_blockCmdIdInfo.append(str);
    }
    else
    {
        t_blockCmdIdInfo.append("\twarning: requested fiff stream client not available\r\n\n");
    }

    return t_blockCmdIdInfo;
}


//*************************************************************************************************************

void CommandThread::run()
{
    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    }
    else
    {
        printf("Command client connection accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket.peerPort());
    }

    QDataStream t_FiffStreamIn(&t_qTcpSocket);
    t_FiffStreamIn.setVersion(QDataStream::Qt_5_0);

    qint64 t_iMaxBufSize = 1024;

    while(true)
    {

        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable() > 0)
        {
            QByteArray t_qByteArrayRaw = t_qTcpSocket.readLine(t_iMaxBufSize);
            QString t_sCommand = QString(t_qByteArrayRaw).simplified();

            if(!t_sCommand.isEmpty())
                parseCommand(t_qTcpSocket, t_sCommand);

//            std::cout << t_sCommand.toUtf8().constData();
        }


//        if (blockSize == 0) {
//            if (t_FiffStreamIn->bytesAvailable() < (int)sizeof(quint16))
//                return;

//            in >> blockSize;
//        }

//        if (tcpSocket->bytesAvailable() < blockSize)
//            return;

//        QByteArray block;
//        QDataStream out(&block, QIODevice::WriteOnly);
//        out.setVersion(QDataStream::Qt_4_0);
//        out << (quint16)0;
//        out << text;
//        out.device()->seek(0);
//        out << (quint16)(block.size() - sizeof(quint16));

//        tcpSocket.write(block);

//        tcpSocket.disconnectFromHost();
//        tcpSocket.waitForDisconnected();

    }

}
