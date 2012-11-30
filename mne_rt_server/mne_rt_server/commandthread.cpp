//=============================================================================================================
/**
* @file     commandthread.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
#include "connectormanager.h"
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
, m_bIsRunning(false)
{

}


//*************************************************************************************************************

CommandThread::~CommandThread()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

QByteArray CommandThread::availableCommands() const
{
    QByteArray t_blockCmdInfoList;
    t_blockCmdInfoList.append("\tclist\t\t\tprints and sends all available FiffStreamClients\r\n");
    t_blockCmdInfoList.append("\tmeasinfo [ID/Alias]\tsends the measurement info to the specified\r\n\t\t\t\tFiffStreamClient\r\n");
    t_blockCmdInfoList.append("\tmeas     [ID/Alias]\tadds specified FiffStreamClient to raw data\r\n\t\t\t\tbuffer receivers. If acquisition is not already strated, it is triggered.\r\n");
    t_blockCmdInfoList.append("\tstop     [ID/Alias]\tremoves specified FiffStreamClient from raw\r\n\t\t\t\tdata buffer receivers.\r\n");
    t_blockCmdInfoList.append("\tstop-all\t\t\tstops the whole acquisition process.\r\n");

    t_blockCmdInfoList.append("\n\tconlist\t\t\tprints and sends all available connectors\r\n");
    t_blockCmdInfoList.append("\tselcon   [ConID]\tselects a new connector, if a measurement is running it will be stopped.\r\n");

    t_blockCmdInfoList.append("\n\thelp\t\t\tprints and sends this list\r\n");

    t_blockCmdInfoList.append("\n\tclose\t\t\tcloses mne_rt_server\r\n\n");

    //Connector Commands
    IConnector* t_pCurrentConnector = qobject_cast<MNERTServer*>(this->parent()->parent())->m_pConnectorManager->getActiveConnector();
    if(t_pCurrentConnector)
        t_blockCmdInfoList.append(t_pCurrentConnector->availableCommands());
    else
        t_blockCmdInfoList.append("No connector commands available - no connector active.\r\n");

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
        printf("clist\n");
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
            t_blockClientList.append("\n");
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
        if(t_qCommandList.size() > 1)
        {
            qint32 t_id = -1;
            t_blockClientList.append(parseToId(t_qCommandList[1],t_id));

            printf("measinfo %d\r\n", t_id);

            if(t_id != -1)
            {
                emit requestMeasInfo(t_id);

                QString str = QString("\tsend measurement info to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                t_blockClientList.append(str);
                success = true;
            }
        }
    }
    else if(t_qCommandList[0].compare("meas",Qt::CaseInsensitive) == 0)
    {
        //
        // meas
        //

        if(t_qCommandList.size() > 1)
        {
            qint32 t_id = -1;
            t_blockClientList.append(parseToId(t_qCommandList[1],t_id));

            printf("meas %d\n", t_id);

            if(t_id != -1)
            {
                emit requestStartMeas(t_id);

                QString str = QString("\tsend measurement raw buffer to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                t_blockClientList.append(str);
            }
        }
        success = true;
    }
    else if(t_qCommandList[0].compare("stop",Qt::CaseInsensitive) == 0)
    {
        //
        // stop
        //

        if(t_qCommandList.size() > 1)
        {
            qint32 t_id = -1;
            t_blockClientList.append(parseToId(t_qCommandList[1],t_id));

            printf("stop %d\n", t_id);

            if(t_id != -1)
            {
                emit requestStopMeas(t_id);

                QString str = QString("\tstop FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                t_blockClientList.append(str);
            }
        }
        success = true;
    }
    else if(t_qCommandList[0].compare("stop-all",Qt::CaseInsensitive) == 0)
    {
        //
        // stop-all
        //
        emit requestStopMeas(-1);
        emit requestStopConnector();

        QString str = QString("\tstop all FiffStreamClients and Connectors\r\n\n");
        t_blockClientList.append(str);

        success = true;
    }
    else if(t_qCommandList[0].compare("help",Qt::CaseInsensitive) == 0)
    {
        //
        // Help
        //
        printf("help\n");
        t_blockClientList.append(CommandThread::availableCommands());
        success = true;
    }
    else if(t_qCommandList[0].compare("close",Qt::CaseInsensitive) == 0)
    {
        //
        // Closes mne_rt_server
        //
        printf("close\n");
        emit qobject_cast<MNERTServer*>(this->parent()->parent())->closeServer();
        success = true;
    }
    else if(qobject_cast<MNERTServer*>(this->parent()->parent())->m_pConnectorManager->parseConnectorCommand(t_qCommandList, t_blockClientList))
    {
        //
        // Connector/ConnectorManager Command
        //
        success = true;
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
        QString str = QString("\tselect FiffStreamClient %1\r\n").arg(p_iParsedId);
        t_blockCmdIdInfo.append(str);
    }
    else
    {
        t_blockCmdIdInfo.append("\twarning: requested FiffStreamClient not available\r\n\n");
    }

    return t_blockCmdIdInfo;
}


//*************************************************************************************************************

void CommandThread::run()
{
    m_bIsRunning = true;

    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    }
    else
    {
        printf("CommandClient connection accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket.peerPort());
    }

    QDataStream t_FiffStreamIn(&t_qTcpSocket);

    qint64 t_iMaxBufSize = 1024;

    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {
        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable() > 0 && t_qTcpSocket.canReadLine())
        {
            QByteArray t_qByteArrayRaw = t_qTcpSocket.readLine(t_iMaxBufSize);
            QString t_sCommand = QString(t_qByteArrayRaw).simplified();

            //
            // Parse command & send answer
            //
            if(!t_sCommand.isEmpty())
                parseCommand(t_qTcpSocket, t_sCommand);
        }
        else if(t_qTcpSocket.bytesAvailable() > t_iMaxBufSize)
        {
            t_qTcpSocket.readAll();//readAll that QTcpSocket is empty again -> prevent overflow
        }
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
