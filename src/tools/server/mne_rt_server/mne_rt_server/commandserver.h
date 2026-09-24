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

#include <com/rt_command/command_parser.h>
#include <com/rt_command/command_manager.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>
#include <QTcpServer>

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
 * Command Server which manages command connections in seperate threads
 *
 * @brief CommandServer manages threaded command connections
 */
class CommandServer : public QTcpServer
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a CommandServer
     *
     * @param[in] parent         Parent QObject (optional).
     */
    CommandServer(QObject *parent = 0);

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
    inline COMLIB::CommandParser& getCommandParser();

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
    void registerCommandManager(COMLIB::CommandManager &p_commandManager);

    //=========================================================================================================
    /**
     * Is called to prepare the reply
     *
     * @param[in] p_sReply   The reply which should be send back.
     * @param[in] p_command  Comman which evoked the reply.
     */
    void prepareReply(QString p_sReply, COMLIB::Command p_command);

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

protected:
    //=========================================================================================================
    /**
     * Slot which handels incomming connections.
     */
    void incomingConnection(qintptr socketDescriptor);

private:
    qint32 m_iThreadCount;              /**< Is incresed each time a new command client connects to mne_rt_server. */

    COMLIB::CommandParser m_commandParser;      /**< Command parser. */

//    QMultiMap<QString, qint32> m_qMultiMapCommandThreadID;//This is need when commands are processed by different threads; currently its only one command per time processed by one thread --> m_iCurrentCommandThreadID
    qint32 m_iCurrentCommandThreadID;   /**< Command Thread ID of the current command. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline COMLIB::CommandParser& CommandServer::getCommandParser()
{
    return m_commandParser;
}
} // NAMESPACE

#endif //INSTRUCTIONSERVER_H
