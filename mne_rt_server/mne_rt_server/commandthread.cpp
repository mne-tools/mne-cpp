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

#include "commandserver.h"

#include "commandthread.h"


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

bool CommandThread::parseCommand(QTcpSocket& p_qTcpSocket, QString& p_sCommand)
{
    QStringList t_qCommandList = p_sCommand.split(" ");

    bool success = false;
    QByteArray t_blockClientList;

    CommandServer* t_pCommandServer = qobject_cast<CommandServer*>(this->parent());
    success = t_pCommandServer->parseCommand(t_qCommandList, t_blockClientList);

    //print
    std::cout << t_blockClientList.data();
    //send
    p_qTcpSocket.write(t_blockClientList);
    p_qTcpSocket.waitForBytesWritten();

    return success;
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
            {
                parseCommand(t_qTcpSocket, t_sCommand);
            }
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
