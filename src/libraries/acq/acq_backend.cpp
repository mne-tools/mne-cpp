//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_backend.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqBackend definitions: stream creation while opening and fatal-error reporting.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_backend.h"

#include "detail/acq_backend_impl.h"
#include "detail/acq_stream_core.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqBackend::AcqBackend()
: m_pImpl(std::make_unique<Impl>())
{
}

AcqBackend::~AcqBackend() = default;

void AcqBackend::close()
{
}

AcqExpected<std::vector<AcqImpedance>> AcqBackend::measureImpedances(std::chrono::milliseconds /*timeout*/)
{
    return AcqErrorCode::NotSupported;
}

//=============================================================================================================

std::vector<AcqControlInfo> AcqBackend::controls() const
{
    return {};
}

//=============================================================================================================

AcqExpected<AcqValue> AcqBackend::readControl(std::size_t /*index*/)
{
    return AcqErrorCode::NotSupported;
}

//=============================================================================================================

AcqError AcqBackend::writeControl(std::size_t /*index*/, const AcqValue& /*value*/)
{
    return AcqErrorCode::NotSupported;
}

//=============================================================================================================

AcqExpected<AcqValue> AcqBackend::invokeControl(std::size_t /*index*/, std::span<const AcqValue> /*arguments*/,
                                                std::chrono::milliseconds /*timeout*/)
{
    return AcqErrorCode::NotSupported;
}

//=============================================================================================================

void AcqBackend::publishControlEvent(const AcqControlEvent& event) noexcept
{
    AcqControlEvent stamped = event;
    if (stamped.hostNs == 0) {
        stamped.hostNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    static_cast<void>(m_pImpl->controlEvents.push(stamped));
}

//=============================================================================================================

std::span<const AcqStream> AcqBackend::streams() const noexcept
{
    return m_pImpl->streams;
}

//=============================================================================================================

AcqExpected<AcqWriter*> AcqBackend::addInputStream(AcqStreamInfo info, const AcqStreamOptions& options)
{
    if (m_pImpl->sealed) {
        return AcqError(AcqErrorCode::InvalidState, "streams can only be added while opening");
    }
    if (info.direction != AcqDirection::Input) {
        return AcqError(AcqErrorCode::WrongDirection, "addInputStream() needs an input stream");
    }
    auto writer = AcqWriter::create(std::move(info), options);
    if (!writer) {
        return writer.error();
    }
    m_pImpl->writers.push_back(std::make_unique<AcqWriter>(std::move(*writer)));
    m_pImpl->streams.push_back(m_pImpl->writers.back()->stream());
    return m_pImpl->writers.back().get();
}

//=============================================================================================================

AcqExpected<AcqReader*> AcqBackend::addOutputStream(AcqStreamInfo info, const AcqStreamOptions& options, AcqWait wait)
{
    if (m_pImpl->sealed) {
        return AcqError(AcqErrorCode::InvalidState, "streams can only be added while opening");
    }
    if (info.direction != AcqDirection::Output) {
        return AcqError(AcqErrorCode::WrongDirection, "addOutputStream() needs an output stream");
    }
    // The application takes the writer later through AcqStream::writer(), so the core is created without one.
    auto core = detail::AcqStreamCore::create(std::move(info), options);
    if (!core) {
        return core.error();
    }
    AcqStream stream(std::move(*core));
    AcqReaderOptions gating;
    gating.wait = wait;
    gating.gating = true;
    auto reader = stream.reader(AcqReader::ReadMode::Lossless, gating);
    if (!reader) {
        return reader.error();
    }
    m_pImpl->deviceReaders.push_back(std::make_unique<AcqReader>(std::move(*reader)));
    m_pImpl->streams.push_back(std::move(stream));
    return m_pImpl->deviceReaders.back().get();
}

//=============================================================================================================

void AcqBackend::reportError(AcqError error) noexcept
{
    int expected = 0;
    if (m_pImpl->errorState.compare_exchange_strong(expected, 1, std::memory_order_acq_rel)) {
        m_pImpl->error = std::move(error);
        m_pImpl->errorState.store(2, std::memory_order_release);
    }
}
