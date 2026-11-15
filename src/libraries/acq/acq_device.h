//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_device.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqDevice: an opened device, file or network endpoint with its input and output streams (control path).
 *
 * Lifecycle: open() -> Opened -> start() -> Running -> stop() -> Stopped -> (start() again) ...
 * -> close() -> Closed. stop() joins the backend threads (output streams emit their underrun fill
 * from then on), close() releases the device, the destructor equals close(). A backend error moves the source to Failed; lastError() names it.
 * Streams and readers obtained from a source stay valid after close() and report EndOfStream.
 */

#ifndef ACQ_DEVICE_H
#define ACQ_DEVICE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_backend.h"
#include "acq_error.h"
#include "acq_stream.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * State of a source.
 */
enum class AcqDeviceState : std::uint8_t
{
    Opened = 0,  /**< Device connected and described; streams exist; no data flows. */
    Running = 1, /**< Acquiring. */
    Stopped = 2, /**< Acquisition stopped; can be restarted. */
    Failed = 3,  /**< The backend reported a fatal error; only close() is allowed. */
    Closed = 4   /**< Device released. */
};

//=============================================================================================================
/**
 * Snapshot of a source and its streams (control path, allocates).
 */
struct AcqDeviceStats
{
    AcqDeviceState state = AcqDeviceState::Closed; /**< State at the time of the snapshot. */
    std::vector<AcqStreamStats> streams;           /**< One entry per stream. */
};

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * An opened source. Move-only handle that owns its backend.
 *
 * @snippet ex_acq_oddball/main.cpp acq_device_open
 */
class ACQSHARED_EXPORT AcqDevice
{
public:
    //=========================================================================================================
    /**
     * Opens a source through AcqDeviceRegistry, e.g. "synthetic://", "fiff:///data/raw.fif?realtime=1".
     *
     * @param[in] uri   Source URI.
     *
     * @return The opened source, or InvalidUri, UnknownScheme, InvalidParameter, BackendUnavailable, ...
     */
    [[nodiscard]] static AcqExpected<AcqDevice> open(std::string_view uri);

    /**
     * Wraps an already opened backend.
     *
     * @param[in] backend   The backend. @pre backend != nullptr
     */
    explicit AcqDevice(std::unique_ptr<AcqBackend> backend);

    AcqDevice() noexcept;
    ~AcqDevice(); /**< Equals close(). */
    AcqDevice(AcqDevice&& other) noexcept;
    AcqDevice& operator=(AcqDevice&& other) noexcept;
    AcqDevice(const AcqDevice&) = delete;
    AcqDevice& operator=(const AcqDevice&) = delete;

    /** Starts acquisition and output. @return InvalidState unless Opened or Stopped, or the backend's error. */
    AcqError start();

    /**
     * Stops acquisition and output and joins the backend threads. No-op unless Running.
     *
     * @return The backend's error, or success.
     */
    AcqError stop();

    /** Stops if needed and releases the device. Idempotent. */
    void close();

    /** @return The current state. */
    [[nodiscard]] AcqDeviceState state() const noexcept;

    /** @return All streams of the source. */
    [[nodiscard]] std::span<const AcqStream> streams() const noexcept;

    /**
     * @param[in] index   Stream index; 0 is the primary stream.
     *
     * @return The stream, or an invalid stream if out of range.
     */
    [[nodiscard]] AcqStream stream(std::size_t index = 0) const;

    /**
     * @param[in] name   Stream name.
     *
     * @return The stream, or an invalid stream if there is none.
     */
    [[nodiscard]] AcqStream findStream(std::string_view name) const;

    /** @return Description of the source. */
    [[nodiscard]] AcqDeviceDescription description() const;

    /** @return State and per-stream counters. */
    [[nodiscard]] AcqDeviceStats stats() const;

    //=========================================================================================================
    /**
     * Measures the electrode impedances (control path, allocates). Not while Running: amplifiers inject a
     * test current for it.
     *
     * @param[in] timeout   Maximum duration of the measurement.
     *
     * @return One entry per electrode channel, or InvalidState (Running, Failed, Closed), NotSupported
     *         (the device cannot measure impedances), or the backend's error.
     */
    [[nodiscard]] AcqExpected<std::vector<AcqImpedance>> measureImpedances(
        std::chrono::milliseconds timeout = std::chrono::seconds(10));

    /** @return The last error reported by the backend or a failed transition; success if none. */
    [[nodiscard]] AcqError lastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};
ACQ_STD_MEMBERS_END

} // namespace mne

#endif // ACQ_DEVICE_H
