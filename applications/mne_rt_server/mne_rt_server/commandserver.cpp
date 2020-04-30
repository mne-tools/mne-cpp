//=============================================================================================================
/**
 * @file     commandserver.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Felix Arndt, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     Definition of the CommandServer Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "commandserver.h"
#include "commandthread.h"

#include "mne_rt_server.h"

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"
#include "mne_rt_server.h"
#include "connectormanager.h"

#include <stdlib.h>
#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace COMMUNICATIONLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandServer::CommandServer(QObject *parent)
: QTcpServer(parent)
, m_iThreadCount(0)
, m_iCurrentCommandThreadID(0)
{
    QObject::connect(&m_commandParser, &CommandParser::response, this, &CommandServer::prepareReply);
}

//=============================================================================================================

CommandServer::~CommandServer()
{
    emit closeCommandThreads();
}

//=============================================================================================================

void CommandServer::incommingCommand(QString p_sCommand, qint32 p_iThreadID)
{
    QStringList t_qListParsedCommands;

    m_iCurrentCommandThreadID = p_iThreadID;

    if(!m_commandParser.parse(p_sCommand, t_qListParsedCommands))
    {
        QByteArray t_blockReply;
        t_blockReply.append("command unknown\r\n");
        printf("%s", t_blockReply.data());

        //send reply
        emit replyCommand(t_blockReply, p_iThreadID);
    }
}

//=============================================================================================================

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

//=============================================================================================================

void CommandServer::registerCommandManager(CommandManager &p_commandManager)
{
    //Attach Observer to Subject
    m_commandParser.attach(&p_commandManager);
    //Register Reply Channel
//    p_commandManager.registerResponseChannel(&m_commandParser, &CommandParser::response);
    QObject::connect(&p_commandManager, &CommandManager::response, &m_commandParser, &CommandParser::response);
}

//=============================================================================================================

void CommandServer::prepareReply(QString p_sReply, Command p_command)
{
    //Only when multi threaded command parsing is applied
//    qDebug() << m_qMultiMapCommandThreadID;
//    QMultiMap<QString, qint32>::iterator it = m_qMultiMapCommandThreadID.find(p_command.command());
//    qint32 t_iThreadID = it.value(); //Remove this id from stored set
//    m_qMultiMapCommandThreadID.remove(p_command.command(), t_iThreadID);
//    qDebug() << QThread::currentThreadId();

    //Currently only one parsing thread per time
    qint32 t_iThreadID = m_iCurrentCommandThreadID;

    //print
//    printf("%s",p_sReply.toUtf8().constData());

    emit replyCommand(p_sReply, t_iThreadID);

    Q_UNUSED(p_command);
}
