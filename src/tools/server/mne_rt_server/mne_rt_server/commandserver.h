//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandserver.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandServer Class.
 */

#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <rtserver_connector/command_parser.h>
#include <rtserver_connector/command_manager.h>
#include <mne/com/tcp_server.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class CommandThread;

//=============================================================================================================
/**
 * @brief The command port: accepts command clients and runs their commands, one at a time, through the parser.
 */
class CommandServer
{
public:
    CommandServer();

    CommandServer(const CommandServer&) = delete;
    CommandServer& operator=(const CommandServer&) = delete;

    ~CommandServer();

    //=========================================================================================================
    /**
     * @return The parser the registered command managers observe.
     */
    inline RTSERVER::CommandParser& getCommandParser();

    //=========================================================================================================
    /**
     * Listens on all interfaces and accepts clients in a thread of its own.
     *
     * @param[in] port       Port, 0 for any free one.
     * @param[out] error     The reason if the server cannot listen.
     *
     * @return True if the server listens.
     */
    bool listen(std::uint16_t port, std::string& error);

    //=========================================================================================================
    /**
     * @return The port the server listens on, 0 if it does not.
     */
    std::uint16_t serverPort() const;

    //=========================================================================================================
    /**
     * Parses and runs a command; called in the thread of the client.
     *
     * @param[in] p_sCommand     The command.
     * @param[in] p_iThreadID    ID of the client thread the reply goes to.
     */
    void incommingCommand(const std::string& p_sCommand, std::int32_t p_iThreadID);

    //=========================================================================================================
    /**
     * Attaches a command manager to the parser and routes its replies to the clients.
     *
     * @param[in] p_commandManager   The manager; must outlive the server's clients.
     */
    void registerCommandManager(RTSERVER::CommandManager& p_commandManager);

    //=========================================================================================================
    /**
     * Sends a reply to the client whose command is running.
     *
     * @param[in] p_sReply   Reply text.
     * @param[in] p_command  The command replied to.
     */
    void prepareReply(const std::string& p_sReply, const RTSERVER::Command& p_command);

    //=========================================================================================================
    /**
     * Stops accepting clients and disconnects all clients.
     */
    void close();

private:
    void incomingConnection(mne::TcpSocket socket);

    void replyCommand(const std::string& p_blockReply, std::int32_t p_iID);

    std::int32_t m_iThreadCount;                           /**< Is incresed each time a new command client connects to mne_rt_server. */
    RTSERVER::CommandParser m_commandParser;               /**< Command parser. */
    std::int32_t m_iCurrentCommandThreadID;                /**< Command Thread ID of the current command. */
    std::uint16_t m_iPort;                                 /**< The port the server listens on, 0 if it does not. */
    std::mutex m_commandMutex;                             /**< Runs one command at a time. */
    std::mutex m_threadsMutex;                             /**< Guards m_threads and m_iThreadCount. */
    std::vector<std::unique_ptr<CommandThread>> m_threads; /**< The client threads. */
    std::jthread m_listener;                               /**< Accepts the clients. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline RTSERVER::CommandParser& CommandServer::getCommandParser()
{
    return m_commandParser;
}

} // NAMESPACE

#endif //INSTRUCTIONSERVER_H
