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
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QStringList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <thread>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Command Server which manages command connections in seperate threads. A listener thread accepts the clients
 * and hands every connection to the thread of the server, which starts a CommandThread for it.
 *
 * @brief CommandServer manages threaded command connections
 */
class CommandServer : public QObject
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a CommandServer
     *
     * @param[in] parent         Parent QObject (optional).
     */
    CommandServer(QObject* parent = 0);

    //=========================================================================================================
    /**
     * Destructor
     */
    virtual ~CommandServer();

    //=========================================================================================================
    /**
     * Returns the command parser.
     *
     * @return the command parser.
     */
    inline RTSERVER::CommandParser& getCommandParser();

    //=========================================================================================================
    /**
     * Starts listening for command clients on all interfaces.
     *
     * @param[in] port       Port to listen on.
     * @param[out] error     Why the server does not listen, if it does not.
     *
     * @return Whether the server listens.
     */
    bool listen(std::uint16_t port, QString& error);

    //=========================================================================================================
    /**
     * Returns the port the server listens on.
     *
     * @return the port, 0 if the server does not listen.
     */
    std::uint16_t serverPort() const;

    //=========================================================================================================
    /**
     * Slot which is called when a new command is available.
     *
     * @param[in] p_sCommand     Raw command.
     * @param[in] p_iThreadID    ID of the thread which received the command.
     */
    void incommingCommand(QString p_sCommand, qint32 p_iThreadID);

    //=========================================================================================================
    /**
     * Registers a CommandManager (Observer) at CommandParser (Subject) to include in the chain of notifications
     *
     * @param[in] p_commandManager   Command Manager to register.
     */
    void registerCommandManager(RTSERVER::CommandManager& p_commandManager);

    //=========================================================================================================
    /**
     * Is called to prepare the reply
     *
     * @param[in] p_sReply   The reply which should be send back.
     * @param[in] p_command  Comman which evoked the reply.
     */
    void prepareReply(QString p_sReply, RTSERVER::Command p_command);

signals:
    //=========================================================================================================
    /**
     * Reply to a command
     *
     * @param[in] p_blockReply   The reply data.
     * @param[in] p_iID          ID of the client thread to identify the target.
     */
    void replyCommand(QString p_blockReply, qint32 p_iID);

    //=========================================================================================================
    /**
     * Signal which triggers closing all command clients
     */
    void closeCommandThreads();

private:
    //=========================================================================================================
    /**
     * Starts a CommandThread for a new connection.
     *
     * @param[in] socket     The accepted connection.
     */
    void incomingConnection(mne::TcpSocket socket);

    qint32 m_iThreadCount; /**< Is incresed each time a new command client connects to mne_rt_server. */

    RTSERVER::CommandParser m_commandParser; /**< Command parser. */

    //    QMultiMap<QString, qint32> m_qMultiMapCommandThreadID;//This is need when commands are processed by different threads; currently its only one command per time processed by one thread --> m_iCurrentCommandThreadID
    qint32 m_iCurrentCommandThreadID; /**< Command Thread ID of the current command. */

    std::uint16_t m_iPort;   /**< The port the server listens on, 0 if it does not. */
    std::jthread m_listener; /**< Accepts the clients; declared last so it stops first. */
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
