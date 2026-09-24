//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandserver.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandServer Class.
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

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace COMLIB;

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
        qWarning("[CommandServer::incommingCommand] command unknown: %s",
                 p_sCommand.toUtf8().constData());

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
