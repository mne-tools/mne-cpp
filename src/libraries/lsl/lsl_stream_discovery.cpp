//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_discovery.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implements the liblsl discovery query: UDP "LSL:shortinfo" requests and the outlets' XML answers.
 *
 * @ref mne::resolve_stream puts the property into the query's predicate, so outlets that do not match stay
 * silent; it also filters locally. Each result's @c data_host is the sender of the answer, which copes with
 * hosts that have several interfaces or resolve their own name to a loopback address.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_stream_discovery.h"
#include "lsl_protocol.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHostAddress>
#include <QNetworkDatagram>
#include <QUdpSocket>
#include <QVariant>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <random>
#include <set>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace mne::detail;

//=============================================================================================================
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::chrono::milliseconds kResend{500};

std::vector<stream_info> resolve(const std::string& predicate, double timeout, std::size_t minimum = 0)
{
    std::vector<stream_info> results;
    QUdpSocket socket;
    if (!detail::bindQuerySocket(socket, 0)) {
        return results;
    }
    const auto port = socket.localPort();
    const QByteArray id = QByteArray::fromStdString(std::to_string(std::random_device{}()));
    const QByteArray query = "LSL:shortinfo\r\n" + QByteArray::fromStdString(predicate) + "\r\n" +
        QByteArray::number(port) + " " + id + "\r\n";
    const QByteArray prefix = id + "\r\n";

    std::set<std::string> seen;
    using Clock = std::chrono::steady_clock;
    const Clock::time_point deadline =
        Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeout));
    Clock::time_point nextSend = Clock::now();
    while (Clock::now() < deadline && (minimum == 0 || results.size() < minimum)) {
        if (Clock::now() >= nextSend) {
            sendQuery(socket, query);
            nextSend = Clock::now() + kResend;
        }
        const auto wait = std::chrono::ceil<std::chrono::milliseconds>(std::min(deadline, nextSend) - Clock::now());
        detail::waitForDatagrams(socket, port, static_cast<int>(std::max<std::chrono::milliseconds::rep>(wait.count(), 0)));
        while (socket.hasPendingDatagrams()) {
            const auto datagram = socket.receiveDatagram();
            const auto data = datagram.data();
            if (!data.startsWith(prefix)) {
                continue;
            }
            stream_info info = stream_info::from_string(data.mid(prefix.size()).toStdString());
            if (info.uid().empty() || !seen.insert(info.uid()).second) {
                continue;
            }
            if (info.data_host().empty()) {
                info.set_data_host(hostText(datagram.senderAddress()));
            }
            results.push_back(std::move(info));
        }
    }
    return results;
}

} // namespace

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

std::vector<mne::stream_info> mne::resolve_streams(double timeout)
{
    return resolve("session_id='default'", timeout);
}

//=============================================================================================================

std::vector<mne::stream_info> mne::resolve_stream(const std::string& prop,
                                                  const std::string& value,
                                                  double timeout,
                                                  int minimum)
{
    std::vector<stream_info> filtered;
    if (value.find('\'') != std::string::npos) {
        return filtered; // a quote cannot appear in an LSL predicate value
    }
    std::string test = prop;
    test.append("='").append(value).append("'");
    for (stream_info& info : resolve("session_id='default' and " + test, timeout, static_cast<std::size_t>(std::max(minimum, 0)))) {
        if (matchesQuery(info, test)) {
            filtered.push_back(std::move(info));
        }
    }
    return filtered;
}
