//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_info.h
 * @since 2026
 * @date  March 2026
 * @brief Declares stream_info, the LSL metadata descriptor that identifies and routes a stream on the network.
 *
 * A @ref LSLLIB::stream_info is the handshake object of the Lab
 * Streaming Layer protocol: every outlet announces itself with one,
 * @ref LSLLIB::resolve_streams returns a vector of them, and every
 * inlet is constructed from one. It carries both the semantic
 * description of the stream (name, content type, channel count,
 * nominal sampling rate, per-channel data format, and a stable
 * @c source_id used to recognise the same device across restarts)
 * and the transport metadata (per-instance UUID, originating
 * hostname, and the TCP host/port pair filled in by the discovery
 * layer) needed to actually open a connection.
 *
 * This header also defines the @ref LSLLIB::ChannelFormat enum,
 * whose integer values intentionally match the @c cf_* constants of
 * the upstream liblsl C API so that wire payloads serialised by
 * @c to_string remain interoperable with third-party LSL clients.
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

//=============================================================================================================
// DEFINE NAMESPACE LSLLIB
//=============================================================================================================

namespace LSLLIB {

//=============================================================================================================
/**
 * Data format of a channel (mirroring the values used in the original LSL protocol).
 */
enum class ChannelFormat : int {
    Undefined = 0,   /**< Undefined format. */
    Float32   = 1,   /**< 32-bit IEEE 754 floating point. */
    Double64  = 2,   /**< 64-bit IEEE 754 floating point. */
    String    = 3,   /**< Variable-length string. */
    Int32     = 4,   /**< 32-bit signed integer. */
    Int16     = 5,   /**< 16-bit signed integer. */
    Int8      = 6,   /**< 8-bit signed integer. */
    Int64     = 7    /**< 64-bit signed integer. */
};

//=============================================================================================================
/**
 * @brief Value-type descriptor of a single LSL stream: semantic metadata plus transport endpoint, API-compatible with liblsl's stream_info.
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
     */
    stream_info(const stream_info& other) = default;

    //=========================================================================================================
    /**
     * Copy assignment operator.
     */
    stream_info& operator=(const stream_info& other) = default;

    //=========================================================================================================
    /** @name Core Stream Properties
     *  @{ */

    /** @brief Name of the stream. */
    [[nodiscard]] std::string name() const noexcept;

    /** @brief Content type of the stream. */
    [[nodiscard]] std::string type() const noexcept;

    /** @brief Number of channels. */
    [[nodiscard]] int channel_count() const noexcept;

    /** @brief Nominal sampling rate in Hz. 0.0 means irregular. */
    [[nodiscard]] double nominal_srate() const noexcept;

    /** @brief Data format of a channel. */
    [[nodiscard]] ChannelFormat channel_format() const noexcept;

    /** @brief Unique source identifier. */
    [[nodiscard]] std::string source_id() const noexcept;

    /** @} */

    //=========================================================================================================
    /** @name Network / Identity Properties
     *  @{ */

    /** @brief A unique identifier for this particular stream instance (auto-generated). */
    [[nodiscard]] std::string uid() const noexcept;

    /** @brief Hostname of the machine from which the stream originates. */
    [[nodiscard]] std::string hostname() const noexcept;

    /** @} */

    //=========================================================================================================
    /** @name Network Endpoint (internal, set during discovery)
     *  @{ */

    /** @brief TCP data port of the outlet. */
    [[nodiscard]] int data_port() const noexcept;

    /** @brief Host address of the outlet (IP, set from UDP sender address during discovery). */
    [[nodiscard]] std::string data_host() const noexcept;

    /** @brief Set the TCP data port (used internally during discovery / outlet creation). */
    void set_data_port(int port);

    /** @brief Set the data host (used internally during discovery). */
    void set_data_host(const std::string& host);

    /** @} */

    //=========================================================================================================
    /** @name Serialization (used for network discovery)
     *  @{ */

    /** @brief Serialize stream_info into a string for network transport. */
    [[nodiscard]] std::string to_string() const;

    /** @brief Deserialize a stream_info from a network transport string. */
    [[nodiscard]] static stream_info from_string(const std::string& data);

    /** @} */

private:
    std::string         m_name;             /**< Stream name. */
    std::string         m_type;             /**< Stream content type. */
    int                 m_channel_count;    /**< Number of channels. */
    double              m_nominal_srate;    /**< Nominal sampling rate (Hz). */
    ChannelFormat       m_channel_format;   /**< Channel data format. */
    std::string         m_source_id;        /**< Source identifier. */
    std::string         m_uid;              /**< Unique per-instance identifier. */
    std::string         m_hostname;         /**< Hostname of the originating machine. */

    int                 m_data_port;        /**< TCP data port (set during discovery). */
    std::string         m_data_host;        /**< Data host address (set during discovery). */
};

} // namespace LSLLIB

#endif // LSL_STREAM_INFO_H
