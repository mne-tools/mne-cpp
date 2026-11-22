//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Declares stream_info, the LSL metadata descriptor that identifies and routes a stream on the network.
 *
 * A @ref mne::stream_info is the handshake object of the Lab
 * Streaming Layer protocol: every outlet announces itself with one,
 * @ref mne::resolve_streams returns a vector of them, and every
 * inlet is constructed from one. It carries both the semantic
 * description of the stream (name, content type, channel count,
 * nominal sampling rate, per-channel data format, and a stable
 * @c source_id used to recognise the same device across restarts)
 * and the transport metadata (per-instance UUID, originating
 * hostname, and the TCP host/port pair filled in by the discovery
 * layer) needed to actually open a connection.
 *
 * This header also defines the @ref mne::ChannelFormat enum,
 * whose integer values match the @c cf_* constants of the upstream
 * liblsl C API. @c to_string produces liblsl's shortinfo XML, so
 * outlets and inlets of this library interoperate with liblsl.
 * The class is intentionally a plain value type (cheap to copy,
 * default-constructible into an "invalid" state) so it can be passed
 * around freely between the discovery, outlet and inlet components
 * without imposing any ownership constraints on callers.
 */

#ifndef LSL_STREAM_INFO_H
#define LSL_STREAM_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Data format of a channel (mirroring the values used in the original LSL protocol).
 */
enum class ChannelFormat : int
{
    Undefined = 0, /**< Undefined format. */
    Float32 = 1,   /**< 32-bit IEEE 754 floating point. */
    Double64 = 2,  /**< 64-bit IEEE 754 floating point. */
    String = 3,    /**< Variable-length string. */
    Int32 = 4,     /**< 32-bit signed integer. */
    Int16 = 5,     /**< 16-bit signed integer. */
    Int8 = 6,      /**< 8-bit signed integer. */
    Int64 = 7      /**< 64-bit signed integer. */
};

//=============================================================================================================
/**
 * Description of one channel, as liblsl's stream description carries it in desc/channels/channel.
 *
 * @snippet ex_lsl/main.cpp lsl_channel_description_usage
 */
struct channel_description
{
    std::string label; /**< Channel name, e.g. "Fz". */
    std::string type;  /**< Content type of the channel, e.g. "EEG"; empty to take the stream's type. */
    std::string unit;  /**< Measurement unit, e.g. "microvolts". */
};

//=============================================================================================================
/**
 * @brief Value-type descriptor of a single LSL stream: semantic metadata plus transport endpoint, API-compatible with liblsl's stream_info.
 *
 * @snippet ex_lsl/main.cpp lsl_stream_outlet_usage
 */
class LSLSHARED_EXPORT stream_info
{
public:
    //=========================================================================================================
    /**
     * Default constructor. Creates an empty / invalid stream_info.
     */
    stream_info();

