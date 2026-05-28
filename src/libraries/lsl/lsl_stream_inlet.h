//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_inlet.h
 * @since 2026
 * @date  March 2026
 * @brief Declares stream_inlet, the client side of an LSL connection that pulls multichannel sample chunks from a remote outlet.
 *
 * A @ref LSLLIB::stream_inlet is constructed from a resolved
 * @ref LSLLIB::stream_info (typically obtained via
 * @ref LSLLIB::resolve_streams) and represents a one-way TCP
 * connection to the corresponding @ref LSLLIB::stream_outlet. The
 * class wraps the @c QTcpSocket plumbing, the framing of
 * channel-count-sized sample blocks, and an internal ring buffer
 * behind a PIMPL so that the public header pulls in no Qt network
 * symbols.
 *
 * The data path is intentionally chunked rather than
 * sample-at-a-time: @ref samples_available drains pending bytes off
 * the socket in a non-blocking pass and @ref pull_chunk returns
 * every fully-received multichannel sample in one call, which is the
 * pattern that real-time acquisition pipelines in mne-cpp (sensor
 * plug-ins under @c applications/mne_scan_plugins) consume most
 * efficiently. The templated @ref pull_chunk is provided purely for
 * source-level compatibility with liblsl; in this implementation
 * only the @c float specialisation is wired up, since every mne-cpp
 * acquisition path operates on 32-bit floating-point samples.
 */

#ifndef LSL_STREAM_INLET_H
#define LSL_STREAM_INLET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"
#include "lsl_stream_info.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <memory>
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE LSLLIB
//=============================================================================================================

namespace LSLLIB {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

/**
 * @brief PIMPL backend of stream_inlet that owns the QTcpSocket and the per-sample reassembly buffer.
 */
class StreamInletPrivate;

//=============================================================================================================
/**
 * @brief Client-side LSL endpoint: connects to a resolved outlet over TCP and exposes pulled samples as float chunks.
 */
class LSLSHARED_EXPORT stream_inlet
{
public:
    //=========================================================================================================
    /**
     * Construct a new stream_inlet from a resolved stream_info.
     *
     * @param[in] info  A stream_info object (typically obtained from resolve_streams).
     */
    explicit stream_inlet(const stream_info& info);

    //=========================================================================================================
    /**
     * Destructor. Closes the connection if still open.
     */
    ~stream_inlet();

    //=========================================================================================================
    /**
     * Open the TCP connection to the outlet and begin receiving data.
     *
     * @throws std::runtime_error if the connection cannot be established.
     */
    void open_stream();

    //=========================================================================================================
    /**
     * Close the TCP connection to the outlet.
     */
    void close_stream();

    //=========================================================================================================
    /**
     * Check whether new samples are available in the internal buffer.
     *
     * Also performs a non-blocking read from the TCP socket to collect any pending data.
     *
     * @return True if at least one complete sample is available.
     */
    [[nodiscard]] bool samples_available();

    //=========================================================================================================
    /**
     * Pull a chunk of multichannel data from the inlet.
     *
     * Returns all complete samples currently in the buffer. Template is provided for API
     * compatibility with liblsl; only float is supported in this implementation.
     *
     * @tparam T  The data type (must be float).
     * @return    A vector of samples, where each sample is a vector of channel values.
     */
    template<typename T>
    [[nodiscard]] std::vector<std::vector<T>> pull_chunk()
    {
        static_assert(std::is_same_v<T, float>,
                      "lsl::stream_inlet::pull_chunk<T>: only float is supported");
        return pull_chunk_float();
    }

    //=========================================================================================================
    /**
     * Pull a chunk of float data (non-template version).
     *
     * @return A vector of samples, where each sample is a vector of float channel values.
     */
    [[nodiscard]] std::vector<std::vector<float>> pull_chunk_float();

private:
    /** Opaque implementation pointer (PIMPL). */
    std::unique_ptr<StreamInletPrivate> m_pImpl;
};

} // namespace LSLLIB

#endif // LSL_STREAM_INLET_H
