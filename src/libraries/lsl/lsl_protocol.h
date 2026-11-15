//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_protocol.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QByteArray>
#include <QHostAddress>
#include <QUdpSocket>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

inline constexpr quint16 kDiscoveryPort = 16571;       /**< UDP port outlets listen on for queries. */
inline constexpr int kProtocolVersion = 110;           /**< Data protocol version spoken. */
inline constexpr double kTestPatternTime = 123456.789; /**< Timestamp of the two test-pattern samples. */
inline constexpr std::uint8_t kTagDeduced = 1;         /**< Sample tag: timestamp deduced from the rate. */
inline constexpr std::uint8_t kTagTransmitted = 2;     /**< Sample tag: an 8-byte timestamp follows. */

/** @return The addresses a resolver sends its queries to (IPv4 multicast groups, broadcast, localhost). */
std::vector<QHostAddress> discoveryTargets();

/** @return The multicast groups an outlet joins to hear queries. */
LSLSHARED_EXPORT std::vector<QHostAddress> discoveryGroups();

/**
 * Joins the discovery groups on the default and the loopback interface; outlets on one host share the port.
 *
 * @param[in, out] socket   The bound discovery socket.
 */
void joinDiscoveryGroups(QUdpSocket& socket);

/**
 * Sends @p query to every discovery target and, over loopback, to a group, which reaches every local outlet.
 *
 * @param[in, out] socket   A socket bound with bindQuerySocket().
 * @param[in]      query    The query datagram.
 */
void sendQuery(QUdpSocket& socket, const QByteArray& query);

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
 * @param[in, out] out         Destination.
 * @param[in] format           Channel format; values are converted (integers rounded, strings as decimal text).
 * @param[in] values           One value per channel.
 * @param[in] timestamp        Timestamp in local_clock() seconds; ignored if @p deduced.
 * @param[in] deduced          Whether the receiver deduces the timestamp from the previous sample and the rate.
 */
void appendSample(QByteArray& out, ChannelFormat format, const std::vector<double>& values, double timestamp, bool deduced);

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
    int read(const QByteArray& data, qsizetype& pos, std::uint8_t& tag, double& timestamp, std::vector<double>& values) const;

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
 * @param[in] address   Sender address of a datagram.
 *
 * @return @p address as text, plain IPv4 where it is an IPv4-mapped IPv6 address.
 */
std::string hostText(const QHostAddress& address);

/**
 * Binds @p socket for sending queries and receiving their replies (multicast TTL 1, loopback on).
 *
 * @param[in, out] socket   The socket.
 * @param[in]      port     Local port, 0 for any.
 *
 * @return Whether the socket is bound.
 */
LSLSHARED_EXPORT bool bindQuerySocket(QUdpSocket& socket, quint16 port);

/**
 * Waits up to @p ms for a datagram on @p socket. QAbstractSocket::waitForReadyRead closes the socket on any error
 * but a timeout; the socket is bound to @p port again then, so replies to queries already sent still arrive.
 *
 * @param[in, out] socket   A socket bound with bindQuerySocket().
 * @param[in]      port     Its local port.
 * @param[in]      ms       Maximum wait in milliseconds.
 *
 * @return Whether a datagram is pending.
 */
LSLSHARED_EXPORT bool waitForDatagrams(QUdpSocket& socket, quint16 port, int ms);

} // namespace mne::detail

#endif // LSL_PROTOCOL_H
