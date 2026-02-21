//=============================================================================================================
/**
 * @file     stream_outlet.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the definition of the stream_outlet class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stream_outlet.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <thread>
#include <mutex>
#include <atomic>
#include <cstring>
#include <queue>
#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLLIB;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace {
    const QHostAddress  DISCOVERY_MULTICAST_GROUP("239.255.172.215");
    const quint16       DISCOVERY_PORT = 16571;
    const int           BROADCAST_INTERVAL_MS = 500;
}

//=============================================================================================================
// PRIVATE IMPLEMENTATION
//=============================================================================================================

/**
 * @brief Private implementation for stream_outlet (PIMPL).
 *
 * Runs a background thread that:
 *   1. Manages a QTcpServer for data connections from inlets.
 *   2. Periodically sends UDP multicast discovery broadcasts.
 *   3. Drains the sample queue and writes data to all connected inlets.
 */
class lsl::StreamOutletPrivate
{
public:
    StreamOutletPrivate(const stream_info& info)
    : m_info(info)
    , m_bRunning(false)
    {
    }

    ~StreamOutletPrivate()
    {
        stop();
    }

    //=========================================================================================================
    /**
     * Start the background thread (TCP server + UDP broadcast).
     */
    void start()
    {
        m_bRunning = true;
        m_bgThread = std::thread(&StreamOutletPrivate::run, this);
    }

    //=========================================================================================================
    /**
     * Stop the background thread and clean up all network resources.
     */
    void stop()
    {
        m_bRunning = false;
        if (m_bgThread.joinable()) {
            m_bgThread.join();
        }
    }

    //=========================================================================================================
    /**
     * Enqueue a single sample for transmission to connected inlets.
     */
    void enqueueSample(const std::vector<float>& sample)
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_sampleQueue.push(sample);
    }

    //=========================================================================================================
    /**
     * Enqueue a chunk of samples for transmission.
     */
    void enqueueChunk(const std::vector<std::vector<float>>& chunk)
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        for (const auto& sample : chunk) {
            m_sampleQueue.push(sample);
        }
    }

    //=========================================================================================================
    /**
     * Get the stream_info with updated data port.
     */
    stream_info info() const
    {
        return m_info;
    }

    //=========================================================================================================
    /**
     * Check if any inlets are connected.
     */
    bool haveConsumers() const
    {
        return m_nClients.load() > 0;
    }

private:
    //=========================================================================================================
    /**
     * Background thread main function.
     *
     * Creates a QTcpServer and QUdpSocket, then loops:
     *   - Accepting new connections
     *   - Sending UDP discovery broadcasts
     *   - Draining sample queue and writing to connected clients
     */
    void run()
    {
        // --- Set up TCP server ---
        QTcpServer tcpServer;
        if (!tcpServer.listen(QHostAddress::Any, 0)) {
            qDebug() << "[lsl::stream_outlet] Failed to start TCP server:" << tcpServer.errorString();
            return;
        }

        // Record the assigned port into stream_info
        m_info.set_data_port(tcpServer.serverPort());

        // --- Set up UDP socket for multicast discovery ---
        QUdpSocket udpSocket;
#ifndef Q_OS_WASM
        udpSocket.setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
#endif

        // --- Prepare the handshake header ---
        // "LSL1" (4 bytes) + channel_count (4 bytes, little-endian int)
        QByteArray handshake("LSL1", 4);
        int ch = m_info.channel_count();
        handshake.append(reinterpret_cast<const char*>(&ch), sizeof(int));

        // Prepare discovery datagram (re-created each loop to include up-to-date port)
        std::string discoveryPayload = m_info.to_string();

        // Track connected client sockets (owned by this thread)
        std::vector<QTcpSocket*> clients;

        auto lastBroadcast = std::chrono::steady_clock::now() - std::chrono::seconds(10); // send immediately

        // --- Main loop ---
        while (m_bRunning) {
            // 1. Accept new connections
            while (tcpServer.waitForNewConnection(0)) {
                QTcpSocket* client = tcpServer.nextPendingConnection();
                if (client) {
                    // Send handshake to the new client
                    client->write(handshake);
                    client->flush();
                    clients.push_back(client);
                    m_nClients.store(static_cast<int>(clients.size()));
                }
            }

            // 2. Send UDP discovery broadcast (every BROADCAST_INTERVAL_MS)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBroadcast).count();
            if (elapsed >= BROADCAST_INTERVAL_MS) {
                QByteArray datagram(discoveryPayload.c_str(), static_cast<int>(discoveryPayload.size()));
                udpSocket.writeDatagram(datagram, DISCOVERY_MULTICAST_GROUP, DISCOVERY_PORT);
                lastBroadcast = now;
            }

            // 3. Drain sample queue and write to all clients
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                while (!m_sampleQueue.empty()) {
                    const std::vector<float>& sample = m_sampleQueue.front();
                    QByteArray sampleData(reinterpret_cast<const char*>(sample.data()),
                                          static_cast<int>(sample.size() * sizeof(float)));

                    // Write to all clients, remove disconnected ones
                    auto it = clients.begin();
                    while (it != clients.end()) {
                        QTcpSocket* client = *it;
                        if (client->state() != QAbstractSocket::ConnectedState) {
                            delete client;
                            it = clients.erase(it);
                            continue;
                        }
                        client->write(sampleData);
                        ++it;
                    }

                    m_sampleQueue.pop();
                }
            }

            // Flush all clients
            for (QTcpSocket* client : clients) {
                client->flush();
            }

            m_nClients.store(static_cast<int>(clients.size()));

            // Sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // --- Cleanup ---
        for (QTcpSocket* client : clients) {
            client->disconnectFromHost();
            delete client;
        }
        clients.clear();
        m_nClients.store(0);

        tcpServer.close();
        udpSocket.close();
    }

    stream_info                         m_info;         /**< Stream description. */
    std::atomic<bool>                   m_bRunning;     /**< Background thread running flag. */
    std::thread                         m_bgThread;     /**< Background thread. */

    std::mutex                          m_queueMutex;   /**< Protects the sample queue. */
    std::queue<std::vector<float>>      m_sampleQueue;  /**< Queue of samples for transmission. */
    std::atomic<int>                    m_nClients{0};  /**< Number of connected clients. */
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

stream_outlet::stream_outlet(const stream_info& info)
: m_pImpl(new StreamOutletPrivate(info))
{
    m_pImpl->start();
}

//=============================================================================================================

stream_outlet::~stream_outlet()
{
    // unique_ptr destructor handles cleanup via StreamOutletPrivate destructor
}

//=============================================================================================================

void stream_outlet::push_sample(const std::vector<float>& sample)
{
    m_pImpl->enqueueSample(sample);
}

//=============================================================================================================

void stream_outlet::push_chunk(const std::vector<std::vector<float>>& chunk)
{
    m_pImpl->enqueueChunk(chunk);
}

//=============================================================================================================

stream_info stream_outlet::info() const
{
    return m_pImpl->info();
}

//=============================================================================================================

bool stream_outlet::have_consumers() const
{
    return m_pImpl->haveConsumers();
}
