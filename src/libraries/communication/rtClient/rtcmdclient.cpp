//=============================================================================================================
/**
 * @file     rtcmdclient.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the RtCmdClient Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtcmdclient.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
#include <QThread>

#include <iostream>

#define USENEW 1

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COMMUNICATIONLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtCmdClient::RtCmdClient(QObject *parent)
: QTcpSocket(parent)
{
    QObject::connect(&m_commandManager, &CommandManager::triggered, this,
            &RtCmdClient::sendCommandJSON);
}

//=============================================================================================================

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

//=============================================================================================================

void RtCmdClient::sendCommandJSON(const Command &p_command)
{
    const QString t_sCommand = QString("{\"commands\":{%1}}\n").arg(p_command.toStringReadySend());

    QString t_sReply;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        // Send request
#ifdef USENEW
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_1);

        out << static_cast<quint16>(0);
        out << t_sCommand;
        out.device()->seek(0);
        out << static_cast<quint16>(static_cast<unsigned long>(block.size()) - sizeof(quint16));

        this->write(block);
        this->waitForBytesWritten();

        // Receive response
        QDataStream in(this);
        in.setVersion(QDataStream::Qt_5_1);

        quint16 blockSize = 0;

        bool respComplete = false;

        do
        {
            this->waitForReadyRead(100);

            if (blockSize == 0)
            {
                if (this->bytesAvailable() >= (int)sizeof(quint16))
                    in >> blockSize;
            }
            else if(this->bytesAvailable() >= blockSize)
            {
                in >> t_sReply;
                respComplete = true;
            }
        } while (!respComplete && blockSize < 65000);//Sanity Check -> allowed maximal blocksize is 65.000
#else
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
#endif
    }
    else
    {
        qWarning() << "Request was not send, because client is not connected!";
    }

    m_qMutex.lock();
//    m_sAvailableData.append(t_sReply); //ToDo check this
    m_sAvailableData = t_sReply;
    m_qMutex.unlock();

    emit response(t_sReply);
}

//=============================================================================================================

qint32 RtCmdClient::requestBufsize()
{
    //Send
    m_commandManager["getbufsize"].send();

    //Receive
    m_qMutex.lock();
    QByteArray t_sJsonCommands = m_sAvailableData.toUtf8();
    m_qMutex.unlock();

    //Parse
    QJsonParseError error;
    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(t_sJsonCommands, &error);

    if (error.error == QJsonParseError::NoError)
    {
//        qDebug() << t_jsonDocumentOrigin;//"Received Commands" << m_commandManager.commandMap().keys();

        //Switch to command object
        if(t_jsonDocumentOrigin.isObject() && t_jsonDocumentOrigin.object().value(QString("bufsize")) != QJsonValue::Undefined)
        {
            qint32 size = (qint32)t_jsonDocumentOrigin.object().value(QString("bufsize")).toDouble();
            return size;
        }
    }

    qCritical() << "Unable to parse JSON response: " << error.errorString();
    return -1;
}

//=============================================================================================================

void RtCmdClient::requestCommands()
{
    //No commands are present -> thats why help has to be send using a self created command
    const QString help("help");
    const QString description("");
    const Command cmdHelp(help, description);
    this->sendCommandJSON(cmdHelp);

    //Clear Commands
    m_commandManager.clear();

    //Receive
    m_qMutex.lock();
    QByteArray t_sJsonCommands = m_sAvailableData.toUtf8();
    m_qMutex.unlock();

    //Parse
    QJsonParseError error;
    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(
            t_sJsonCommands, &error);

    if (error.error == QJsonParseError::NoError)
        m_commandManager.insert(t_jsonDocumentOrigin);
    else
        qCritical() << "Unable to parse JSON response: " << error.errorString();
}

//=============================================================================================================

qint32 RtCmdClient::requestConnectors(QMap<qint32, QString> &p_qMapConnectors)
{
    //Send
    m_commandManager["conlist"].send();

    //Receive
    m_qMutex.lock();
    QByteArray t_sJsonConnectors = m_sAvailableData.toUtf8();
    m_qMutex.unlock();

    //Parse
    QJsonParseError error;
    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(
            t_sJsonConnectors, &error);

    QJsonObject t_jsonObjectConnectors;

    //Switch to command object
    if(t_jsonDocumentOrigin.isObject() && t_jsonDocumentOrigin.object().value(QString("connectors")) != QJsonValue::Undefined)
        t_jsonObjectConnectors = t_jsonDocumentOrigin.object().value(QString("connectors")).toObject();

    //inits
    qint32 p_iActiveId = -1;
    p_qMapConnectors.clear();

    //insert connectors
    QJsonObject::Iterator it;
    for(it = t_jsonObjectConnectors.begin(); it != t_jsonObjectConnectors.end(); ++it)
    {
        QString t_qConnectorName = it.key();

        qint32 id = it.value().toObject().value(QString("id")).toDouble();

        if(!p_qMapConnectors.contains(id))
            p_qMapConnectors.insert(id, t_qConnectorName);
        else
            qWarning("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toUtf8().constData());

        //if connector is active indicate it
        if(it.value().toObject().value(QString("active")).toBool())
            p_iActiveId = id;
    }

    return p_iActiveId;
}

////=============================================================================================================

//void RtCmdClient::requestMeasInfo(qint32 p_id)
//{
//    QString t_sCommand = QString("measinfo %1").arg(p_id);
//    this->sendCommand(t_sCommand);
//}

////=============================================================================================================

//void RtCmdClient::requestMeasInfo(const QString &p_Alias)
//{
//    QString t_sCommand = QString("measinfo %1").arg(p_Alias);
//    this->sendCommand(t_sCommand);
//}

////=============================================================================================================

//void RtCmdClient::requestMeas(qint32 p_id)
//{
//    QString t_sCommand = QString("start %1").arg(p_id);
//    this->sendCommand(t_sCommand);
//}

////=============================================================================================================

//void RtCmdClient::requestMeas(QString p_Alias)
//{
//    QString t_sCommand = QString("start %1").arg(p_Alias);
//    this->sendCommand(t_sCommand);
//}

////=============================================================================================================

//void RtCmdClient::stopAll()
//{
//    QString t_sCommand = QString("stop-all");
//    this->sendCommand(t_sCommand);
//}

//=============================================================================================================

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

//=============================================================================================================

Command& RtCmdClient::operator[](const QString &key)
{
    return m_commandManager[key];
}

//=============================================================================================================

const Command RtCmdClient::operator[](const QString &key) const
{
    return m_commandManager[key];
}
