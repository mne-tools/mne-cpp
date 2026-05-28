//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_discovery.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implements the UDP multicast listener that turns LSL outlet announcements on 239.255.172.215:16571 into stream_info results.
 *
 * @ref LSLLIB::resolve_streams binds a @c QUdpSocket on the LSL
 * discovery port @c 16571 with @c ShareAddress + @c ReuseAddressHint
 * so multiple LSLLIB consumers can coexist on the same host, joins
 * the well-known LSL multicast group @c 239.255.172.215 (skipped on
 * WebAssembly, where the runtime cannot join multicast groups but
 * may still receive broadcast traffic), and then enters a
 * timeout-bounded loop that pulls every pending datagram, parses it
 * via @ref LSLLIB::stream_info::from_string and accumulates the
 * results.
 *
 * Each accepted descriptor is deduplicated on its per-instance UUID
 * and has its @c data_host overwritten with the datagram's actual
 * sender address, which is more reliable than the hostname the
 * outlet self-reports (it copes with hosts that have multiple
 * interfaces or that resolve their own name to a loopback address).
 * @ref LSLLIB::resolve_stream is implemented as a thin client-side
 * filter on top: it calls @ref LSLLIB::resolve_streams once and
 * then matches the requested property locally, which keeps the
 * network-facing surface to a single well-tested code path.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_discovery.h"

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

std::vector<LSLLIB::stream_info> LSLLIB::resolve_streams(double timeout)
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

std::vector<LSLLIB::stream_info> LSLLIB::resolve_stream(const std::string& prop,
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
