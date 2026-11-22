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

#include <base/log.h>
#include "commandserver.h"

#include "commandthread.h"
#include "mne_rt_server.h"
#include "fiffstreamserver.h"
#include "fiffstreamthread.h"
#include "connectormanager.h"
#include <stdlib.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandServer::CommandServer(QObject* parent)
: QObject(parent)
, m_iThreadCount(0)
, m_iCurrentCommandThreadID(0)
, m_iPort(0)
{
    QObject::connect(&m_commandParser, &CommandParser::response, this, &CommandServer::prepareReply);
}

//=============================================================================================================

CommandServer::~CommandServer()
{
    m_listener.request_stop();
    if (m_listener.joinable()) {
        m_listener.join();
    }
    emit closeCommandThreads();
}

//=============================================================================================================

bool CommandServer::listen(std::uint16_t port, QString& error)
{
    auto server = mne::TcpServer::listen("0.0.0.0", port);
    if (!server) {
        error = QString::fromStdString(server.error().message());
        return false;
    }
    m_iPort = server->port();
    m_listener = std::jthread([server = std::move(*server), receiver = QPointer<CommandServer>(this)](const std::stop_token& stop) mutable {
        while (!stop.stop_requested()) {
            auto client = server.accept(std::chrono::milliseconds(100));
            if (!client) {
                if (client.error() != std::errc::timed_out) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }
            auto socket = std::make_shared<mne::TcpSocket>(std::move(*client));
            QMetaObject::invokeMethod(
                receiver,
                [receiver, socket]() {
                    if (receiver) {
                        receiver->incomingConnection(std::move(*socket));
                    }
                },
                Qt::QueuedConnection);
        }
    });
    return true;
}

//=============================================================================================================

std::uint16_t CommandServer::serverPort() const
{
    return m_iPort;
}

//=============================================================================================================

void CommandServer::incommingCommand(QString p_sCommand, qint32 p_iThreadID)
{
    QStringList t_qListParsedCommands;

    m_iCurrentCommandThreadID = p_iThreadID;

    if (!m_commandParser.parse(p_sCommand, t_qListParsedCommands)) {
        QByteArray t_blockReply;
        t_blockReply.append("command unknown\r\n");
        mne::Log::warning("[CommandServer::incommingCommand] command unknown: {}", p_sCommand.toStdString());

        //send reply
        emit replyCommand(t_blockReply, p_iThreadID);
    }
}

//=============================================================================================================

void CommandServer::incomingConnection(mne::TcpSocket socket)
{
    CommandThread* t_pCommandThread = new CommandThread(std::move(socket), m_iThreadCount, this);
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

void CommandServer::registerCommandManager(CommandManager& p_commandManager)
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
