//=============================================================================================================
/**
 * @file     stream_inlet.h
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
 * @brief    Contains the declaration of the stream_inlet class.
 *
 */

#ifndef LSL_STREAM_INLET_H
#define LSL_STREAM_INLET_H

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
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE lsl
//=============================================================================================================

namespace lsl {

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class StreamInletPrivate;

//=============================================================================================================
/**
 * @brief A stream inlet to receive data from a stream_outlet on the network.
 *
 * The stream_inlet connects to an outlet (described by a stream_info obtained from resolve_streams)
 * over TCP and reads data from it. Data is received as chunks of multichannel float samples.
 *
 * This class is API-compatible with the liblsl stream_inlet to serve as a drop-in replacement.
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
    bool samples_available();

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
    std::vector<std::vector<T>> pull_chunk()
    {
        static_assert(std::is_same<T, float>::value,
                      "lsl::stream_inlet::pull_chunk<T>: only float is supported");
        return pull_chunk_float();
    }

    //=========================================================================================================
    /**
     * Pull a chunk of float data (non-template version).
     *
     * @return A vector of samples, where each sample is a vector of float channel values.
     */
    std::vector<std::vector<float>> pull_chunk_float();

private:
    /** Opaque implementation pointer (PIMPL). */
    std::unique_ptr<StreamInletPrivate> m_pImpl;
};

} // namespace lsl

#endif // LSL_STREAM_INLET_H
