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

#include <com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
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
    auto socket = detail::bindQuerySocket(0);
    if (!socket) {
        return results;
    }
    const std::string id = std::to_string(std::random_device{}());
    const std::string query = "LSL:shortinfo\r\n" + predicate + "\r\n" + std::to_string(socket->port()) + " " + id + "\r\n";
    const std::string prefix = id + "\r\n";

    std::set<std::string> seen;
    std::array<std::byte, 65536> buffer{};
    using Clock = std::chrono::steady_clock;
    const Clock::time_point deadline =
        Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeout));
    Clock::time_point nextSend = Clock::now();
    while (Clock::now() < deadline && (minimum == 0 || results.size() < minimum)) {
        if (Clock::now() >= nextSend) {
            sendQuery(*socket, query);
            nextSend = Clock::now() + kResend;
        }
        const auto wait = std::chrono::ceil<std::chrono::milliseconds>(std::min(deadline, nextSend) - Clock::now());
        auto datagram = socket->receive_from(buffer, std::max(wait, std::chrono::milliseconds(0)));
        while (datagram) {
            const std::string_view data(reinterpret_cast<const char*>(buffer.data()), datagram->size);
            if (data.starts_with(prefix)) {
                stream_info info = stream_info::from_string(std::string(data.substr(prefix.size())));
                if (!info.uid().empty() && seen.insert(info.uid()).second) {
                    if (info.data_host().empty()) {
                        info.set_data_host(datagram->address);
                    }
                    results.push_back(std::move(info));
                }
            }
            datagram = socket->receive_from(buffer, std::chrono::milliseconds(0));
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
