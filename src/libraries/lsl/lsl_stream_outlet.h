//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file lsl_stream_outlet.h
 * @since 2026
 * @date  March 2026
 * @brief Declares stream_outlet, the server side of an LSL stream that publishes samples over TCP and advertises itself via UDP multicast.
 *
 * A @ref LSLLIB::stream_outlet is the producer counterpart of
 * @ref LSLLIB::stream_inlet: instantiating it immediately binds a
 * @c QTcpServer on an OS-assigned port (which is written back into
 * the contained @ref LSLLIB::stream_info so callers can read it via
 * @ref info), starts a background worker that periodically multicasts
 * the stream's serialised metadata on the LSL discovery channel, and
 * accepts incoming inlet connections so they can be fed from the
 * outlet's internal sample queue.
 *
 * The public API is the standard LSL @c push_sample / @c push_chunk
 * pair plus @c have_consumers for back-pressure decisions in
 * acquisition loops. As with the inlet, all Qt network and threading
 * machinery is hidden behind a PIMPL so this header stays
 * lightweight and the implementation is free to evolve (for example,
 * to swap the broadcast scheme or add a peer-discovery cache)
 * without rippling through every translation unit that produces LSL
 * data in mne-cpp.
 */

#ifndef LSL_STREAM_OUTLET_H
#define LSL_STREAM_OUTLET_H

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

//=============================================================================================================
// DEFINE NAMESPACE LSLLIB
//=============================================================================================================

namespace LSLLIB {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

/**
 * @brief PIMPL backend of stream_outlet that owns the QTcpServer, the UDP multicast announcer and the producer-side sample queue.
 */
class StreamOutletPrivate;

//=============================================================================================================
/**
 * @brief Server-side LSL endpoint: binds a TCP data server, advertises the stream on the LSL discovery multicast group, and fans samples out to every connected inlet.
 */
class LSLSHARED_EXPORT stream_outlet
{
public:
    //=========================================================================================================
    /**
     * Construct a stream_outlet for the given stream_info.
     *
     * This immediately starts the TCP data server and begins UDP discovery broadcasts.
     *
     * @param[in] info  The stream_info describing this outlet's stream.
     */
    explicit stream_outlet(const stream_info& info);

    //=========================================================================================================
    /**
     * Destructor. Stops broadcasting and closes all connections.
     */
    ~stream_outlet();

    //=========================================================================================================
    /**
     * Push a single multichannel sample into the outlet.
     *
     * @param[in] sample  A vector of channel values (must match the stream's channel_count).
     */
    void push_sample(const std::vector<float>& sample);

    //=========================================================================================================
    /**
     * Push a chunk of multichannel samples into the outlet.
     *
     * @param[in] chunk  A vector of samples, where each sample is a vector of channel values.
     */
    void push_chunk(const std::vector<std::vector<float>>& chunk);

    //=========================================================================================================
    /**
     * Get the stream_info associated with this outlet (including the assigned data port).
     *
     * @return The stream_info for this outlet.
     */
    [[nodiscard]] stream_info info() const;

    //=========================================================================================================
    /**
     * Check whether any consumers are currently connected.
     *
     * @return True if at least one inlet is connected.
     */
    [[nodiscard]] bool have_consumers() const;

private:
    /** Opaque implementation pointer (PIMPL). */
    std::unique_ptr<StreamOutletPrivate> m_pImpl;
};

} // namespace LSLLIB

#endif // LSL_STREAM_OUTLET_H
