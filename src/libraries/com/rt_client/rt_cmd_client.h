//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rt_cmd_client.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    TCP client for the @c mne_rt_server command port (4217): negotiates session state via line- or JSON-encoded messages.
 *
 * @ref COMLIB::RtCmdClient is a @c QTcpSocket subclass that drives the
 * server’s control channel. Two equivalent wire encodings are
 * supported: plain CLI strings (newline-terminated tokens) for
 * interactive shell use, and JSON command objects for programmatic
 * clients. Both are produced from the same @ref COMLIB::Command
 * vocabulary so the help text, parameter list and reply parsing stay in
 * lock-step regardless of which transport a caller picks.
 *
 * The client maintains an internal @ref COMLIB::CommandManager that is
 * populated on demand by @c requestCommands(); after that round-trip the
 * @c [] operator and @c hasCommand() expose the server’s self-described
 * vocabulary so callers can ask, for example, @c client["start-measurement"]
 * without hard-coding the parameter list locally.
 * @c requestBufsize() and @c requestConnectors() are convenience wrappers
 * around the corresponding server commands used during the
 * @ref RtClient handshake.
 *
 * All socket reads are funnelled through @c waitForDataAvailable() and
 * accumulated into @c m_sAvailableData, which @c readAvailableData()
 * drains atomically under @c m_qMutex. The @c response(QString) signal
 * fires whenever a complete reply has been collected so callers running
 * in the GUI thread can react without polling the socket directly.
 */

#ifndef RTCMDCLIENT_H
#define RTCMDCLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../com_global.h"
#include "../rt_command/command_manager.h"
#include "../rt_command/command.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QTcpSocket>

//=============================================================================================================
// DEFINE NAMESPACE COMLIB
//=============================================================================================================

namespace COMLIB
{

//=============================================================================================================
/**
 * @brief @c QTcpSocket subclass driving the @c mne_rt_server control channel (default port 4217).
 *
 * Sends commands in either CLI or JSON form via @c sendCLICommand /
 * @c sendCommandJSON, accumulates replies under an internal mutex so a
 * worker thread can produce while the GUI thread consumes through
 * @c readAvailableData, and mirrors the server’s self-described command
 * vocabulary into an embedded @ref CommandManager so callers can look
 * commands up by name without hard-coding their schemas.
 */
class COMSHARED_EXPORT RtCmdClient : public QTcpSocket
{
    Q_OBJECT

public:
    typedef QSharedPointer<RtCmdClient> SPtr;            /**< Shared pointer type for RtCmdClient. */
    typedef QSharedPointer<const RtCmdClient> ConstSPtr; /**< Const shared pointer type for RtCmdClient. */

    //=========================================================================================================
    /**
     * Creates the real-time command client.
     *
     * @param[in] parent     Parent QObject (optional).
     */
    explicit RtCmdClient(QObject *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Checks if a command is managed;
     *
     * @param p_sCommand     Command to check.
     *
     * @return true if part of command manager, false otherwise.
     */
    inline bool hasCommand(const QString &p_sCommand) const;

    //=========================================================================================================
    /**
     * Sends a command line formatted command to a connected mne_rt_server
     *
     * @param[in] p_sCommand    The command to send.
     *
     * @return mne_rt_server reply.
     */
    QString sendCLICommand(const QString &p_sCommand);

    //=========================================================================================================
    /**
     * Sends a command to a connected mne_rt_server
     *
     * @param[in] p_command    The command to send.
     *
     * @return mne_rt_server reply.
     */
    void sendCommandJSON(const Command &p_command);

    //=========================================================================================================
    /**
     * Returns the available data.
     *
     * @return the available data.
     */
    inline QString readAvailableData();

    //=========================================================================================================
    /**
     * Request buffer size from mne_rt_server
     */
    qint32 requestBufsize();

    //=========================================================================================================
    /**
     * Request available commands from mne_rt_server
     */
    void requestCommands();

    //=========================================================================================================
    /**
     * Request available connectors from mne_rt_server
     *
     * @param[in] p_qMapConnectors   list of connectors.
     *
     * @return the active connector.
     */
    qint32 requestConnectors(QMap<qint32, QString> &p_qMapConnectors);

    //=========================================================================================================
    /**
     * Wait for ready read until data are available.
     *
     * @param[in] msecs  time to wait in milliseconds, if -1 function will not time out. Default value is 30000.
     *
     * @return Command object related to command key word.
     */
    bool waitForDataAvailable(qint32 msecs = 30000) const;

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param[in] key    the command key word.
     *
     * @return Command object related to command key word.
     */
    Command& operator[] (const QString &key);

    //=========================================================================================================
    /**
     * Subscript operator [] to access commands by command name
     *
     * @param key    the command key word.
     *
     * @return Command object related to command key word.
     */
    const Command operator[] (const QString &key) const;

signals:
    //=========================================================================================================
    /**
     * Emits the received response.
     *
     * @param[in] p_sResponse    the received response.
     */
    void response(QString p_sResponse);

private:
    CommandManager  m_commandManager;   /**< The command manager. */
    QMutex          m_qMutex;           /**< Access serialization between threads. */
    QString         m_sAvailableData;   /**< The last received response. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline QString RtCmdClient::readAvailableData()
{
    m_qMutex.lock();
    QString p_sResponse = m_sAvailableData;
    m_sAvailableData.clear();
    m_qMutex.unlock();

    return p_sResponse;
}

//=============================================================================================================

inline bool RtCmdClient::hasCommand(const QString &p_sCommand) const
{
    return m_commandManager.hasCommand(p_sCommand);
}
} // NAMESPACE

#endif // RTCMDCLIENT_H
