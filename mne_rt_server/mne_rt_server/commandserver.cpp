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
* @brief     implementation of the CommandServer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandserver.h"
#include "commandthread.h"

#include "ICommandParser.h"

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"
#include "mne_rt_server.h"
#include "connectormanager.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <stdlib.h>
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

CommandServer::CommandServer(QObject *parent)
: QTcpServer(parent)
, m_iThreadCount(0)
{

    this->init();

    QObject::connect(&m_commandParser, &CommandParser::response, this, &CommandServer::replyCommandNew);
}


//*************************************************************************************************************

CommandServer::~CommandServer()
{
    emit closeCommandThreads();
}


//*************************************************************************************************************

QByteArray CommandServer::availableCommands()
{
    QByteArray t_blockCmdInfoList;
    t_blockCmdInfoList.append("\tmeasinfo [ID/Alias]\tsends the measurement info to the specified\r\n\t\t\t\tFiffStreamClient\r\n");
    t_blockCmdInfoList.append("\tmeas     [ID/Alias]\tadds specified FiffStreamClient to raw data\r\n\t\t\t\tbuffer receivers. If acquisition is not already strated, it is triggered.\r\n");
    t_blockCmdInfoList.append("\tstop     [ID/Alias]\tremoves specified FiffStreamClient from raw\r\n\t\t\t\tdata buffer receivers.\r\n");
    t_blockCmdInfoList.append("\tstop-all\t\t\tstops the whole acquisition process.\r\n");

    t_blockCmdInfoList.append("\n\tconlist\t\t\tprints and sends all available connectors\r\n");
    t_blockCmdInfoList.append("\tselcon   [ConID]\tselects a new connector, if a measurement is running it will be stopped.\r\n");

    t_blockCmdInfoList.append("\n\thelp\t\t\tprints and sends this list\r\n");

    t_blockCmdInfoList.append("\n\tclose\t\t\tcloses mne_rt_server\r\n\n");

    //Commands of Registered command parsers
    QList<ICommandParser*>::const_iterator i;
    for (i = m_qListParser.begin(); i != m_qListParser.end(); ++i)
        t_blockCmdInfoList.append((*i)->availableCommands());

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

void CommandServer::incommingCommand(QString p_sCommand, qint32 p_iThreadID)
{

    QStringList t_qCommandList = p_sCommand.split(" ");

    QByteArray t_blockReply;

    bool success = this->parseCommand(t_qCommandList, t_blockReply);

    //
    // Unknown command
    //
    if(!success)
    {
        t_blockReply.append("command unknown\r\n");
//        t_blockReply.append(this->availableCommands());
    }

    //print
    std::cout << t_blockReply.data();

    //send reply
    emit replyCommand(t_blockReply, p_iThreadID);

    //NEW
    qDebug() << "################### NEW ###################";
    m_commandParser.parse(p_sCommand);
}


//*************************************************************************************************************

void CommandServer::comClose()
{
    //
    // Closes mne_rt_server
    //
    printf("close NEW\n");
    emit qobject_cast<MNERTServer*>(this->parent())->closeServer();
}


//*************************************************************************************************************

void CommandServer::comHelp()
{

    //
    // Help
    //
    printf("help NEW\n");
    Subject::t_Observers::Iterator it;
    for(it = m_commandParser.observers().begin(); it != m_commandParser.observers().end(); ++it)
    {
        CommandManager* t_pCommandManager = static_cast<CommandManager*> (*it);

        qDebug() << t_pCommandManager->toJsonObject();

//        printf("NEW %s\n", t_pCommandManager->toString().toLatin1().constData());
//        qDebug() << "NEW\n" << t_pCommandManager->toString();
    }
    m_commandManager["help"].reply("Was in Help");
}


//*************************************************************************************************************

void CommandServer::init()
{
    //insert commands

//    //OPTION 1
//    QStringList t_qListParamNames;
//    QList<QVariant> t_qListParamValues;
//    QStringList t_qListParamDescription;

//    t_qListParamNames.append("id");
//    t_qListParamValues.append(QVariant(QVariant::String));
//    t_qListParamDescription.append("ID/Alias");

//    m_commandManager.insert("measinfo", Command("measinfo", "sends the measurement info to the specified FiffStreamClient.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    m_commandManager.insert("meas", Command("meas", "adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already strated, it is triggered.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    m_commandManager.insert("stop", Command("stop", "removes specified FiffStreamClient from raw data buffer receivers.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));
//    t_qListParamNames.clear(); t_qListParamValues.clear();t_qListParamDescription.clear();
//    m_commandManager.insert(QString("stop-all"), QString("stops the whole acquisition process."));

//    m_commandManager.insert(QString("conlist"), QString("prints and sends all available connectors"));

//    t_qListParamNames.append("ConID");
//    t_qListParamValues.append(QVariant(QVariant::Int));
//    t_qListParamDescription.append("Connector ID");
//    m_commandManager.insert("\tselcon", Command("\tselcon", "selects a new connector, if a measurement is running it will be stopped.", t_qListParamNames, t_qListParamValues, t_qListParamDescription));

//    m_commandManager.insert(QString("help"), QString("prints and sends this list"));

//    m_commandManager.insert(QString("close"), QString("closes mne_rt_server"));

    //OPTION 2
    QString t_sJsonCommand =
                    "{"
                    "   \"commands\": {"
                    "       \"measinfo\": {"
                    "           \"description\": \"Sends the measurement info to the specified FiffStreamClient.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "       },"
                    "       \"meas\": {"
                    "           \"description\": \"Adds specified FiffStreamClient to raw data buffer receivers. If acquisition is not already strated, it is triggered.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "       },"
                    "       \"stop\": {"
                    "           \"description\": \"Removes specified FiffStreamClient from raw data buffer receivers.\","
                    "           \"parameters\": {"
                    "               \"id\": {"
                    "                   \"description\": \"ID/Alias\","
                    "                   \"type\": \"QString\" "
                    "               }"
                    "           }"
                    "       },"
                    "       \"stop-all\": {"
                    "           \"description\": \"Stops the whole acquisition process.\","
                    "           \"parameters\": {}"
                    "       },"
                    "       \"conlist\": {"
                    "           \"description\": \"Prints and sends all available connectors.\","
                    "           \"parameters\": {}"
                    "       },"
                    "       \"selcon\": {"
                    "           \"description\": \"Selects a new connector, if a measurement is running it will be stopped.\","
                    "           \"parameters\": {"
                    "               \"ConID\": {"
                    "                   \"description\": \"Connector ID\","
                    "                   \"type\": \"int\" "
                    "               }"
                    "           }"
                    "       },"
                    "       \"help\": {"
                    "           \"description\": \"Prints and sends this list.\","
                    "           \"parameters\": {}"
                    "       },"
                    "       \"close\": {"
                    "           \"description\": \"Closes mne_rt_server.\","
                    "           \"parameters\": {}"
                    "       }"
                    "    }"
                    "}";

    QJsonDocument t_jsonDocumentOrigin = QJsonDocument::fromJson(t_sJsonCommand.toLatin1());
    m_commandManager.insert(t_jsonDocumentOrigin);


    //Register the own command manager
    this->registerCommandManager(m_commandManager);

    //Connect slots
    m_commandManager.connectSlot(QString("help"), this, &CommandServer::comHelp);
    m_commandManager.connectSlot(QString("close"), this, &CommandServer::comClose);
}


