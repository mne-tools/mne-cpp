//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     lsl_stream_inlet.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Declares stream_inlet, the client side of an LSL connection that pulls multichannel sample chunks from a remote outlet.
 *
 * A @ref mne::stream_inlet is constructed from a resolved
 * @ref mne::stream_info (typically obtained via
 * @ref mne::resolve_streams) and represents a one-way TCP
 * connection to the corresponding outlet, of this library or of
 * liblsl, speaking liblsl's data protocol 1.10. The class wraps the
 * TCP connection (mne_com's @c TcpSocket) and the sample decoding behind a
 * PIMPL so that the public header pulls in no socket headers.
 *
 * The data path is intentionally chunked rather than
 * sample-at-a-time: @ref mne::stream_inlet::samples_available "samples_available" drains pending bytes off
 * the socket in a non-blocking pass and @ref mne::stream_inlet::pull_chunk "pull_chunk" returns
 * every fully-received multichannel sample in one call, which is the
 * pattern that real-time acquisition pipelines in mne-cpp (sensor
 * plug-ins under @c applications/mne_scan_plugins) consume most
 * efficiently. The templated @ref mne::stream_inlet::pull_chunk "pull_chunk" mirrors liblsl's; samples
 * are decoded as double, which holds every float32, double64, int32, int16 and int8 value exactly, so
 * @ref mne::stream_inlet::pull_chunk_double "pull_chunk_double" delivers the native values without loss
 * (int64 beyond 2^53 excepted). @ref mne::stream_inlet::pull_chunk_float "pull_chunk_float" rounds to float.
 * @ref mne::stream_inlet::info "info" fetches the outlet's full info, including the channel description.
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

#include <cmath>
#include <cstddef>
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
 * @brief PIMPL backend of stream_inlet that owns the TCP connection and the per-sample reassembly buffer.
 */
class StreamInletPrivate;

//=============================================================================================================
/**
 * @brief Client-side LSL endpoint: connects to a resolved outlet over TCP and exposes pulled samples as float or double chunks.
 *
 * @snippet ex_lsl/main.cpp lsl_stream_inlet_usage
 *
 * @snippet ex_lsl/main.cpp lsl_resolve_stream_usage
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
     * @throws std::runtime_error if the connection cannot be established, the outlet refuses the stream or
     *         its test-pattern samples do not match the protocol.
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
     * Returns all complete samples currently in the buffer, converted to @p T as liblsl's pull_chunk<T> does
     * (integers rounded to the nearest value).
     *
     * @tparam T  The data type: float, double or an integer type.
     * @return    A vector of samples, where each sample is a vector of channel values.
     */
    template<typename T>
    [[nodiscard]] std::vector<std::vector<T>> pull_chunk()
    {
        static_assert(std::is_arithmetic_v<T>, "mne::stream_inlet::pull_chunk<T>: T must be a numeric type");
        if constexpr (std::is_same_v<T, float>) {
            return pull_chunk_float();
        } else if constexpr (std::is_same_v<T, double>) {
            return pull_chunk_double();
        } else {
            std::vector<std::vector<T>> chunk;
            for (const std::vector<double>& sample : pull_chunk_double()) {
                std::vector<T>& out = chunk.emplace_back(sample.size());
                for (std::size_t c = 0; c < sample.size(); ++c) {
                    out[c] = static_cast<T>(std::llround(sample[c]));
                }
            }
            return chunk;
        }
    }

    //=========================================================================================================
    /**
     * Pull a chunk of float data (non-template version).
     *
     * @return A vector of samples, where each sample is a vector of float channel values.
     */
    [[nodiscard]] std::vector<std::vector<float>> pull_chunk_float();

    //=========================================================================================================
    /**
     * Pull a chunk of float data with the timestamp of every sample.
     *
     * @param[out] timestamps   One timestamp per returned sample, in the outlet host's local_clock() seconds.
     *
     * @return A vector of samples, where each sample is a vector of float channel values.
     */
    [[nodiscard]] std::vector<std::vector<float>> pull_chunk_float(std::vector<double>& timestamps);

    //=========================================================================================================
    /**
     * Pull a chunk of double data: the native values of float32, double64, int32, int16 and int8 streams.
     *
     * @return A vector of samples, where each sample is a vector of double channel values.
     */
    [[nodiscard]] std::vector<std::vector<double>> pull_chunk_double();

    //=========================================================================================================
    /**
     * Pull a chunk of double data with the timestamp of every sample.
     *
     * @param[out] timestamps   One timestamp per returned sample, in the outlet host's local_clock() seconds.
     *
     * @return A vector of samples, where each sample is a vector of double channel values.
     */
    [[nodiscard]] std::vector<std::vector<double>> pull_chunk_double(std::vector<double>& timestamps);

    //=========================================================================================================
    /**
     * The full info of the stream, as liblsl's stream_inlet::info(): fetched from the outlet once (fullinfo
     * request), then cached. Unlike the info from discovery it carries the channel description.
     *
     * @param[in] timeout   Maximum wait in seconds.
     *
     * @return The stream info with its channel description.
     *
     * @throws std::runtime_error if the outlet cannot be reached or sends no valid info within @p timeout.
     */
    [[nodiscard]] stream_info info(double timeout = 5.0);

    //=========================================================================================================
    /**
     * Estimates the offset between the outlet host's clock and local_clock() from round trips to the outlet's
     * service port, keeping the one with the shortest round-trip time.
     *
     * @param[in] timeout   Maximum wait in seconds.
     *
     * @return Seconds to add to the outlet's timestamps to express them in local_clock(), or NaN if the outlet
     *         did not answer within @p timeout.
     */
    [[nodiscard]] double time_correction(double timeout = 2.0);

private:
    /** Opaque implementation pointer (PIMPL). */
    std::unique_ptr<StreamInletPrivate> m_pImpl;
};

} // namespace mne

#endif // LSL_STREAM_INLET_H
