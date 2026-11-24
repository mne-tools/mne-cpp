//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     commandthread.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the CommandThread Class.
 */

#ifndef COMMANDTHREAD_H
#define COMMANDTHREAD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/com/tcp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
/**
 * @brief Serves one client of the command port in its own thread: reads framed commands and writes the replies.
 *
 * Frames are a big-endian quint16 size followed by a QDataStream (Qt 6.0) serialized QString, i.e. a big-endian
 * quint32 byte count and big-endian UTF-16 text.
 */
class CommandThread
{
public:
    /** Called in the thread of the client with each received command and the ID of the thread. */
    using CommandCallback = std::function<void(const std::string& p_sCommand, std::int32_t p_iThreadID)>;

    //=========================================================================================================
    /**
     * @param[in] socket     The connected client.
     * @param[in] p_iId      ID of the thread.
     * @param[in] onCommand  Receiver of the commands.
     */
    CommandThread(mne::TcpSocket socket, std::int32_t p_iId, CommandCallback onCommand);

    CommandThread(const CommandThread&) = delete;
    CommandThread& operator=(const CommandThread&) = delete;

    ~CommandThread();

    //=========================================================================================================
    /**
     * Queues a reply for the client, replacing a reply not sent yet, if @p p_iID is the ID of this thread.
     *
     * @param[in] p_blockReply   Reply text.
     * @param[in] p_iID          ID of the addressed thread.
     */
    void attachCommandReply(const std::string& p_blockReply, std::int32_t p_iID);

    //=========================================================================================================
    /**
     * Starts serving the client.
     */
    void start();

    //=========================================================================================================
    /**
     * Stops serving the client and waits for the thread.
     */
    void stop();

    //=========================================================================================================
    /**
     * @return Whether the client is gone and the thread has ended.
     */
    bool isFinished() const;

private:
    void run();

    mne::TcpSocket m_socket;         /**< The client. */
    std::atomic<bool> m_bIsRunning;  /**< False to stop the thread. */
    std::atomic<bool> m_bIsFinished; /**< True once the thread has ended. */
    std::int32_t m_iThreadID;        /**< ID of the thread. */
    CommandCallback m_onCommand;     /**< Receiver of the commands. */
    std::mutex m_qMutex;             /**< Guards m_qSendData. */
    std::string m_qSendData;         /**< Reply not sent yet. */
    std::thread m_thread;            /**< Serves the client. */
};

} // NAMESPACE

#endif //COMMANDTHREAD_H
