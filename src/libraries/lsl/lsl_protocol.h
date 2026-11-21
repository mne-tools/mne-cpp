//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_protocol.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Wire format of the Lab Streaming Layer as liblsl 1.10+ speaks it (internal to mne_lsl).
 *
 * Discovery: a resolver sends "LSL:shortinfo\r\n<predicate>\r\n<return port> <query id>\r\n" by UDP to port 16571
 * (multicast, broadcast and localhost); every matching outlet answers "<query id>\r\n<shortinfo XML>" to the
 * return port. Time correction: "LSL:timedata\r\n<wave id> <t0>\r\n" to the outlet's service port is answered
 * with " <wave id> <t0> <t1> <t2>". Data: a TCP client sends "LSL:streamfeed/110 <uid>\r\n" and headers, the
 * outlet answers "LSL/110 200 OK" and headers, then two test-pattern samples, then the samples. A sample is a
 * tag byte (1: timestamp deduced from the previous one and the rate, 2: an 8-byte timestamp follows) and the
 * channel values in the outlet's byte order; strings carry a length-size byte, the length and the bytes.
 */

#ifndef LSL_PROTOCOL_H
#define LSL_PROTOCOL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"
#include "lsl_stream_info.h"
#include <com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

inline constexpr std::uint16_t kDiscoveryPort = 16571; /**< UDP port outlets listen on for queries. */
inline constexpr int kProtocolVersion = 110;           /**< Data protocol version spoken. */
inline constexpr double kTestPatternTime = 123456.789; /**< Timestamp of the two test-pattern samples. */
inline constexpr std::uint8_t kTagDeduced = 1;         /**< Sample tag: timestamp deduced from the rate. */
inline constexpr std::uint8_t kTagTransmitted = 2;     /**< Sample tag: an 8-byte timestamp follows. */
inline constexpr const char* kLoopback = "127.0.0.1";  /**< IPv4 address of the loopback interface. */

/** @return The addresses a resolver sends its queries to (IPv4 multicast groups, broadcast, localhost). */
std::vector<std::string> discoveryTargets();

/** @return The multicast groups an outlet joins to hear queries. */
LSLSHARED_EXPORT std::vector<std::string> discoveryGroups();

/**
 * Joins the discovery groups on the default and the loopback interface; outlets on one host share the port.
 *
 * @param[in, out] socket   The discovery socket, bound to the discovery port with sharing.
 */
void joinDiscoveryGroups(UdpSocket& socket);

/**
 * Sends @p query to every discovery target and, over loopback, to a group, which reaches every local outlet.
 *
 * @param[in, out] socket   The query socket.
 * @param[in]      query    The query datagram.
 */
void sendQuery(UdpSocket& socket, std::string_view query);

/**
 * @param[in] format   Channel format.
 *
 * @return Bytes per value of @p format; 0 for strings and Undefined.
 */
std::size_t valueSize(ChannelFormat format) noexcept;

/**
 * @param[in] format   Channel format.
 *
 * @return liblsl's name of @p format ("float32", "double64", ...).
 */
std::string formatName(ChannelFormat format);

/**
 * @param[in] name   liblsl's name of a channel format.
 *
 * @return The format called @p name, Undefined if unknown.
 */
ChannelFormat formatFromName(const std::string& name);

/** @return True on little-endian hosts. */
bool littleEndian() noexcept;

/** @return "1234" on little-endian hosts, "4321" on big-endian ones (liblsl's Byte-Order header). */
const char* byteOrderName() noexcept;

/**
 * @param[in] format   Channel format.
 * @param[in] k        Channel index.
 * @param[in] offset   4 for the first test-pattern sample, 2 for the second.
 *
 * @return Value of channel @p k of liblsl's test-pattern sample.
 */
double testPatternValue(ChannelFormat format, int k, int offset) noexcept;

/**
 * Appends one sample in host byte order.
 *
 * @param[in, out] out         Destination bytes.
 * @param[in] format           Channel format; values are converted (integers rounded, strings as decimal text).
 * @param[in] values           One value per channel.
 * @param[in] timestamp        Timestamp in local_clock() seconds; ignored if @p deduced.
 * @param[in] deduced          Whether the receiver deduces the timestamp from the previous sample and the rate.
 */
void appendSample(std::string& out, ChannelFormat format, const std::vector<double>& values, double timestamp, bool deduced);

/** Incremental decoder of the sample stream after the response headers. */
class SampleReader
{
public:
    SampleReader() = default;
    SampleReader(ChannelFormat format, int channels, bool reverseBytes) noexcept;

    /**
     * Decodes the sample starting at @p pos in @p data.
     *
     * @param[in] data            Received bytes.
     * @param[in, out] pos        Read position; advanced past the sample when one is decoded.
     * @param[out] tag            Timestamp tag of the sample.
     * @param[out] timestamp      Transmitted timestamp; unchanged for a deduced one.
     * @param[out] values         One value per channel.
     *
     * @return 1 for a decoded sample, 0 if the sample is not complete yet, -1 for a malformed sample.
     */
    int read(std::string_view data, std::size_t& pos, std::uint8_t& tag, double& timestamp, std::vector<double>& values) const;

private:
    ChannelFormat m_format = ChannelFormat::Undefined;
    int m_channels = 0;
    bool m_reverse = false;
};

/**
 * @param[in] info        Stream to test.
 * @param[in] predicate   Equality tests "key='value'" joined by " and " on name, type, source_id, uid, hostname,
 *                        session_id, channel_count, channel_format and nominal_srate.
 *
 * @return Whether @p info satisfies the predicate; anything else (full XPath) is not understood and does not match.
 */
bool matchesQuery(const stream_info& info, const std::string& predicate);

/**
 * Binds a socket for sending queries and receiving their replies on all interfaces.
 *
 * @param[in] port   Local port, 0 for any.
 *
 * @return The socket, or why it could not be bound.
 */
LSLSHARED_EXPORT std::expected<UdpSocket, std::error_code> bindQuerySocket(std::uint16_t port);

} // namespace mne::detail

#endif // LSL_PROTOCOL_H
