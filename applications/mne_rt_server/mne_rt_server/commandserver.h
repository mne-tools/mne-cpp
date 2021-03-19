//=============================================================================================================
/**
 * @file     commandserver.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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

#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <communication/rtCommand/commandparser.h>
#include <communication/rtCommand/commandmanager.h>

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
    inline COMMUNICATIONLIB::CommandParser& getCommandParser();

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
    void registerCommandManager(COMMUNICATIONLIB::CommandManager &p_commandManager);

    //=========================================================================================================
    /**
     * Is called to prepare the reply
     *
     * @param[in] p_sReply   The reply which should be send back.
     * @param[in] p_command  Comman which evoked the reply.
     */
    void prepareReply(QString p_sReply, COMMUNICATIONLIB::Command p_command);

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

    COMMUNICATIONLIB::CommandParser m_commandParser;      /**< Command parser. */

//    QMultiMap<QString, qint32> m_qMultiMapCommandThreadID;//This is need when commands are processed by different threads; currently its only one command per time processed by one thread --> m_iCurrentCommandThreadID
    qint32 m_iCurrentCommandThreadID;   /**< Command Thread ID of the current command. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline COMMUNICATIONLIB::CommandParser& CommandServer::getCommandParser()
{
    return m_commandParser;
}
} // NAMESPACE

#endif //INSTRUCTIONSERVER_H
