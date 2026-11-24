//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamthread.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffStreamThread Class.
 */

#ifndef FIFFSTREAMTHREAD_H
#define FIFFSTREAMTHREAD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/com/tcp_socket.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <atomic>
#include <cstddef>
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
/**
 * @brief Serves one client of the FIFF data port in its own thread.
 *
 * Other threads queue FIFF tags (measurement info, raw buffers) for the client; the thread writes them and answers
 * the client's FIFF_MNE_RT_COMMAND tags (client ID, alias).
 */
class FiffStreamThread
{
public:
    //=========================================================================================================
    /**
     * @param[in] id         ID of the client.
     * @param[in] socket     The connected client.
     */
    FiffStreamThread(std::int32_t id, mne::TcpSocket socket);

    FiffStreamThread(const FiffStreamThread&) = delete;
    FiffStreamThread& operator=(const FiffStreamThread&) = delete;

    ~FiffStreamThread();

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

    //=========================================================================================================
    /**
     * @return The client ID.
     */
    inline std::int32_t getID() const;

    //=========================================================================================================
    /**
     * @return The alias the client set.
     */
    std::string getAlias();

    //=========================================================================================================
    /**
     * Answers a FIFF_MNE_RT_COMMAND tag of the client.
     *
     * @param[in] p_pTag     The tag.
     */
    void parseCommand(const mne::FiffTag& p_pTag);

    //=========================================================================================================
    /**
     * Queues the client ID.
     */
    void writeClientId();

    //=========================================================================================================
    /**
     * Starts the raw data block and the raw buffer sending, if @p ID is the ID of this client.
     *
     * @param[in] ID     The addressed client.
     */
    void startMeas(std::int32_t ID);

    //=========================================================================================================
    /**
     * Ends the raw data block and the raw buffer sending, if @p ID is the ID of this client or -1.
     *
     * @param[in] ID     The addressed client, -1 for all.
     */
    void stopMeas(std::int32_t ID);

    //=========================================================================================================
    /**
     * Queues the measurement info, if @p ID is the ID of this client.
     *
     * @param[in] ID             The addressed client.
     * @param[in] p_fiffInfo     The measurement info.
     */
    void sendMeasurementInfo(std::int32_t ID, const mne::FiffInfo& p_fiffInfo);

    //=========================================================================================================
    /**
     * Queues a raw buffer, if the client receives raw buffers.
     *
     * @param[in] m_pMatRawData  The raw buffer.
     */
    void sendRawBuffer(const std::shared_ptr<const Eigen::MatrixXf>& m_pMatRawData);

private:
    void run();

    std::int32_t m_iDataClientId;        /**< The client ID. */
    std::string m_sDataClientAlias;      /**< The client alias. */
    mne::TcpSocket m_socket;             /**< The client. */
    std::mutex m_qMutex;                 /**< Guards the alias, the send block and the sending flag. */
    std::vector<std::byte> m_qSendBlock; /**< Bytes not sent yet. */
    bool m_bIsSendingRawBuffer;          /**< Whether the client receives raw buffers. */
    std::atomic<bool> m_bIsRunning;      /**< False to stop the thread. */
    std::atomic<bool> m_bIsFinished;     /**< True once the thread has ended. */
    std::thread m_thread;                /**< Serves the client. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline std::int32_t FiffStreamThread::getID() const
{
    return m_iDataClientId;
}

} // NAMESPACE

#endif //FIFFSTREAMTHREAD_H
