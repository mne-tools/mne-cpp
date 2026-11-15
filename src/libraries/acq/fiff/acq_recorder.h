//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_recorder.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqRecorder: records any stream to FIFF (optionally as BIDS) while it is acquired.
 *
 * The recorder consumes the stream through its own Lossless reader, so it never slows the producer; if the
 * disk cannot keep up, the reader loses samples instead and the recorder writes them as zeros under a
 * BAD_ACQ_SKIP annotation, which keeps the file's sample axis that of the stream.
 *
 * Samples keep their native format: int16 is stored as FIFFT_DAU_PACK16, packed int24 and int32 as FIFFT_INT,
 * float32 as FIFFT_FLOAT and float64 as FIFFT_DOUBLE, each bit-exact. FIFF has one calibration per channel
 * and no offset, so mne.io.read_raw_fif returns raw * float32(scale * 10^unitExponent) for every channel;
 * a stream with a non-zero offset or digital bits in its words is stored as float64 physical values
 * (raw * scale + offset) with that power of ten as calibration instead. The calibration is that of the
 * stream's metadata when recording starts.
 *
 * Events become annotations (label from AcqStreamInfo::eventLabels, or the code) and, when the stream has
 * no stim channel of its own, the extra stim channel "STI 101" carrying each code for the event's duration.
 * The measurement date is the UTC time of stream sample 0 from the stream's clock model; the device and
 * the clock provenance (clock domain, timebase and lock state, clock source) go into the device info and
 * the description of the measurement info.
 *
 * File layout: every FIFF data buffer is one block of whole 4 KiB pages, written with aligned direct I/O
 * and flushed before the previous block's end marker is turned into a link to it. A file cut off by a crash
 * or power loss therefore ends at the last complete buffer and reads without warnings. Files are split
 * at AcqRecorderOptions::splitBytes into linked parts named like those of MNE-Python (raw.fif,
 * raw-1.fif, ...; split-01, split-02 for BIDS); annotations are written into the first part on stop().
 *
 * @code
 * auto device = mne::AcqDevice::open("synthetic://?eeg=32&sfreq=1000");
 * mne::AcqRecorderOptions options;
 * options.device = device->description();
 * auto recorder = mne::AcqRecorder::start(device->stream(), "/data/session_raw.fif", options);
 * device->start();
 * // ... acquire ...
 * device->stop();
 * recorder->stop(); // everything committed until now is on disk; mne.io.read_raw_fif reads it
 * @endcode
 */

#ifndef ACQ_RECORDER_H
#define ACQ_RECORDER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_fiff_global.h"
#include "../acq_backend.h"
#include "../acq_error.h"
#include "../acq_stream.h"
#include "../acq_thread.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Where a BIDS recording goes: root/sub-&lt;subject&gt;[/ses-&lt;session&gt;]/meg/..._meg.fif with its sidecars.
 * BIDS stores FIFF under the meg datatype, whatever the channel types.
 *
 * @snippet ex_acq_record/main.cpp acq_record_bids
 */
struct AcqBidsPath
{
    std::string root;        /**< Dataset root; created if missing. */
    std::string subject;     /**< Subject label without "sub-". */
    std::string session;     /**< Session label without "ses-"; empty for none. */
    std::string task;        /**< Task label without "task-". */
    std::string run;         /**< Run label without "run-"; empty for none. */
    std::string datasetName; /**< Name in a newly created dataset_description.json. */
};

//=============================================================================================================
/**
 * Options of a recording; fixed at start().
 *
 * @snippet ex_acq_record/main.cpp acq_record_fiff
 */
struct AcqRecorderOptions
{
    std::size_t bufferSamples = 0;         /**< Samples per FIFF data buffer, the unit flushed to disk; 0 = 1 s. */
    std::uint64_t splitBytes = 2147483648; /**< Size at which the file is split, at most 2 GiB (FIFF positions are int32). */
    bool directIo = true;                  /**< Bypass the page cache where the file system allows it. */
    AcqDeviceDescription device;           /**< Source, for the device info of the file, e.g. AcqDevice::description(). */
    AcqThreadPolicy thread;                /**< Placement and priority of the recorder's reader thread. */
};

