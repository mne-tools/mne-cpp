//=============================================================================================================
/**
 * @file     lsl_stream_info.h
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
 * @brief    Contains the declaration of the stream_info class.
 *
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
 * @brief Describes a particular stream on the network.
 *
 * The stream_info class holds the declaration of a data stream. It describes the core properties of
 * a stream such as its name, content type, channel count and sampling rate, as well as optional
 * meta-data and network endpoint information.
 *
 * This class is API-compatible with the liblsl stream_info to serve as a drop-in replacement.
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
