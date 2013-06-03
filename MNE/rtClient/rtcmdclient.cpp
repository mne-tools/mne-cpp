//=============================================================================================================
/**
 * @file     rtcmdclient.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           To Be continued...
 *
 * @version  1.0
 * @date     July, 2012; May, 2013 (modified by Limin Sun)
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Limin Sun and Matti Hamalainen. All rights reserved.
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

#include <QDebug>

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

int RtCmdClient::MGH_LM_Byte2Int(QByteArray b)
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

QByteArray RtCmdClient::MGH_LM_Int2Byte(int a)
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

//*************************************************************************************************************

QString RtCmdClient::RecvData()
{
    // Receive response
    bool respComplete = false;
    QByteArray t_qByteArrayRaw;

    bool lenflag = true;
    do {
        qint32 cmdlen;
        this->waitForReadyRead(100);
        if(this->bytesAvailable()>4 && lenflag){
            QByteArray clen = this->read(4);
            cmdlen = MGH_LM_Byte2Int(clen);
            lenflag = false;
        }
        if(this->bytesAvailable()>=cmdlen && !lenflag)
        {
            t_qByteArrayRaw = this->read(cmdlen);
            respComplete = true;
        }

    }while (!respComplete);


    return QString(t_qByteArrayRaw);

}

//*************************************************************************************************************

void RtCmdClient::SendData(QString t_sCommand)
{
    qint32 t_iBlockSize = t_sCommand.size();
    QByteArray Scmd = MGH_LM_Int2Byte(t_iBlockSize);
    this->write(Scmd);
    this->write(t_sCommand.toUtf8().constData(), t_sCommand.size());
    this->waitForBytesWritten();
}

//*************************************************************************************************************

QString RtCmdClient::sendCLICommandFLL(const QString &p_sCommand)
{
    QString t_sCommand = QString("%1\n").arg(p_sCommand);
    QString p_sReply;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "Write FLL command: " << t_sCommand;
        SendData(t_sCommand);
        p_sReply = RtCmdClient::RecvData();
    }

    return p_sReply;
}


//*************************************************************************************************************

QString RtCmdClient::sendCLICommand(const QString &p_sCommand)
{
    QString t_sCommand = QString("%1\n").arg(p_sCommand);
    QString p_sReply;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "Write command: " << t_sCommand;
        SendData(t_sCommand);

        p_sReply = RtCmdClient::RecvData();
    }
    return p_sReply;
}

//*************************************************************************************************************

void RtCmdClient::sendCommandJSON(const Command &p_command)
{
    const QString t_sCommand = QString("{\"commands\":{%1}}\n").arg(
            p_command.toStringReadySend());

    QString t_sReply;

    qDebug() << "JSON Command Start " << t_sCommand;

    if (this->state() == QAbstractSocket::ConnectedState)
    {
        qDebug() << "JSON Request: " << t_sCommand;

        // Send request
        SendData(t_sCommand);

        // Receive response
        t_sReply = RtCmdClient::RecvData();

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


//*************************************************************************************************************

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
        qDebug() << t_jsonDocumentOrigin;//"Received Commands" << m_commandManager.commandMap().keys();

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


//*************************************************************************************************************

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
    {
        m_commandManager.insert(t_jsonDocumentOrigin);
//        qDebug() << "Received Commands" << m_commandManager.commandMap().keys();
    }
    else
    {
        qCritical() << "Unable to parse JSON response: " << error.errorString();
    }
}


//*************************************************************************************************************

//QMap<qint32, QString>
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
            qWarning("Warning: CommandMap contains command %s already. Insertion skipped.\n", it.key().toLatin1().constData());

        //if connector is active indicate it
        if(it.value().toObject().value(QString("active")).toBool())
            p_iActiveId = id;
    }

    return p_iActiveId;
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