    //=========================================================================================================
    /**
     * Construct a new stream_info object.
     *
     * @param[in] name              Name of the stream (e.g. "EEG" or "MoCap").
     * @param[in] type              Content type of the stream (e.g. "EEG", "Markers").
     * @param[in] channel_count     Number of channels in the stream.
     * @param[in] nominal_srate     The nominal sampling rate (Hz). Use 0.0 for irregular streams.
     * @param[in] channel_format    Data format of a channel (default: cf_float32).
     * @param[in] source_id         A unique identifier for the device or source that generates the stream.
     */
    stream_info(const std::string& name,
                const std::string& type,
                int channel_count = 0,
                double nominal_srate = 0.0,
                ChannelFormat channel_format = ChannelFormat::Float32,
                const std::string& source_id = "");

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] other             stream_info to copy.
     */
    stream_info(const stream_info& other) = default;

    //=========================================================================================================
    /**
     * Copy assignment operator.
     *
     * @param[in] other             stream_info to copy.
     *
     * @return Reference to this object.
     */
    stream_info& operator=(const stream_info& other) = default;

    //=========================================================================================================
    /** @name Core Stream Properties
     *  @{ */

    /**
     * @brief Name of the stream.
     *
     * @return The stream name, e.g. "EEG"; empty for a default-constructed object.
     */
    [[nodiscard]] std::string name() const noexcept;

    /**
     * @brief Content type of the stream.
     *
     * @return The content type, e.g. "EEG" or "Markers".
     */
    [[nodiscard]] std::string type() const noexcept;

    /**
     * @brief Number of channels.
     *
     * @return The number of channels per sample.
     */
    [[nodiscard]] int channel_count() const noexcept;

    /**
     * @brief Nominal sampling rate in Hz. 0.0 means irregular.
     *
     * @return The nominal sampling rate in Hz, or 0.0 for irregular streams.
     */
    [[nodiscard]] double nominal_srate() const noexcept;

    /**
     * @brief Data format of a channel.
     *
     * @return The per-channel sample format.
     */
    [[nodiscard]] ChannelFormat channel_format() const noexcept;

    /**
     * @brief Unique source identifier.
     *
     * @return The device or source identifier; may be empty if none was given.
     */
    [[nodiscard]] std::string source_id() const noexcept;

    /** @} */

    //=========================================================================================================
    /** @name Stream Description
     *  @{ */

    /**
     * @brief Per-channel description, as liblsl's desc/channels/channel elements (label, type, unit).
     *
     * Discovery replies carry no description (as in liblsl); stream_inlet::info() fetches the full one.
     *
     * @return One entry per described channel; empty if the stream has no channel description.
     */
    [[nodiscard]] const std::vector<channel_description>& channels() const noexcept;

    /**
     * @brief Sets the per-channel description an outlet publishes with its full info.
     *
     * @param[in] channels          One entry per channel, in channel order.
     */
    void set_channels(std::vector<channel_description> channels);

    /** @} */

    //=========================================================================================================
    /** @name Network / Identity Properties
     *  @{ */

    /**
     * @brief A unique identifier for this particular stream instance (auto-generated).
     *
     * @return The instance UUID as a string without braces; empty for a default-constructed object.
     */
    [[nodiscard]] std::string uid() const noexcept;

    /**
     * @brief Hostname of the machine from which the stream originates.
     *
     * @return The local host name captured at construction time.
     */
    [[nodiscard]] std::string hostname() const noexcept;

    /** @} */

    //=========================================================================================================
    /** @name Network Endpoint (internal, set during discovery)
     *  @{ */

    /**
     * @brief TCP data port of the outlet.
     *
     * @return The TCP port number, or 0 if not yet assigned.
     */
    [[nodiscard]] int data_port() const noexcept;

    /**
     * @brief Host address of the outlet (IP, set from UDP sender address during discovery).
     *
     * @return The outlet host address, or an empty string if not yet discovered.
     */
    [[nodiscard]] std::string data_host() const noexcept;

    /**
     * @brief Set the TCP data port (used internally during discovery / outlet creation).
     *
     * @param[in] port              TCP port number on which the outlet serves data.
     */
    void set_data_port(int port);

    /**
     * @brief Set the data host (used internally during discovery).
     *
     * @param[in] host              IP address or host name of the outlet.
     */
    void set_data_host(const std::string& host);

    /**
     * @brief UDP port on which the outlet answers time-correction and direct queries.
     *
     * @return The UDP port number, or 0 if not assigned.
     */
    [[nodiscard]] int service_port() const noexcept;

    /**
     * @brief Set the UDP service port (used internally during discovery / outlet creation).
     *
     * @param[in] port              UDP port number.
     */
    void set_service_port(int port);

    /**
     * @brief Time the stream was created, in local_clock() seconds of the originating host.
     *
     * @return The creation time, 0.0 for a default-constructed object.
     */
    [[nodiscard]] double created_at() const noexcept;

    /** @} */

    //=========================================================================================================
    /** @name Serialization (used for network discovery)
     *  @{ */

    /**
     * @brief Serialize stream_info into a string for network transport.
     *
     * @return Pipe-delimited string starting with the MNELSL1 header followed by all fields except data_host.
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief The full info XML, as liblsl's stream_info::as_xml(): the shortinfo plus the stream description.
     *
     * @return The XML that an outlet sends in reply to a fullinfo request.
     */
    [[nodiscard]] std::string as_xml() const;

    /**
     * @brief Deserialize a stream_info from a network transport string.
     *
     * A desc/channels/channel description, as in as_xml() and liblsl's full info, is read into channels().
     *
     * @param[in] data              Pipe-delimited string as produced by to_string() or as_xml().
     *
     * @return The parsed stream_info, or a default-constructed (empty) one if the header or field count is invalid.
     */
    [[nodiscard]] static stream_info from_string(const std::string& data);

    /** @} */

private:
    std::string m_name;                          /**< Stream name. */
    std::string m_type;                          /**< Stream content type. */
    int m_channel_count;                         /**< Number of channels. */
    double m_nominal_srate;                      /**< Nominal sampling rate (Hz). */
    ChannelFormat m_channel_format;              /**< Channel data format. */
    std::string m_source_id;                     /**< Source identifier. */
    std::string m_uid;                           /**< Unique per-instance identifier. */
    std::string m_hostname;                      /**< Hostname of the originating machine. */
    std::vector<channel_description> m_channels; /**< Channel description (desc/channels/channel). */

    int m_data_port;           /**< TCP data port (set during discovery). */
    std::string m_data_host;   /**< Data host address (set during discovery). */
    int m_service_port = 0;    /**< UDP service port. */
    double m_created_at = 0.0; /**< Creation time in local_clock() seconds. */

    /** @return The info XML; with the stream description if @p full. */
    [[nodiscard]] std::string xml(bool full) const;
};

//=============================================================================================================
/**
 * @brief Seconds of the monotonic clock LSL timestamps refer to, as liblsl's lsl_local_clock().
 *
 * @return The current local LSL time.
 */
LSLSHARED_EXPORT double local_clock();

} // namespace mne

#endif // LSL_STREAM_INFO_H
