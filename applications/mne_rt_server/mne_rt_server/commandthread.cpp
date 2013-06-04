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
* @brief     implementation of the CommandThread Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandthread.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandThread::CommandThread(int socketDescriptor, qint32 p_iId, QObject *parent)
: QThread(parent)
, socketDescriptor(socketDescriptor)
, m_bIsRunning(false)
, m_iThreadID(p_iId)
{
  //  connect(&t_qTcpSocket,SIGNAL(readyRead()),this,SLOT(SocketReadProc()));

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

}


//*************************************************************************************************************

CommandThread::~CommandThread()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void CommandThread::attachCommandReply(QByteArray p_blockReply, qint32 p_iID)
{
    if(p_iID == m_iThreadID)
    {
        m_qMutex.lock();
        m_qSendBlock.append(p_blockReply);
        //
        // Write available data
        //
        if(m_qSendBlock.size() > 0)
        {
            qDebug()<<"SLM1 Reply"<<m_qSendBlock;
            qint32 t_iBlockSize = m_qSendBlock.size();
            QByteArray Scmd = MGH_LM_Int2Byte(t_iBlockSize);
            t_qTcpSocket.write(Scmd);
            t_qTcpSocket.write(m_qSendBlock);
            //qint32 t_iBytesWritten = t_qTcpSocket.write(m_qSendBlock);
            t_qTcpSocket.waitForBytesWritten();
            m_qSendBlock.clear();
        }
        m_qMutex.unlock();

    }
}

//*************************************************************************************************************

int CommandThread::MGH_LM_Byte2Int(QByteArray b)
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

//*************************************************************************************************************

QByteArray CommandThread::MGH_LM_Int2Byte(int a)
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

void CommandThread::SocketReadProc()
{
    qint64 t_iMaxBufSize = 1024;

    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {

        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable()> 4 && t_qTcpSocket.canReadLine())
        {
            t_qTcpSocket.read(4);
            QByteArray t_qByteArrayRaw = t_qTcpSocket.readLine(t_iMaxBufSize);
            QString t_sCommand = QString(t_qByteArrayRaw).simplified();
            qDebug()<<"SLM receive command"<<t_sCommand;
            //
            // Parse command
            //
            if(!t_sCommand.isEmpty())
                emit newCommand(t_sCommand, m_iThreadID);

        }
    }
else{
    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
    }
}

//*************************************************************************************************************

void CommandThread::run()
{
    m_bIsRunning = true;

    qint64 t_iMaxBufSize = 1024;

    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {

        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable()> 4 && t_qTcpSocket.canReadLine())
        {
            t_qTcpSocket.read(4);
            QByteArray t_qByteArrayRaw = t_qTcpSocket.readLine(t_iMaxBufSize);
            QString t_sCommand = QString(t_qByteArrayRaw).simplified();
            qDebug()<<"SLM receive command"<<t_sCommand;
            //
            // Parse command
            //
            if(!t_sCommand.isEmpty())
                emit newCommand(t_sCommand, m_iThreadID);

        }
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();

}
