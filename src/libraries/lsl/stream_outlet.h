//=============================================================================================================
/**
 * @file     stream_outlet.h
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
 * @brief    Contains the declaration of the stream_outlet class.
 *
 */

#ifndef LSL_STREAM_OUTLET_H
#define LSL_STREAM_OUTLET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsl_global.h"
#include "stream_info.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE lsl
//=============================================================================================================

namespace LSLLIB {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

/**
 * @brief Private PIMPL implementation class for stream_outlet, managing the TCP server, UDP discovery broadcasts, and sample queue.
 */
class StreamOutletPrivate;

//=============================================================================================================
/**
 * @brief A stream outlet to publish data on the network.
 *
 * A stream_outlet creates a TCP server that stream_inlet instances can connect to, and periodically
 * broadcasts its stream_info via UDP multicast so that resolve_streams() can discover it.
 *
 * Data is pushed to connected inlets via push_sample() or push_chunk().
 *
 * This class is API-compatible with the liblsl stream_outlet to serve as a drop-in replacement.
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
    stream_info info() const;

    //=========================================================================================================
    /**
     * Check whether any consumers are currently connected.
     *
     * @return True if at least one inlet is connected.
     */
    bool have_consumers() const;

private:
    /** Opaque implementation pointer (PIMPL). */
    std::unique_ptr<StreamOutletPrivate> m_pImpl;
};

} // namespace LSLLIB

#endif // LSL_STREAM_OUTLET_H
