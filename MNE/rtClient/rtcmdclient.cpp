//=============================================================================================================
/**
 * @file     rtcmdclient.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           To Be continued...
 *
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
 * @brief     implementation of the RtCmdClient Class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "rtcmdclient.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
#include <QThread>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCLIENTLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCmdClient::RtCmdClient(QObject *parent) :
        QTcpSocket(parent)
{
    QObject::connect(&m_commandManager, &CommandManager::triggered, this,
            &RtCmdClient::sendCommandJSON);
}

//*************************************************************************************************************

void RtCmdClient::connectToHost(QString &p_sRtServerHostName)
{
    QTcpSocket::connectToHost(p_sRtServerHostName, 4217);
}

//*************************************************************************************************************

QString RtCmdClient::sendCLICommand(const QString &p_sCommand)
{
    QString t_sCommand = QString("%1\n").arg(p_sCommand);
    QString p_sReply;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        this->write(t_sCommand.toUtf8().constData(), t_sCommand.size());
        this->waitForBytesWritten();

        //thats not the most elegant way
        this->waitForReadyRead(1000);
        QByteArray t_qByteArrayRaw;
        // TODO(cpieloth): We need a break condition e.g. last byte == \0 or \n
        // Large responses can be split to more than one packet which could be a problem on big network latencies.
        while (this->bytesAvailable() > 0 && this->canReadLine())
            t_qByteArrayRaw += this->readAll();

        p_sReply = QString(t_qByteArrayRaw);
    }
    return p_sReply;
}

//*************************************************************************************************************

void RtCmdClient::sendCommandJSON(const Command &p_command)
{
    const QString t_sCommand = QString("{\"commands\":{%1}}\n").arg(
            p_command.toStringReadySend());

    QString t_sReply;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "Request: " << t_sCommand;

        // Send request
        this->write(t_sCommand.toUtf8().constData(), t_sCommand.size());
        this->waitForBytesWritten();

        // Receive response
        bool respComplete = false;
        QByteArray t_qByteArrayRaw;
        do
        {
            if (this->waitForReadyRead(100))
            {
                t_qByteArrayRaw += this->readAll();
                // We need a break condition,
                // because we do not have a stop character and do not know how many bytes to receive.
                respComplete = t_qByteArrayRaw.count('{')
                        == t_qByteArrayRaw.count('}');
            }
            qDebug() << "Response: " << t_qByteArrayRaw.size() << " bytes";
        } while (!respComplete);
        t_sReply = QString(t_qByteArrayRaw);
    }
    else
    {
        qWarning() << "Request was not send, because client is not connected!";
    }

    m_qMutex.lock();
    m_sAvailableData.append(t_sReply);
    m_qMutex.unlock();

    emit response(t_sReply);
}

//*************************************************************************************************************

void RtCmdClient::requestCommands()
{
    const QString help("help");
    const QString description("");
    const Command cmdHelp(help, description);
    this->sendCommandJSON(cmdHelp);

    m_qMutex.lock();
    QByteArray t_sJsonCommands = m_sAvailableData.toUtf8();
    m_qMutex.unlock();
    QJsonParseError error;
    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(
            t_sJsonCommands, &error);
    if (error.error == QJsonParseError::NoError)
    {
        m_commandManager.insert(t_jsonDocumentOrigin);
        qDebug() << "Received Commands" << m_commandManager.commandMap().keys();
    }
    else
    {
        qCritical() << "Unable to parse JSON response: " << error.errorString();
    }
}

////*************************************************************************************************************

//void RtCmdClient::requestMeasInfo(qint32 p_id)
//{
//    QString t_sCommand = QString("measinfo %1").arg(p_id);
//    this->sendCommand(t_sCommand);
//}

////*************************************************************************************************************

//void RtCmdClient::requestMeasInfo(const QString &p_Alias)
//{
//    QString t_sCommand = QString("measinfo %1").arg(p_Alias);
//    this->sendCommand(t_sCommand);
//}

////*************************************************************************************************************

//void RtCmdClient::requestMeas(qint32 p_id)
//{
//    QString t_sCommand = QString("start %1").arg(p_id);
//    this->sendCommand(t_sCommand);
//}

////*************************************************************************************************************

//void RtCmdClient::requestMeas(QString p_Alias)
//{
//    QString t_sCommand = QString("start %1").arg(p_Alias);
//    this->sendCommand(t_sCommand);
//}

////*************************************************************************************************************

//void RtCmdClient::stopAll()
//{
//    QString t_sCommand = QString("stop-all");
//    this->sendCommand(t_sCommand);
//}

//*************************************************************************************************************

bool RtCmdClient::waitForDataAvailable(qint32 msecs) const
{
    if (m_sAvailableData.size() > 0)
        return true;

    qint64 t_msecsStart = QDateTime::currentMSecsSinceEpoch();

    while (msecs == -1
            || (qint64) msecs
                    < QDateTime::currentMSecsSinceEpoch() - t_msecsStart)
    {
        QThread::msleep(5);
        if (m_sAvailableData.size() > 0)
            return true;
    }
    return false;
}

//*************************************************************************************************************

Command& RtCmdClient::operator[](const QString &key)
{
    return m_commandManager[key];
}

//*************************************************************************************************************

const Command RtCmdClient::operator[](const QString &key) const
{
    return m_commandManager[key];
}
