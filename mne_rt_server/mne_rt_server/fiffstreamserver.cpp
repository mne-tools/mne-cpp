//=============================================================================================================
/**
* @file     fiffstreamserver.cpp
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
* @brief     implementation of the FiffStreamServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdlib.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamServer::FiffStreamServer(QObject *parent)
: QTcpServer(parent)
, m_iNextClientId(0)
{
    init();
}


//*************************************************************************************************************

FiffStreamServer::~FiffStreamServer()
{
    emit closeFiffStreamServer();
//    clearClients();
}


////*************************************************************************************************************

//QByteArray FiffStreamServer::availableCommands()
//{
//    QByteArray t_blockCmdInfoList;
//    t_blockCmdInfoList.append("\tclist\t\t\tprints and sends all available FiffStreamClients\r\n");
//    return t_blockCmdInfoList;
//}


//*************************************************************************************************************

void FiffStreamServer::comClist(Command p_command)
{
    QString t_sOutput("");
    t_sOutput.append("\tID\tAlias\r\n");
    QMap<qint32, FiffStreamThread*>::iterator i;
    for (i = this->m_qClientList.begin(); i != this->m_qClientList.end(); ++i)
    {
        QString str = QString("\t%1\t%2\r\n").arg(i.key()).arg(i.value()->getAlias());
        t_sOutput.append(str);
    }
    t_sOutput.append("\n");
    m_commandManager["clist"].reply(t_sOutput);
}


//*************************************************************************************************************

void FiffStreamServer::comMeasinfo(Command p_command)
{
    qint32 t_id = -1;
//            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

    t_id = p_command.pValues()[0].toInt();

    if(t_id != -1)
    {
        emit requestMeasInfo(t_id);//requestMeasInfo(t_id);

        QString str = QString("\tsend measurement info to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
        m_commandManager["measinfo"].reply(str);
    }
}


//*************************************************************************************************************

void FiffStreamServer::comStart(Command p_command)
{
    qint32 t_id = -1;
    QString t_sOutput("");
    t_sOutput.append(parseToId(p_command.pValues()[0].toString(),t_id));

    if(t_id != -1)
    {
        emit startMeasFiffStreamClient(t_id);

        QString str = QString("\tFiffStreamClient (ID: %1) is now set to accept raw buffers\r\n\n").arg(t_id);
        t_sOutput.append(str);
    }
    m_commandManager["start"].reply(t_sOutput);
}


//*************************************************************************************************************

void FiffStreamServer::comStop(Command p_command)
{
    qint32 t_id = -1;
    QString t_sOutput("");
    t_sOutput.append(parseToId(p_command.pValues()[0].toString(),t_id));

    if(t_id != -1)
    {
        emit stopMeasFiffStreamClient(t_id);//emit requestStopMeas(t_id);

        QString str = QString("\tstop FiffStreamClient (ID: %1) from receiving raw Buffers.\r\n\n").arg(t_id);
        t_sOutput.append(str);
    }
    m_commandManager["stop"].reply(t_sOutput);
}


//*************************************************************************************************************

void FiffStreamServer::comStopAll(Command p_command)
{
    emit stopMeasFiffStreamClient(-1);
    QString str = QString("\tstop all FiffStreamClients from receiving raw buffers\r\n\n");
    m_commandManager["stop"].reply(str);
}


//*************************************************************************************************************

void FiffStreamServer::init()
{
    QString t_sJsonCommand =
                    "{"
                    "   \"commands\": {"
                    "       \"clist\": {"
                    "           \"description\": \"Prints and sends all available FiffStreamClients.\","
                    "           \"parameters\": {}"
                    "        },"
                    "       \"measinfo\": {"
                    "           \"description\": \"Sends the measurement info to the specified FiffStreamClient.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "       },"
                    "       \"start\": {"
                    "           \"description\": \"Adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already started, it is triggered.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "        },"
                    "       \"stop\": {"
                    "           \"description\": \"Removes specified FiffStreamClient from raw data buffer receivers.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "        },"
                    "       \"stop-all\": {"
                    "           \"description\": \"Stops the whole acquisition process.\","
                    "           \"parameters\": {}"
                    "        }"
                    "    }"
                    "}";

    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(t_sJsonCommand.toLatin1());
    m_commandManager.insert(t_jsonDocumentOrigin);

    //Connect slots
    m_commandManager.connectSlot(QString("clist"), this, &FiffStreamServer::comClist);
    m_commandManager.connectSlot(QString("measinfo"), this, &FiffStreamServer::comMeasinfo);
    m_commandManager.connectSlot(QString("start"), this, &FiffStreamServer::comStart);
    m_commandManager.connectSlot(QString("stop"), this, &FiffStreamServer::comStop);
    m_commandManager.connectSlot(QString("stop-all"), this, &FiffStreamServer::comStopAll);
}


////*************************************************************************************************************

//bool FiffStreamServer::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
//{
//    bool success = false;

//    if(p_sListCommand[0].compare("clist",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // client list
//        //
//        printf("clist\n");

//        p_blockOutputInfo.append("\tID\tAlias\r\n");
//        QMap<qint32, FiffStreamThread*>::iterator i;
//        for (i = this->m_qClientList.begin(); i != this->m_qClientList.end(); ++i)
//        {
//            QString str = QString("\t%1\t%2\r\n").arg(i.key()).arg(i.value()->getAlias());
//            p_blockOutputInfo.append(str);
//        }
//        p_blockOutputInfo.append("\n");
//        success = true;
//    }
//    else if(p_sListCommand[0].compare("measinfo",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // Measurement Info
//        //
//        if(p_sListCommand.size() > 1)
//        {
//            qint32 t_id = -1;
////            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

//            bool t_isInt;
//            t_id = p_sListCommand[1].toInt(&t_isInt);

//            if(t_isInt)// ToDo Check whether ID is correct --> move this parsing to fiff stream server
//            {
//                printf("measinfo %d\r\n", t_id);
//            }
//            if(t_id != -1)
//            {
//                emit requestMeasInfo(t_id);//requestMeasInfo(t_id);

//                QString str = QString("\tsend measurement info to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
//                p_blockOutputInfo.append(str);
//                success = true;
//            }
//        }
//    }
//    else if(p_sListCommand[0].compare("meas",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // meas
//        //
//        if(p_sListCommand.size() > 1)
//        {
//            qint32 t_id = -1;
//            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

//            printf("meas %d\n", t_id);
//            if(t_id != -1)
//            {
//                emit startMeasFiffStreamClient(t_id);

//                QString str = QString("\tFiffStreamClient (ID: %1) is now set to accept raw buffers\r\n\n").arg(t_id);
//                p_blockOutputInfo.append(str);
//            }
//        }
//        success = true;
//    }
//    else if(p_sListCommand[0].compare("stop",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // stop
//        //

//        if(p_sListCommand.size() > 1)
//        {
//            qint32 t_id = -1;
//            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

//            printf("stop %d\n", t_id);

//            if(t_id != -1)
//            {
//                emit stopMeasFiffStreamClient(t_id);//emit requestStopMeas(t_id);

//                QString str = QString("\tstop FiffStreamClient (ID: %1) from receiving raw Buffers.\r\n\n").arg(t_id);
//                p_blockOutputInfo.append(str);
//            }
//        }
//        success = true;
//    }
//    else if(p_sListCommand[0].compare("stop-all",Qt::CaseInsensitive) == 0)
//    {
//        //
//        // stop-all
//        //
//        emit stopMeasFiffStreamClient(-1);

//        QString str = QString("\tstop all FiffStreamClients from receiving raw buffers\r\n\n");
//        p_blockOutputInfo.append(str);

//        success = true;
//    }

//    return success;
//}


//*************************************************************************************************************

QByteArray FiffStreamServer::parseToId(QString& p_sRawId, qint32& p_iParsedId)
{
    p_iParsedId = -1;
    QByteArray t_blockCmdIdInfo;
    if(!p_sRawId.isEmpty())
    {
        bool t_isInt;
        qint32 t_id = p_sRawId.toInt(&t_isInt);

        if(t_isInt && this->m_qClientList.contains(t_id))
        {
            p_iParsedId = t_id;
        }
        else
        {
            QMap<qint32, FiffStreamThread*>::iterator i;
            for (i = this->m_qClientList.begin(); i != this->m_qClientList.end(); ++i)
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

//void FiffStreamServer::clearClients()
//{
//    QMap<qint32, FiffStreamThread*>::const_iterator i = m_qClientList.constBegin();
//    while (i != m_qClientList.constEnd()) {
//        if(i.value())
//            delete i.value();
//        ++i;
//    }
//    m_qClientList.clear();
//}


//*************************************************************************************************************

void FiffStreamServer::forwardMeasInfo(qint32 ID, FiffInfo::SDPtr p_FiffInfo)
{
    emit remitMeasInfo(ID, p_FiffInfo);
}


//*************************************************************************************************************
//ToDo increase preformance --> try inline
void FiffStreamServer::forwardRawBuffer(Eigen::MatrixXf m_matRawData)
{
    emit remitRawBuffer(m_matRawData);
}



//*************************************************************************************************************

void FiffStreamServer::incomingConnection(qintptr socketDescriptor)
{
    FiffStreamThread* t_pStreamThread = new FiffStreamThread(m_iNextClientId, socketDescriptor, this);

    m_qClientList.insert(m_iNextClientId, t_pStreamThread);
    ++m_iNextClientId;

    //when thread has finished it gets deleted
    connect(t_pStreamThread, SIGNAL(finished()), t_pStreamThread, SLOT(deleteLater()));
    connect(this, SIGNAL(closeFiffStreamServer()), t_pStreamThread, SLOT(deleteLater()));

    t_pStreamThread->start();
}
