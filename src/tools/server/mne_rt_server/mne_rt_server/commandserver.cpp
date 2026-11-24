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

#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CommandServer::CommandServer()
: m_iThreadCount(0)
, m_iCurrentCommandThreadID(0)
, m_iPort(0)
{
    m_commandParser.connectResponse([this](const std::string& p_sReply, const Command& p_command) { prepareReply(p_sReply, p_command); });
}

//=============================================================================================================

CommandServer::~CommandServer()
{
    close();
}

//=============================================================================================================

bool CommandServer::listen(std::uint16_t port, std::string& error)
{
    auto server = mne::TcpServer::listen("0.0.0.0", port);
    if (!server) {
        error = server.error().message();
        return false;
    }
    m_iPort = server->port();
    m_listener = std::jthread([this, server = std::move(*server)](const std::stop_token& stop) mutable {
        while (!stop.stop_requested()) {
            auto client = server.accept(std::chrono::milliseconds(100));
            if (!client) {
                if (client.error() != std::errc::timed_out) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                continue;
            }
            incomingConnection(std::move(*client));
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

void CommandServer::incommingCommand(const std::string& p_sCommand, std::int32_t p_iThreadID)
{
    const std::scoped_lock lock(m_commandMutex);

    mne::StringList t_qListParsedCommands;

    m_iCurrentCommandThreadID = p_iThreadID;

    if (!m_commandParser.parse(p_sCommand, t_qListParsedCommands)) {
        mne::Log::warning("[CommandServer::incommingCommand] command unknown: {}", p_sCommand);

        //send reply
        replyCommand("command unknown\r\n", p_iThreadID);
    }
}

//=============================================================================================================

void CommandServer::incomingConnection(mne::TcpSocket socket)
{
    std::vector<std::unique_ptr<CommandThread>> t_finished;
    const std::scoped_lock lock(m_threadsMutex);

    // Threads of clients that left are joined and dropped
    const auto t_firstFinished = std::partition(m_threads.begin(), m_threads.end(), [](const auto& thread) { return !thread->isFinished(); });
    std::move(t_firstFinished, m_threads.end(), std::back_inserter(t_finished));
    m_threads.erase(t_firstFinished, m_threads.end());

    auto t_pCommandThread = std::make_unique<CommandThread>(std::move(socket), m_iThreadCount, [this](const std::string& p_sCommand, std::int32_t p_iThreadID) { incommingCommand(p_sCommand, p_iThreadID); });
    ++m_iThreadCount;
    t_pCommandThread->start();
    m_threads.push_back(std::move(t_pCommandThread));
}

//=============================================================================================================

void CommandServer::registerCommandManager(CommandManager& p_commandManager)
{
    //Attach Observer to Subject
    m_commandParser.attach(&p_commandManager);
    //Register Reply Channel
    p_commandManager.connectResponse([this](const std::string& p_sReply, const Command& p_command) { m_commandParser.response(p_sReply, p_command); });
}

//=============================================================================================================

void CommandServer::prepareReply(const std::string& p_sReply, const Command& /*p_command*/)
{
    //Currently only one parsing thread per time
    replyCommand(p_sReply, m_iCurrentCommandThreadID);
}

//=============================================================================================================

void CommandServer::replyCommand(const std::string& p_blockReply, std::int32_t p_iID)
{
    const std::scoped_lock lock(m_threadsMutex);
    for (const auto& thread : m_threads) {
        thread->attachCommandReply(p_blockReply, p_iID);
    }
}

//=============================================================================================================

void CommandServer::close()
{
    m_listener.request_stop();
    if (m_listener.joinable()) {
        m_listener.join();
    }
    std::vector<std::unique_ptr<CommandThread>> t_threads;
    {
        const std::scoped_lock lock(m_threadsMutex);
        t_threads.swap(m_threads);
    }
    t_threads.clear(); // stops and joins every client thread
}