//=============================================================================================================
/**
 * Progress of a recording; exact once stop() has returned.
 *
 * @snippet ex_acq_record/main.cpp acq_record_fiff
 */
struct AcqRecorderStats
{
    std::uint64_t firstSample = 0; /**< Stream sample index of the first recorded sample (FIFF first_samp). */
    std::uint64_t samples = 0;     /**< Samples per channel flushed to disk, including zero-filled lost ones. */
    std::uint64_t lost = 0;        /**< Samples the reader lost or saw overwritten, stored as zeros under BAD_ACQ_SKIP. */
    std::uint64_t events = 0;      /**< Events recorded. */
    std::uint64_t buffers = 0;     /**< FIFF data buffers flushed to disk. */
    std::uint64_t bytes = 0;       /**< Bytes written to all files. */
    std::size_t files = 0;         /**< Files started (split parts). */
    bool directIo = false;         /**< Whether the page cache is bypassed. */
};

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * Records one stream to FIFF on its own threads (reader and writer, double-buffered). Move-only; the
 * destructor equals stop().
 *
 * @snippet ex_acq_record/main.cpp acq_record_fiff
 * @snippet ex_acq_record/main.cpp acq_record_bids
 */
class ACQFIFFSHARED_EXPORT AcqRecorder
{
public:
    //=========================================================================================================
    /**
     * Starts recording @p stream to @p path from its next sample; the measurement info is on disk on return.
     *
     * @param[in] stream    An input or output stream of real, continuous samples in host memory.
     * @param[in] path      FIFF file to create; further split parts are created next to it.
     * @param[in] options   Buffering, splitting, I/O mode, device description, thread policy.
     *
     * @return The recorder, or UnsupportedFormat (complex or segmented stream), DeviceMemory (the ring is in
     *         GPU memory), InvalidParameter (splitBytes out of range), TooManyReaders, or DeviceIo.
     */
    [[nodiscard]] static AcqExpected<AcqRecorder> start(const AcqStream& stream, const std::string& path,
                                                        const AcqRecorderOptions& options = {});

    //=========================================================================================================
    /**
     * Starts recording @p stream into a BIDS dataset; stop() writes the sidecars (channels.tsv, events.tsv,
     * meg.json, dataset_description.json) as mne-bids does.
     *
     * @param[in] stream    As start(const AcqStream&, const std::string&, const AcqRecorderOptions&).
     * @param[in] bids      Dataset root and entities.
     * @param[in] options   As start(const AcqStream&, const std::string&, const AcqRecorderOptions&).
     *
     * @return As start(const AcqStream&, const std::string&, const AcqRecorderOptions&), or InvalidParameter
     *         for a missing root, subject or task, or a label that is not alphanumeric.
     */
    [[nodiscard]] static AcqExpected<AcqRecorder> start(const AcqStream& stream, const AcqBidsPath& bids,
                                                        const AcqRecorderOptions& options = {});

    AcqRecorder() noexcept;
    ~AcqRecorder();
    AcqRecorder(AcqRecorder&& other) noexcept;
    AcqRecorder& operator=(AcqRecorder&& other) noexcept;
    AcqRecorder(const AcqRecorder&) = delete;
    AcqRecorder& operator=(const AcqRecorder&) = delete;

    /** @return true from start() until stop(), or until the stream ended and everything is on disk. */
    [[nodiscard]] bool isRecording() const noexcept;

    //=========================================================================================================
    /**
     * Records every sample committed until now, writes the annotations and closes the files (and writes
     * the BIDS sidecars). Idempotent.
     *
     * @return The first error of the recording (DeviceIo), or success.
     */
    AcqError stop();

    /** @return Progress counters. */
    [[nodiscard]] AcqRecorderStats stats() const noexcept;

    /** @return The files written so far, in split order. */
    [[nodiscard]] std::vector<std::string> files() const;

private:
    struct Impl;
    explicit AcqRecorder(std::unique_ptr<Impl> impl) noexcept;

    std::unique_ptr<Impl> m_pImpl;
};
ACQ_STD_MEMBERS_END

} // namespace mne

#endif // ACQ_RECORDER_H