//*************************************************************************************************************
//OLD
bool CommandServer::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    //OLD
    bool success = false;

    if(p_sListCommand[0].compare("help",Qt::CaseInsensitive) == 0)
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
        emit qobject_cast<MNERTServer*>(this->parent())->closeServer();
        success = true;
    }
    else
    {
        //
        // Forward the command to all registered command parsers
        //
        QList<ICommandParser*>::iterator i;
        for (i = m_qListParser.begin(); i != m_qListParser.end(); ++i)
            if((*i)->parseCommand(p_sListCommand, p_blockOutputInfo))
                success = true;
    }

    return success;
}


//*************************************************************************************************************

void CommandServer::incomingConnection(qintptr socketDescriptor)
{
    CommandThread* t_pCommandThread = new CommandThread(socketDescriptor, m_iThreadCount, this);
    ++m_iThreadCount;

    //when thread has finished it gets deleted
    connect(t_pCommandThread, SIGNAL(finished()), t_pCommandThread, SLOT(deleteLater()));
    connect(this, SIGNAL(closeCommandThreads()), t_pCommandThread, SLOT(deleteLater()));

    //Forwards for thread safety
    //Connect incomming commands
    connect(t_pCommandThread, &CommandThread::newCommand,
            this, &CommandServer::incommingCommand);
    //Connect command Replies
    connect(this, &CommandServer::replyCommand,
            t_pCommandThread, &CommandThread::attachCommandReply);

    t_pCommandThread->start();
}


//*************************************************************************************************************
//OLD remove this
void CommandServer::registerCommandParser(ICommandParser* p_pCommandParser)
{
    m_qListParser.append(p_pCommandParser);
}


//*************************************************************************************************************
// NEW
void CommandServer::registerCommandManager(CommandManager &p_commandManager)
{
    //Attach Observer to Subject
    m_commandParser.attach(&p_commandManager);
    //Register Reply Channel
    p_commandManager.registerResponseChannel(&m_commandParser, &CommandParser::response);
}


//*************************************************************************************************************

void CommandServer::replyCommandNew(QString p_sReply)
{
    qDebug() << "void CommandServer::replyCommandNew(QString p_sReply)\n\t" << p_sReply;
}
