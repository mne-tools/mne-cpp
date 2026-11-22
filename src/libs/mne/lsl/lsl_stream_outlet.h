//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_outlet.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Declares stream_outlet, the server side of an LSL stream that publishes samples over TCP and answers discovery queries by UDP.
 *
 * A @ref mne::stream_outlet is the producer counterpart of
 * @ref mne::stream_inlet "stream_inlet": instantiating it immediately binds a
 * TCP server on an OS-assigned port (which is written back into
 * the contained @ref mne::stream_info so callers can read it via
 * @ref mne::stream_outlet::info "info"), starts a background worker that answers
 * liblsl discovery and time-correction queries by UDP, and serves
 * inlets (of this library or of liblsl) over TCP with liblsl's data
 * protocol 1.10 from the outlet's internal sample queue.
 *
 * The public API is the standard LSL @c push_sample / @c push_chunk
 * pair plus @c have_consumers for back-pressure decisions in
 * acquisition loops. As with the inlet, all socket and threading
 * machinery is hidden behind a PIMPL so this header stays
 * lightweight and the implementation is free to evolve (for example,
 * to add IPv6 or a peer-discovery cache)
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
#include <type_traits>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

/**
 * @brief PIMPL backend of stream_outlet that owns the TCP server, the UDP query responders and the producer-side sample queue.
 */
class StreamOutletPrivate;

//=============================================================================================================
/**
 * @brief Server-side LSL endpoint: binds a TCP data server, answers LSL discovery queries, and fans samples out to every connected inlet.
 *
 * @snippet ex_lsl/main.cpp lsl_stream_outlet_usage
 */
class LSLSHARED_EXPORT stream_outlet
{
public:
    //=========================================================================================================
    /**
     * Construct a stream_outlet for the given stream_info.
     *
     * This immediately starts the TCP data server and the UDP query responders.
     *
     * @param[in] info  The stream_info describing this outlet's stream.
     */
    explicit stream_outlet(const stream_info& info);

    //=========================================================================================================
    /**
     * Destructor. Stops answering queries and closes all connections.
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
     * Push a chunk of double samples; double64 and int32 streams carry them without loss through float.
     *
     * @param[in] chunk  A vector of samples, where each sample is a vector of channel values.
     */
    void push_chunk_double(const std::vector<std::vector<double>>& chunk);

    //=========================================================================================================
    /**
     * Push a single sample of any numeric type, as liblsl's push_sample<T>; float takes the overload above.
     *
     * @tparam T         Arithmetic value type; values are converted to the stream's channel format.
     * @param[in] sample A vector of channel values (must match the stream's channel_count).
     */
    template<typename T>
    void push_sample(const std::vector<T>& sample)
    {
        push_chunk(std::vector<std::vector<T>>{sample});
    }

    //=========================================================================================================
    /**
     * Push a chunk of samples of any numeric type, as liblsl's push_chunk<T>; float takes the overload above.
     *
     * @tparam T         Arithmetic value type; values are converted to the stream's channel format.
     * @param[in] chunk  A vector of samples, where each sample is a vector of channel values.
     */
    template<typename T>
    void push_chunk(const std::vector<std::vector<T>>& chunk)
    {
        static_assert(std::is_arithmetic_v<T>, "mne::stream_outlet::push_chunk<T>: T must be a numeric type");
        std::vector<std::vector<double>> values;
        values.reserve(chunk.size());
        for (const std::vector<T>& sample : chunk) {
            values.emplace_back(sample.begin(), sample.end());
        }
        push_chunk_double(values);
    }

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

} // namespace mne

#endif // LSL_STREAM_OUTLET_H
