//=============================================================================================================
/**
 * @file     commandthread.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     Definition of the CommandThread Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandthread.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

#define USENEWSERVER 1

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandThread::CommandThread(int socketDescriptor, qint32 p_iId, QObject *parent)
: QThread(parent)
, socketDescriptor(socketDescriptor)
, m_bIsRunning(false)
, m_iThreadID(p_iId)
{
}

//=============================================================================================================

CommandThread::~CommandThread()
{
    m_bIsRunning = false;
    QThread::wait();
}

//=============================================================================================================

void CommandThread::attachCommandReply(QString p_blockReply, qint32 p_iID)
{
    qDebug() << "CommandThread::attachCommandReply";
    if(p_iID == m_iThreadID)
    {
        m_qMutex.lock();
        m_qSendData = p_blockReply;
        m_qMutex.unlock();
    }
}

//=============================================================================================================

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
    t_FiffStreamIn.setVersion(QDataStream::Qt_5_1);

#ifndef USENEWSERVER
    qint64 t_iMaxBufSize = 1024;
#endif

    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState && m_bIsRunning)
    {
#ifdef USENEWSERVER
        //
        // Write available data
        //
        if(m_qSendData.size() > 0)
        {
            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_5_1);
            out << (quint16)0;
            m_qMutex.lock();
            out << m_qSendData;
            m_qSendData.clear();
            m_qMutex.unlock();
            out.device()->seek(0);
            out << (quint16)(block.size() - sizeof(quint16));

            t_qTcpSocket.write(block);
            t_qTcpSocket.waitForBytesWritten();
        }

        //
        // Read: Wait 100ms for incomming tag header, read and continue
        //

        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable() >= (int)sizeof(quint16))
        {
            quint16 blockSize = 0;

            bool respComplete = false;
            t_FiffStreamIn >> blockSize;

            while(!respComplete && blockSize < 65000)//Sanity Check -> allowed maximal blocksize is 65.000
            {
                if (t_qTcpSocket.bytesAvailable() >= blockSize)
                {
                    QString t_sCommand;

                    t_FiffStreamIn >> t_sCommand;

                    t_sCommand = t_sCommand.simplified();

                    //
                    // Parse command
                    //
                    if(!t_sCommand.isEmpty())
                        emit newCommand(t_sCommand, m_iThreadID);

                    respComplete = true;
                }
            }
        }

#else
        //
        // Write available data
        //
        if(m_qSendBlock.size() > 0)
        {
            qint32 t_iBlockSize = m_qSendBlock.size();
            m_qMutex.lock();
            qint32 t_iBytesWritten = t_qTcpSocket.write(m_qSendBlock);
            t_qTcpSocket.waitForBytesWritten();
            if(t_iBytesWritten == t_iBlockSize)
                m_qSendBlock.clear();
            else
                m_qSendBlock = m_qSendBlock.mid(t_iBytesWritten, t_iBlockSize-t_iBytesWritten);
            m_qMutex.unlock();
        }

        //
        // Read: Wait 100ms for incomming tag header, read and continue
        //
        //ToDo its not the best solution in terms of receiving the command for sure
        t_qTcpSocket.waitForReadyRead(100);

        if (t_qTcpSocket.bytesAvailable() > 0 && t_qTcpSocket.canReadLine())
        {
            QByteArray t_qByteArrayRaw = t_qTcpSocket.readLine(t_iMaxBufSize);
            QString t_sCommand = QString(t_qByteArrayRaw).simplified();

            //
            // Parse command
            //
            if(!t_sCommand.isEmpty())
                emit newCommand(t_sCommand, m_iThreadID);
        }
        else if(t_qTcpSocket.bytesAvailable() > t_iMaxBufSize)
        {
            t_qTcpSocket.readAll();//readAll that QTcpSocket is empty again -> prevent overflow
        }
#endif
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
