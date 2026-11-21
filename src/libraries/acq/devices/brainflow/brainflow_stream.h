//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     brainflow_stream.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    BrainFlow's rows as an acquisition stream, and BrainFlow's multicast streamer.
 *
 * A BrainFlow board delivers samples of num_rows doubles: EEG and other channels in the board's units
 * (microvolts for ExG), a package counter, the timestamp in seconds since 1970 and a marker. Its streamer sends
 * them to a UDP address as datagrams of whole samples, double after double in the sender's byte order.
 */

#ifndef BRAINFLOW_STREAM_H
#define BRAINFLOW_STREAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainflow_library.h"
#include "../acq_devices_global.h"
#include "../../acq_stream.h"
#include <com/udp_socket.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::detail
//=============================================================================================================

namespace mne::detail
{

/**
 * @param[in] board   A BrainFlow board id or name, e.g. -1 or "synthetic".
 * @return The board's id and description from the table generated from BrainFlow, nullopt for a board it does not
 *         hold.
 */
ACQDEVICESSHARED_EXPORT std::optional<std::pair<int, BrainFlowDescription>> builtin_brainflow_board(std::string_view board);

/**
 * @param[in] board   A BrainFlow board id or name.
 * @return The board's id and description: from the generated table, else from BrainFlow if it is installed.
 */
ACQDEVICESSHARED_EXPORT AcqExpected<std::pair<int, BrainFlowDescription>> brainflow_board(std::string_view board);

/** @return The places to look for BrainFlow's BoardController: MNE_CPP_BRAINFLOW_LIB, pip's brainflow, the system. */
std::vector<std::string> brainflow_library_candidates();

/**
 * @param[in] description   The board.
 * @param[in] sourceId      The stream's source id.
 * @return The stream of the board: a double channel per row, ExG in volts.
 */
AcqStreamInfo brainflow_stream_info(const BrainFlowDescription& description, std::string sourceId);

/** Writes samples of BrainFlow's rows into an input stream, with the marker row as events and the timestamp row as clock. */
class BrainFlowRowWriter
{
public:
    BrainFlowRowWriter() = default;
    BrainFlowRowWriter(AcqWriter& writer, const BrainFlowDescription& description);

    /** Anchors the clock at the next write, as after a start. */
    void restart() noexcept
    {
        m_clocked = false;
    }

    /** Ends the stream after the samples written. */
    void finish() noexcept
    {
        m_writer->finish();
    }

    /**
     * Writes @p samples samples; row r of sample s is data[s * sampleStride + r * rowStride].
     *
     * @param[in] data           The samples.
     * @param[in] samples        Samples to write.
     * @param[in] sampleStride   Doubles from a sample to the next.
     * @param[in] rowStride      Doubles from a row to the next.
     * @param[in] stopping       Whether to give up waiting for space.
     * @return Success, DeviceStopped if @p stopping became true while waiting, or the writer's error.
     */
    template<typename Stopping>
    AcqError write(const double* data, std::size_t samples, std::size_t sampleStride, std::size_t rowStride, Stopping stopping)
    {
        AcqExpected<AcqWriteView> view = AcqError(AcqErrorCode::Timeout);
        while (!view && view.error() == AcqErrorCode::Timeout) {
            if (stopping()) {
                return AcqError(AcqErrorCode::DeviceStopped);
            }
            view = m_writer->beginWrite(samples, std::chrono::milliseconds(2)); // a gating reader may hold the space
        }
        if (!view) {
            return view.error();
        }
        return commit(*view, data, samples, sampleStride, rowStride);
    }

private:
    AcqError commit(const AcqWriteView& view, const double* data, std::size_t samples, std::size_t sampleStride, std::size_t rowStride);
    void publishClock(std::uint64_t sample, double timestamp);

    AcqWriter* m_writer = nullptr;
    std::size_t m_rows = 0;
    double m_sfreq = 0.0;
    std::optional<std::size_t> m_timestamp_row;
    std::optional<std::size_t> m_marker_row;
    bool m_clocked = false;
    std::chrono::steady_clock::time_point m_last_clock;
};

/**
 * Sends samples in a board's layout as BrainFlow's streamer does: @p batch samples per datagram. The EEG rows get
 * the values given, the package row counts samples modulo 256, and the marker and timestamp rows get theirs.
 */
class ACQDEVICESSHARED_EXPORT BrainFlowStreamer
{
public:
    /**
     * @param[in] description   The board whose layout to send.
     * @param[in] address       Destination: a multicast group as BrainFlow's STREAMING_BOARD reads it, or a host.
     * @param[in] port          Destination port.
     * @param[in] batch         Samples per datagram; BrainFlow's receiver expects BRAINFLOW_BATCH_SIZE (default 3).
     * @return The streamer, or DeviceIo if no socket opens.
     */
    [[nodiscard]] static AcqExpected<BrainFlowStreamer> open(const BrainFlowDescription& description, std::string address, std::uint16_t port,
                                                             std::size_t batch);

    /**
     * Adds a sample and sends the datagram once it holds a batch.
     *
     * @param[in] eeg         Values of the first EEG rows, in BrainFlow's units (microvolts).
     * @param[in] marker      Marker of the sample, 0 for none.
     * @param[in] timestamp   Seconds since 1970.
     * @return Success, or DeviceIo naming the socket's error.
     */
    AcqError push(std::span<const double> eeg, double marker, double timestamp);

    /** @return Rows of the EEG channels. */
    [[nodiscard]] std::span<const int> eeg_rows() const noexcept
    {
        return m_eeg_rows;
    }

private:
    mne::UdpSocket m_socket;
    std::string m_address;
    std::uint16_t m_port = 0;
    std::size_t m_rows = 0;
    std::size_t m_batch = 0;
    std::vector<int> m_eeg_rows;
    int m_package_row = -1;
    int m_timestamp_row = -1;
    int m_marker_row = -1;
    std::uint64_t m_sample = 0;
    std::vector<double> m_datagram;
};

} // namespace mne::detail

#endif // BRAINFLOW_STREAM_H
