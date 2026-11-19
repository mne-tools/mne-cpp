//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_device.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqDevice definitions: the life cycle of an opened source around its backend.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_device.h"

#include "acq_device_registry.h"
#include "detail/acq_backend_impl.h"

#include <mutex>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

struct AcqDevice::Impl
{
    mutable std::mutex mutex;
    std::unique_ptr<AcqBackend> backend;
    AcqDeviceState state = AcqDeviceState::Opened;
    bool backendRunning = false; // start() succeeded and stop() has not yet joined the backend threads
    AcqError lastError;

    [[nodiscard]] bool failed() const noexcept
    {
        return backend && detail::BackendAccess::state(*backend).failed();
    }

    [[nodiscard]] AcqDeviceState effectiveState() const noexcept
    {
        return failed() ? AcqDeviceState::Failed : state;
    }

    [[nodiscard]] AcqError error() const
    {
        return failed() ? detail::BackendAccess::state(*backend).error : lastError;
    }

    void stopBackend()
    {
        if (backendRunning) {
            backend->stop();
            backendRunning = false;
        }
    }
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqExpected<AcqDevice> AcqDevice::open(std::string_view uri)
{
    return AcqDeviceRegistry::instance().open(uri);
}

//=============================================================================================================

AcqDevice::AcqDevice(std::unique_ptr<AcqBackend> backend)
: m_pImpl(std::make_unique<Impl>())
{
    detail::BackendAccess::state(*backend).sealed = true;
    m_pImpl->backend = std::move(backend);
}

AcqDevice::AcqDevice() noexcept = default;

AcqDevice::~AcqDevice()
{
    close();
}

AcqDevice::AcqDevice(AcqDevice&& other) noexcept = default;

AcqDevice& AcqDevice::operator=(AcqDevice&& other) noexcept
{
    if (this != &other) {
        close();
        m_pImpl = std::move(other.m_pImpl);
    }
    return *this;
}

//=============================================================================================================

AcqError AcqDevice::start()
{
    if (!m_pImpl) {
        return AcqErrorCode::InvalidState;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    const AcqDeviceState state = m_pImpl->effectiveState();
    if (state != AcqDeviceState::Opened && state != AcqDeviceState::Stopped) {
        return AcqErrorCode::InvalidState;
    }
    AcqError error = m_pImpl->backend->start();
    if (!error.ok()) {
        m_pImpl->lastError = error;
        return error;
    }
    m_pImpl->backendRunning = true;
    m_pImpl->state = AcqDeviceState::Running;
    return {};
}

//=============================================================================================================

AcqExpected<std::vector<AcqImpedance>> AcqDevice::measureImpedances(std::chrono::milliseconds timeout)
{
    if (!m_pImpl) {
        return AcqErrorCode::InvalidState;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    const AcqDeviceState state = m_pImpl->effectiveState();
    if (state != AcqDeviceState::Opened && state != AcqDeviceState::Stopped) {
        return AcqErrorCode::InvalidState;
    }
    return m_pImpl->backend->measureImpedances(timeout);
}

//=============================================================================================================

AcqError AcqDevice::stop()
{
    if (!m_pImpl) {
        return {};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    m_pImpl->stopBackend();
    if (m_pImpl->state == AcqDeviceState::Running) {
        m_pImpl->state = AcqDeviceState::Stopped;
    }
    return m_pImpl->failed() ? m_pImpl->error() : AcqError();
}

//=============================================================================================================

void AcqDevice::close()
{
    if (!m_pImpl) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    if (!m_pImpl->backend) {
        return;
    }
    m_pImpl->stopBackend();
    m_pImpl->lastError = m_pImpl->error();
    m_pImpl->backend->close();
    // Destroying the backend finishes its input streams: readers drain what was committed, then see EndOfStream.
    m_pImpl->backend.reset();
    m_pImpl->state = AcqDeviceState::Closed;
}

//=============================================================================================================

AcqDeviceState AcqDevice::state() const noexcept
{
    if (!m_pImpl) {
        return AcqDeviceState::Closed;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->effectiveState();
}

//=============================================================================================================

std::span<const AcqStream> AcqDevice::streams() const noexcept
{
    if (!m_pImpl) {
        return {};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->backend ? m_pImpl->backend->streams() : std::span<const AcqStream>();
}

//=============================================================================================================

AcqStream AcqDevice::stream(std::size_t index) const
{
    const auto all = streams();
    return index < all.size() ? all[index] : AcqStream();
}

//=============================================================================================================

AcqStream AcqDevice::findStream(std::string_view name) const
{
    for (const AcqStream& stream : streams()) {
        if (stream.info()->name == name) {
            return stream;
        }
    }
    return {};
}

//=============================================================================================================

AcqDeviceDescription AcqDevice::description() const
{
    if (!m_pImpl) {
        return {};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->backend ? m_pImpl->backend->description() : AcqDeviceDescription();
}

//=============================================================================================================

AcqDeviceStats AcqDevice::stats() const
{
    AcqDeviceStats out;
    out.state = state();
    for (const AcqStream& stream : streams()) {
        out.streams.push_back(stream.stats());
    }
    return out;
}

//=============================================================================================================

AcqError AcqDevice::lastError() const
{
    if (!m_pImpl) {
        return {};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->error();
}
