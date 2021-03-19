//=============================================================================================================
/**
 * @file     rtcmdclient.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     declaration of the RtCmdClient Class.
 *
 */

#ifndef RTCMDCLIENT_H
#define RTCMDCLIENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../communication_global.h"
#include "../rtCommand/commandmanager.h"
#include "../rtCommand/command.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDataStream>
#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QTcpSocket>

//=============================================================================================================
// DEFINE NAMESPACE COMMUNICATIONLIB
//=============================================================================================================

namespace COMMUNICATIONLIB
{

//=============================================================================================================
/**
 * The real-time command client class provides an interface to communicate with the command port 4217 of a running mne_rt_server.
 *
 * @brief Real-time command client
 */
class COMMUNICATIONSHARED_EXPORT RtCmdClient : public QTcpSocket
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
