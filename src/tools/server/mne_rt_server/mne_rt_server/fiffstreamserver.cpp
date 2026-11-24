//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffstreamserver.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Felix Arndt <Felix.Arndt@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffStreamServer Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamserver.h"
#include "fiffstreamthread.h"

#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <format>
#include <optional>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamServer::FiffStreamServer(CommandManager& p_commandManager)
: m_commandManager(p_commandManager)
, m_iNextClientId(0)
, m_iPort(0)
{
}

//=============================================================================================================

FiffStreamServer::~FiffStreamServer()
{
    close();
}

//=============================================================================================================

bool FiffStreamServer::listen(std::uint16_t port, std::string& error)
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

std::uint16_t FiffStreamServer::serverPort() const
{
    return m_iPort;
}

//=============================================================================================================

void FiffStreamServer::setMeasInfoRequestCallback(MeasInfoRequestCallback callback)
{
    const std::scoped_lock lock(m_requestMutex);
    m_measInfoRequest = std::move(callback);
}

//=============================================================================================================

void FiffStreamServer::comClist(const Command& /*p_command*/)
{
    //ToDo JSON
    std::string t_sOutput("\tID\tAlias\r\n");
    {
        const std::scoped_lock lock(m_clientsMutex);
        removeFinishedClients();
        for (const auto& [id, client] : m_qClientList) {
            t_sOutput += std::format("\t{}\t{}\r\n", id, client->getAlias());
        }
    }
    t_sOutput += "\n";
    m_commandManager["clist"].reply(t_sOutput);
}

//=============================================================================================================

void FiffStreamServer::comMeasinfo(const Command& p_command)
{
    const std::int32_t t_id = p_command.pValues()[0].toInt();

    if (t_id != -1) {
        requestMeasInfo(t_id);

        m_commandManager["measinfo"].reply(std::format("\tsend measurement info to FiffStreamClient (ID: {})\r\n\n", t_id));
    }
}

//=============================================================================================================

void FiffStreamServer::comStart(const Command& p_command)
{
    std::int32_t t_id = -1;
    std::string t_sOutput = parseToId(p_command.pValues()[0].toString(), t_id);

    if (t_id != -1) {
        startMeasFiffStreamClient(t_id);

        t_sOutput += std::format("\tFiffStreamClient (ID: {}) is now set to accept raw buffers\r\n\n", t_id);
    }
    m_commandManager["start"].reply(t_sOutput);
}

//=============================================================================================================

void FiffStreamServer::comStop(const Command& p_command)
{
    std::int32_t t_id = -1;
    std::string t_sOutput = parseToId(p_command.pValues()[0].toString(), t_id);

    if (t_id != -1) {
        stopMeasFiffStreamClient(t_id);

        t_sOutput += std::format("\tstop FiffStreamClient (ID: {}) from receiving raw Buffers.\r\n\n", t_id);
    }
    m_commandManager["stop"].reply(t_sOutput);
}

//=============================================================================================================

void FiffStreamServer::comStopAll(const Command& /*p_command*/)
{
    stopMeasFiffStreamClient(-1);
    m_commandManager["stop-all"].reply("\tstop all FiffStreamClients from receiving raw buffers\r\n\n");
}

//=============================================================================================================

void FiffStreamServer::connectCommands()
{
    m_commandManager["clist"].connectExecuted([this](const Command& command) { comClist(command); });
    m_commandManager["measinfo"].connectExecuted([this](const Command& command) { comMeasinfo(command); });
    m_commandManager["start"].connectExecuted([this](const Command& command) { comStart(command); });
    m_commandManager["stop"].connectExecuted([this](const Command& command) { comStop(command); });
    m_commandManager["stop-all"].connectExecuted([this](const Command& command) { comStopAll(command); });
}

//=============================================================================================================

std::string FiffStreamServer::parseToId(const std::string& p_sRawId, std::int32_t& p_iParsedId)
{
    p_iParsedId = -1;
    std::string t_blockCmdIdInfo;
    if (!p_sRawId.empty()) {
        const std::scoped_lock lock(m_clientsMutex);
        removeFinishedClients();
        const std::optional<int> t_id = mne::to_int(p_sRawId);

        if (t_id && m_qClientList.contains(*t_id)) {
            p_iParsedId = *t_id;
        } else {
            for (const auto& [id, client] : m_qClientList) {
                const std::string t_sAlias = client->getAlias();
                if (t_sAlias == p_sRawId) {
                    p_iParsedId = id;
                    t_blockCmdIdInfo += std::format("\tconvert alias '{}' => {}\r\n", t_sAlias, id);
                    break;
                }
            }
        }
    }

    if (p_iParsedId != -1) {
        t_blockCmdIdInfo += std::format("\tselect FiffStreamClient {}\r\n", p_iParsedId);
    } else {
        t_blockCmdIdInfo += "\twarning: requested FiffStreamClient not available\r\n\n";
    }

    return t_blockCmdIdInfo;
}

//=============================================================================================================

void FiffStreamServer::requestMeasInfo(std::int32_t ID)
{
    MeasInfoRequestCallback t_request;
    {
        const std::scoped_lock lock(m_requestMutex);
        t_request = m_measInfoRequest;
    }
    if (t_request) {
        t_request(ID);
    }
}

//=============================================================================================================

void FiffStreamServer::startMeasFiffStreamClient(std::int32_t ID)
{
    const std::scoped_lock lock(m_clientsMutex);
    for (const auto& [id, client] : m_qClientList) {
        client->startMeas(ID);
    }
}

//=============================================================================================================

void FiffStreamServer::stopMeasFiffStreamClient(std::int32_t ID)
{
    const std::scoped_lock lock(m_clientsMutex);
    for (const auto& [id, client] : m_qClientList) {
        client->stopMeas(ID);
    }
}

//=============================================================================================================

void FiffStreamServer::forwardMeasInfo(std::int32_t ID, const FiffInfo& p_fiffInfo)
{
    const std::scoped_lock lock(m_clientsMutex);
    for (const auto& [id, client] : m_qClientList) {
        client->sendMeasurementInfo(ID, p_fiffInfo);
    }
}

//=============================================================================================================

void FiffStreamServer::forwardRawBuffer(const std::shared_ptr<const Eigen::MatrixXf>& m_pMatRawData)
{
    const std::scoped_lock lock(m_clientsMutex);
    for (const auto& [id, client] : m_qClientList) {
        client->sendRawBuffer(m_pMatRawData);
    }
}

//=============================================================================================================

void FiffStreamServer::removeFinishedClients()
{
    // Joining a finished client thread returns at once, so this may run under m_clientsMutex
    std::erase_if(m_qClientList, [](const auto& entry) { return entry.second->isFinished(); });
}

//=============================================================================================================

void FiffStreamServer::incomingConnection(mne::TcpSocket socket)
{
    const std::scoped_lock lock(m_clientsMutex);
    removeFinishedClients();

    auto t_pStreamThread = std::make_unique<FiffStreamThread>(m_iNextClientId, std::move(socket));
    t_pStreamThread->start();
    m_qClientList.emplace(m_iNextClientId, std::move(t_pStreamThread));
    ++m_iNextClientId;
}

//=============================================================================================================

void FiffStreamServer::close()
{
    m_listener.request_stop();
    if (m_listener.joinable()) {
        m_listener.join();
    }
    std::map<std::int32_t, std::unique_ptr<FiffStreamThread>> t_clients;
    {
        const std::scoped_lock lock(m_clientsMutex);
        t_clients.swap(m_qClientList);
    }
    t_clients.clear(); // stops and joins every client thread
}
