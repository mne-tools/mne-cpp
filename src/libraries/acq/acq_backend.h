//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_backend.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    AcqBackend: the interface a device, file or network backend implements behind an AcqDevice.
 *
 * Users hold an AcqDevice; backend authors derive from AcqBackend. While opening, a backend creates
 * its input streams (it gets their writers) and output streams (it gets their gating readers). Between
 * start() and stop() it runs its own acquisition/output thread, interrupt handler or DMA callback.
 * AcqDevice owns the backend and enforces the state machine, so a backend only implements transitions.
 */

#ifndef ACQ_BACKEND_H
#define ACQ_BACKEND_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_global.h"
#include "acq_error.h"
#include "acq_reader.h"
#include "acq_stream.h"
#include "acq_stream_info.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

namespace detail
{
struct BackendAccess;
}

//=============================================================================================================
/**
 * What a source is, for display and provenance.
 *
 * @snippet ex_acq_oddball/main.cpp acq_device_open
 */
struct AcqDeviceDescription
{
    std::string backend;        /**< Backend name, e.g. "synthetic", "brainflow". */
    std::string backendVersion; /**< Version of the runtime-loaded library, empty if none. */
    std::string device;         /**< Device model and serial, file path or stream name. */
    std::string uri;            /**< Canonical URI that reopens this source. */
};

//=============================================================================================================
/**
 * Electrode impedance of one channel.
 *
 * @snippet ex_acq_oddball/main.cpp acq_device_open
 */
struct AcqImpedance
{
    std::size_t stream = 0;  /**< Stream index. */
    std::size_t channel = 0; /**< Channel index within the stream. */
    double ohms = 0.0;       /**< Electrode-skin (or -tissue) impedance magnitude in ohm. */
};

ACQ_STD_MEMBERS_BEGIN
//=============================================================================================================
/**
 * Base class of all backends.
 */
class ACQSHARED_EXPORT AcqBackend
{
public:
    virtual ~AcqBackend();
    AcqBackend(const AcqBackend&) = delete;
    AcqBackend& operator=(const AcqBackend&) = delete;

    /** @return Description of the opened device. */
    [[nodiscard]] virtual AcqDeviceDescription description() const = 0;

    /**
     * Starts acquisition and output. Called in state Opened or Stopped.
     *
     * @return Success, or the error that prevented the start.
     */
    virtual AcqError start() = 0;

    /** Stops and joins every backend thread before returning. Called in state Running. */
    virtual void stop() = 0;

    /** Releases the device. Called once, after stop(). The default does nothing. */
    virtual void close();

    /**
     * Measures electrode impedances. Called in state Opened or Stopped; the default returns NotSupported.
     *
     * @param[in] timeout   Maximum duration of the measurement.
     *
     * @return One entry per measured electrode, or the error.
     */
    [[nodiscard]] virtual AcqExpected<std::vector<AcqImpedance>> measureImpedances(std::chrono::milliseconds timeout);

    /** @return The streams created while opening, in creation order. */
    [[nodiscard]] std::span<const AcqStream> streams() const noexcept;

protected:
    AcqBackend();

    //=========================================================================================================
    /**
     * Creates an input stream. Only while opening.
     *
     * @param[in] info      Stream description; direction must be Input.
     * @param[in] options   Ring options, e.g. an adopted DMA buffer.
     *
     * @return The writer, owned by the backend base and valid until the backend is destroyed.
     */
    AcqExpected<AcqWriter*> addInputStream(AcqStreamInfo info, const AcqStreamOptions& options = {});

    //=========================================================================================================
    /**
     * Creates an output stream. Only while opening.
     *
     * @param[in] info      Stream description; direction must be Output.
     * @param[in] options   Ring options, e.g. the device's DMA transmit buffer.
     * @param[in] wait      How the device reader waits; device loops usually poll with acquire(0, 0).
     *
     * @return The gating device reader, owned by the backend base and valid until the backend is destroyed.
     */
    AcqExpected<AcqReader*> addOutputStream(AcqStreamInfo info, const AcqStreamOptions& options = {},
                                            AcqWait wait = AcqWait::Spin);

    /**
     * Reports a fatal error from any thread; the device moves to state Failed. Allocation-free.
     *
     * @param[in] error   The error.
     */
    void reportError(AcqError error) noexcept;

private:
    friend struct detail::BackendAccess;
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};
ACQ_STD_MEMBERS_END

} // namespace mne

#endif // ACQ_BACKEND_H
