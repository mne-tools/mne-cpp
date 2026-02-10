//=============================================================================================================
/**
 * @file     stream_discovery.cpp
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
 * @brief    Contains the definition of stream discovery functions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stream_discovery.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QElapsedTimer>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <set>

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

namespace {
    const QHostAddress  DISCOVERY_MULTICAST_GROUP("239.255.172.215");
    const quint16       DISCOVERY_PORT = 16571;
}

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

std::vector<lsl::stream_info> lsl::resolve_streams(double timeout)
{
    std::vector<stream_info> results;
    std::set<std::string> seenUIDs;  // track unique streams by UID

    // Create a UDP socket and join the multicast group
    QUdpSocket udpSocket;

    // Bind to the discovery port, allow address reuse for multiple listeners
    if (!udpSocket.bind(QHostAddress::AnyIPv4, DISCOVERY_PORT,
                        QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        qDebug() << "[lsl::resolve_streams] Failed to bind UDP socket:" << udpSocket.errorString();
        return results;
    }

#ifndef Q_OS_WASM
    // Join the multicast group (not available on WebAssembly)
    if (!udpSocket.joinMulticastGroup(DISCOVERY_MULTICAST_GROUP)) {
        qDebug() << "[lsl::resolve_streams] Failed to join multicast group:" << udpSocket.errorString();
        // Continue anyway — broadcast messages may still be received on some platforms
    }
#endif

    // Listen for the specified timeout
    int timeoutMs = static_cast<int>(timeout * 1000.0);
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        // Wait for datagrams with remaining time budget
        int remainingMs = timeoutMs - static_cast<int>(timer.elapsed());
        if (remainingMs <= 0) {
            break;
        }

        if (!udpSocket.waitForReadyRead(remainingMs)) {
            continue;
        }

        // Process all pending datagrams
        while (udpSocket.hasPendingDatagrams()) {
            QNetworkDatagram datagram = udpSocket.receiveDatagram();
            if (!datagram.isValid()) {
                continue;
            }

            QByteArray data = datagram.data();
            std::string payload(data.constData(), data.size());

            // Try to parse as stream_info
            stream_info info = stream_info::from_string(payload);

            // Validate the parsed info
            if (info.uid().empty() || info.name().empty()) {
                continue;
            }

            // Skip duplicates (same UID)
            if (seenUIDs.count(info.uid()) > 0) {
                continue;
            }

            // Set the data host from the UDP sender address (more reliable than self-reported hostname)
            info.set_data_host(datagram.senderAddress().toString().toStdString());

            seenUIDs.insert(info.uid());
            results.push_back(info);
        }
    }

#ifndef Q_OS_WASM
    // Leave multicast group and close
    udpSocket.leaveMulticastGroup(DISCOVERY_MULTICAST_GROUP);
#endif
    udpSocket.close();

    return results;
}

//=============================================================================================================

std::vector<lsl::stream_info> lsl::resolve_stream(const std::string& prop,
                                                   const std::string& value,
                                                   double timeout)
{
    // First get all streams
    std::vector<stream_info> all = resolve_streams(timeout);

    // Filter by the requested property
    std::vector<stream_info> filtered;
    for (const auto& info : all) {
        std::string actual;
        if (prop == "name") {
            actual = info.name();
        } else if (prop == "type") {
            actual = info.type();
        } else if (prop == "source_id") {
            actual = info.source_id();
        } else if (prop == "uid") {
            actual = info.uid();
        } else if (prop == "hostname") {
            actual = info.hostname();
        } else {
            continue;  // unknown property
        }

        if (actual == value) {
            filtered.push_back(info);
        }
    }

    return filtered;
}
