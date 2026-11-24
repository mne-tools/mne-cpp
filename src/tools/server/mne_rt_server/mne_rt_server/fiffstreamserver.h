//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamserver.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffStreamServer Class.
 */

#ifndef FIFFSTREAMSERVER_H
#define FIFFSTREAMSERVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_info.h>
#include <rtserver_connector/command_manager.h>
#include <mne/com/tcp_server.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

//=============================================================================================================
// DEFINE NAMESPACE RTSERVER
//=============================================================================================================

namespace RTSERVER
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffStreamThread;

//=============================================================================================================
/**
 * @brief The FIFF data port: accepts FiffStreamClients and forwards measurement info and raw buffers to them.
 */
class FiffStreamServer
{
public:
    /** Requests the measurement info for the FiffStreamClient with the given ID. */
    using MeasInfoRequestCallback = std::function<void(std::int32_t)>;

    //=========================================================================================================
    /**
     * @param[in] p_commandManager   The command manager of mne_rt_server, which holds the commands of the server.
     */
    explicit FiffStreamServer(RTSERVER::CommandManager& p_commandManager);

    FiffStreamServer(const FiffStreamServer&) = delete;
    FiffStreamServer& operator=(const FiffStreamServer&) = delete;

    ~FiffStreamServer();

    //=========================================================================================================
    /**
     * Connects the commands of the server to their handlers.
     */
    void connectCommands();

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
     * @param[in] callback   Receiver of the measurement info requests; empty to disconnect.
     */
    void setMeasInfoRequestCallback(MeasInfoRequestCallback callback);

    //=========================================================================================================
    /**
     * Sends measurement info to a client.
     *
     * @param[in] ID             The addressed client.
     * @param[in] p_fiffInfo     The measurement info.
     */
    void forwardMeasInfo(std::int32_t ID, const mne::FiffInfo& p_fiffInfo);

    //=========================================================================================================
    /**
     * Sends a raw buffer to the clients that receive raw buffers.
     *
     * @param[in] m_pMatRawData  The raw buffer.
     */
    void forwardRawBuffer(const std::shared_ptr<const Eigen::MatrixXf>& m_pMatRawData);

    //=========================================================================================================
    /**
     * Stops accepting clients and disconnects all clients.
     */
    void close();

private:
    void incomingConnection(mne::TcpSocket socket);

    void comClist(const RTSERVER::Command& p_command);
    void comMeasinfo(const RTSERVER::Command& p_command);
    void comStart(const RTSERVER::Command& p_command);
    void comStop(const RTSERVER::Command& p_command);
    void comStopAll(const RTSERVER::Command& p_command);

    void requestMeasInfo(std::int32_t ID);
    void startMeasFiffStreamClient(std::int32_t ID);
    void stopMeasFiffStreamClient(std::int32_t ID);

    std::string parseToId(const std::string& p_sRawId, std::int32_t& p_iParsedId);

    void removeFinishedClients();

    RTSERVER::CommandManager& m_commandManager;                              /**< Commands of mne_rt_server. */
    std::mutex m_clientsMutex;                                               /**< Guards the clients and the next ID. */
    std::map<std::int32_t, std::unique_ptr<FiffStreamThread>> m_qClientList; /**< The clients by ID. */
    std::int32_t m_iNextClientId;                                            /**< ID of the next client. */
    std::mutex m_requestMutex;                                               /**< Guards m_measInfoRequest. */
    MeasInfoRequestCallback m_measInfoRequest;                               /**< Receiver of measurement info requests. */
    std::uint16_t m_iPort;                                                   /**< The port the server listens on, 0 if it does not. */
    std::jthread m_listener;                                                 /**< Accepts the clients. */
};

} // NAMESPACE

#endif //FIFFSTREAMSERVER_H
