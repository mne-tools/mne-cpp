//=============================================================================================================
/**
* @file     commandserver.cpp
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
* @brief    Contains the implementation of the CommandServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandserver.h"
#include "commandthread.h"

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"
#include "mne_rt_server.h"
#include "connectormanager.h"


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandServer::CommandServer(QObject *parent)
: QTcpServer(parent)
{

}


//*************************************************************************************************************

CommandServer::~CommandServer()
{
    emit closeCommandThreads();
}


//*************************************************************************************************************

QByteArray CommandServer::availableCommands() const
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
    // ToDo Iterate over registered IConnectors
    IConnector* t_pCurrentConnector = qobject_cast<MNERTServer*>(this->parent())->m_pConnectorManager->getActiveConnector();
    if(t_pCurrentConnector)
        t_blockCmdInfoList.append(t_pCurrentConnector->availableCommands());
    else
        t_blockCmdInfoList.append("No connector commands available - no connector active.\r\n");

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

bool CommandServer::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    bool success = false;


    if(p_sListCommand[0].compare("clist",Qt::CaseInsensitive) == 0)
    {
        //
        // Print & send client list
        //
        // ToDO move this to connector manager
        printf("clist\n");
        FiffStreamServer* t_pFiffStreamServer = qobject_cast<MNERTServer*>(this->parent())->m_pFiffStreamServer;
        if(t_pFiffStreamServer)
        {
            p_blockOutputInfo.append("\tID\tAlias\r\n");
            QMap<qint32, FiffStreamThread*>::iterator i;
            for (i = t_pFiffStreamServer->m_qClientList.begin(); i != t_pFiffStreamServer->m_qClientList.end(); ++i)
            {
                QString str = QString("\t%1\t%2\r\n").arg(i.key()).arg(i.value()->getAlias());
                p_blockOutputInfo.append(str);
            }
            p_blockOutputInfo.append("\n");
            success = true;
        }
        else
        {
            p_blockOutputInfo.append("Error: fiff stream not available.\r\n");
            success = false;
        }
    }
    else if(p_sListCommand[0].compare("measinfo",Qt::CaseInsensitive) == 0)
    {
        //
        // Measurement Info
        //
        if(p_sListCommand.size() > 1)
        {
            qint32 t_id = -1;
            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

            printf("measinfo %d\r\n", t_id);

            if(t_id != -1)
            {
                emit requestMeasInfoConnector(t_id);//requestMeasInfo(t_id);

                QString str = QString("\tsend measurement info to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                p_blockOutputInfo.append(str);
                success = true;
            }
        }
    }
    else if(p_sListCommand[0].compare("meas",Qt::CaseInsensitive) == 0)
    {
        //
        // meas
        //

        if(p_sListCommand.size() > 1)
        {
            qint32 t_id = -1;
            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

            printf("meas %d\n", t_id);

            if(t_id != -1)
            {
                //emit requestStartMeas(t_id);
                emit startMeasFiffStreamClient(t_id);
                emit startMeasConnector();

                QString str = QString("\tsend measurement raw buffer to FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                p_blockOutputInfo.append(str);
            }
        }
        success = true;
    }
    else if(p_sListCommand[0].compare("stop",Qt::CaseInsensitive) == 0)
    {
        //
        // stop
        //

        if(p_sListCommand.size() > 1)
        {
            qint32 t_id = -1;
            p_blockOutputInfo.append(parseToId(p_sListCommand[1],t_id));

            printf("stop %d\n", t_id);

            if(t_id != -1)
            {
                emit stopMeasFiffStreamClient(t_id);//emit requestStopMeas(t_id);

                QString str = QString("\tstop FiffStreamClient (ID: %1)\r\n\n").arg(t_id);
                p_blockOutputInfo.append(str);
            }
        }
        success = true;
    }
    else if(p_sListCommand[0].compare("stop-all",Qt::CaseInsensitive) == 0)
    {
        //
        // stop-all
        //
        emit stopMeasFiffStreamClient(-1);//emit requestStopMeas(-1);
        emit stopMeasConnector();//emit requestStopConnector();

        QString str = QString("\tstop all FiffStreamClients and Connectors\r\n\n");
        p_blockOutputInfo.append(str);

        success = true;
    }
    else if(p_sListCommand[0].compare("help",Qt::CaseInsensitive) == 0)
    {
        //
        // Help
        //
        printf("help\n");
        p_blockOutputInfo.append(this->availableCommands());
        success = true;
    }
    else if(p_sListCommand[0].compare("close",Qt::CaseInsensitive) == 0)
    {
        //
        // Closes mne_rt_server
        //
        printf("close\n");
        emit qobject_cast<MNERTServer*>(this->parent()->parent())->closeServer();
        success = true;
    }
    else if(qobject_cast<MNERTServer*>(this->parent())->m_pConnectorManager->parseCommand(p_sListCommand, p_blockOutputInfo))
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
        p_blockOutputInfo.append("command unknown\r\n");
        p_blockOutputInfo.append(this->availableCommands());
    }

    return success;
}


//*************************************************************************************************************

void CommandServer::incomingConnection(qintptr socketDescriptor)
{
    CommandThread* t_pCommandThread = new CommandThread(socketDescriptor, this);

    //when thread has finished it gets deleted
    connect(t_pCommandThread, SIGNAL(finished()), t_pCommandThread, SLOT(deleteLater()));
    connect(this, SIGNAL(closeCommandThreads()), t_pCommandThread, SLOT(deleteLater()));

//    //Forwards for thread safety - check if obsolete!?
//    connect(t_pCommandThread, &CommandThread::requestMeasInfo,
//            this, &CommandServer::forwardMeasInfo);
//    connect(t_pCommandThread, &CommandThread::requestStartMeas,
//            this, &CommandServer::forwardStartMeas);
//    connect(t_pCommandThread, &CommandThread::requestStopMeas,
//            this, &CommandServer::forwardStopMeas);
//    connect(t_pCommandThread, &CommandThread::requestStopConnector,
//            this, &CommandServer::forwardStopConnector);

//    connect(t_pCommandThread, &CommandThread::requestSetBufferSize,
//            this, &CommandServer::forwardSetBufferSize);

    t_pCommandThread->start();
}


////*************************************************************************************************************

//void CommandServer::forwardMeasInfo(qint32 ID)
//{
//    emit requestMeasInfoConnector(ID);
//}


////*************************************************************************************************************

//void CommandServer::forwardStartMeas(qint32 ID)
//{
//    emit startMeasFiffStreamClient(ID);
//    emit startMeasConnector();
//}


////*************************************************************************************************************

//void CommandServer::forwardStopMeas(qint32 ID)
//{
//    emit stopMeasFiffStreamClient(ID);
//}


////*************************************************************************************************************

//void CommandServer::forwardStopConnector()
//{
//    emit stopMeasConnector();
//}


////*************************************************************************************************************

//void CommandServer::forwardSetBufferSize(quint32 p_uiBuffSize)
//{
//    emit setBufferSize(p_uiBuffSize);
//}


//*************************************************************************************************************

QByteArray CommandServer::parseToId(QString& p_sRawId, qint32& p_iParsedId)
{
    p_iParsedId = -1;
    QByteArray t_blockCmdIdInfo;
    if(!p_sRawId.isEmpty())
    {
        FiffStreamServer* t_pFiffStreamServer = qobject_cast<MNERTServer*>(this->parent())->m_pFiffStreamServer;
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

void CommandServer::registerCommandParser(ICommandParser* p_pCommandParser)
{

}

