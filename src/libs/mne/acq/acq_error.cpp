//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_error.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Error catalogue and AcqError definitions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_error.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{
using C = AcqErrorCode;

constexpr std::array<AcqErrorInfo, 29> kCatalogue{{
    {C::None, "None", "Success", "", "acq-errors#none"},
    {C::Timeout, "Timeout", "No data or space became available within the timeout",
     "Retry, use a longer timeout, or check that the device is running", "acq-errors#timeout"},
    {C::EndOfStream, "EndOfStream", "The stream has ended and every sample has been read",
     "Stop reading; open or restart the device for more data", "acq-errors#end-of-stream"},
    {C::Overrun, "Overrun", "The producer overwrote samples while the view was held",
     "Discard results computed from this view; release views sooner or enlarge the ring", "acq-errors#overrun"},
    {C::DeviceStopped, "DeviceStopped", "The device was stopped while waiting", "Start the device again",
     "acq-errors#device-stopped"},
    {C::Late, "Late", "The output targets a sample the device has already played",
     "Schedule at or after earliestWritableSample(), or write further ahead", "acq-errors#late"},
    {C::NotSynchronized, "NotSynchronized", "The streams are not phase-locked in one clock domain",
     "Share a clock between the devices and verify it with sync markers", "acq-errors#not-synchronized"},
    {C::BeforeStart, "BeforeStart", "The mapped instant lies before the first sample of the target stream",
     "Map a later sample or use a positive offset", "acq-errors#before-start"},
    {C::NotOnGrid, "NotOnGrid", "The instant is not on the schedulable sample grid of the target stream",
     "Choose sample rates and delays that are multiples of the stream's sample alignment, or round explicitly",
     "acq-errors#not-on-grid"},
    {C::InvalidUri, "InvalidUri", "The device URI is malformed", "Use the form scheme://target?key=value",
     "acq-errors#invalid-uri"},
    {C::UnknownScheme, "UnknownScheme", "No backend is registered for the URI scheme",
     "List the available schemes with AcqDeviceRegistry::schemes()", "acq-errors#unknown-scheme"},
    {C::InvalidParameter, "InvalidParameter", "A parameter is unknown or out of range",
     "Check the parameter against the documentation of the call or scheme", "acq-errors#invalid-parameter"},
    {C::BackendUnavailable, "BackendUnavailable", "A required runtime library was not found",
     "Install the library or point the documented environment variable at it", "acq-errors#backend-unavailable"},
    {C::UnsupportedVersion, "UnsupportedVersion", "A runtime library has an unsupported version",
     "Install a version inside the supported range", "acq-errors#unsupported-version"},
    {C::DeviceNotFound, "DeviceNotFound", "The device, file or endpoint does not exist or does not answer",
     "Check the connection, path or address", "acq-errors#device-not-found"},
    {C::DeviceIo, "DeviceIo", "The device or file failed during transfer", "Check cabling, power and permissions",
     "acq-errors#device-io"},
    {C::InvalidState, "InvalidState", "The call is not allowed in the current state",
     "Follow the documented call order (e.g. release a view before acquiring the next)", "acq-errors#invalid-state"},
    {C::UnsupportedFormat, "UnsupportedFormat", "The sample format or layout cannot be used here",
     "Use a supported format or convert explicitly", "acq-errors#unsupported-format"},
    {C::ShapeMismatch, "ShapeMismatch", "A caller-owned buffer has the wrong shape",
     "Size the buffer as channels x samples", "acq-errors#shape-mismatch"},
    {C::TooManyReaders, "TooManyReaders", "All reader slots of the stream are in use",
     "Destroy unused readers or raise AcqStreamOptions::maxReaders", "acq-errors#too-many-readers"},
    {C::InvalidBuffer, "InvalidBuffer", "The external buffer is misaligned or not a power of two in samples",
     "Align the buffer to 4096 bytes and size it as a power of two in samples", "acq-errors#invalid-buffer"},
    {C::OutOfMemory, "OutOfMemory", "The ring could not be allocated", "Reduce the capacity or the channel count",
     "acq-errors#out-of-memory"},
    {C::PlatformUnsupported, "PlatformUnsupported", "The backend or option is not available on this platform",
     "Use a supported platform or disable the option", "acq-errors#platform-unsupported"},
    {C::WrongDirection, "WrongDirection", "The call does not match the direction of the stream",
     "Use writer() and consumeIdle() only on output streams, grant() only on input streams",
     "acq-errors#wrong-direction"},
    {C::WriterInUse, "WriterInUse", "The output stream already has its writer",
     "Keep and share the existing AcqWriter", "acq-errors#writer-in-use"},
    {C::DeviceMemory, "DeviceMemory", "The samples live in GPU memory, which the CPU must not access",
     "Process the view on the GPU (e.g. through DLPack) or copy it to host memory there",
     "acq-errors#device-memory"},
    {C::NotSupported, "NotSupported", "The device does not offer this function",
     "Check the device documentation; for impedances, use an amplifier with an impedance mode",
     "acq-errors#not-supported"},
    {C::PermissionDenied, "PermissionDenied", "The operating system refused the request",
     "Grant the right (Linux: CAP_SYS_NICE or rtprio/nice limits in limits.conf) or use a lower priority",
     "acq-errors#permission-denied"},
    {C::Internal, "Internal", "An internal invariant of mne_acq was violated",
     "Please report this with the steps that led to it", "acq-errors#internal"},
}};
} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

const AcqErrorInfo& mne::acqErrorInfo(AcqErrorCode code) noexcept
{
    for (const AcqErrorInfo& info : kCatalogue) {
        if (info.code == code) {
            return info;
        }
    }
    return kCatalogue.back();
}

//=============================================================================================================

AcqError::AcqError(AcqErrorCode code, std::string context)
: m_code(code)
, m_context(std::make_shared<const std::string>(std::move(context)))
{
}

//=============================================================================================================

std::string_view AcqError::context() const noexcept
{
    return m_context ? std::string_view(*m_context) : std::string_view();
}

//=============================================================================================================

std::string AcqError::text() const
{
    const AcqErrorInfo& i = info();
    std::string out(i.message);
    if (m_context && !m_context->empty()) {
        out += " (";
        out += *m_context;
        out += ')';
    }
    if (!i.fix.empty()) {
        out += ". ";
        out += i.fix;
    }
    return out;
}
