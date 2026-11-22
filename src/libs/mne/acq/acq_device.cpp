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

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
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
    std::mutex controlMutex; // serializes control calls with start(), stop() and close(); taken before mutex
    mutable std::mutex mutex;
    std::unique_ptr<AcqBackend> backend;
    std::vector<AcqControlInfo> controls;
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
// DEFINE LOCAL METHODS
//=============================================================================================================

namespace
{

/** @return Whether @p value is a valid value of type @p type for @p info (options and range included). */
bool fits(const AcqControlInfo& info, AcqValueType type, const AcqValue& value)
{
    switch (type) {
        case AcqValueType::None:
            return std::holds_alternative<std::monostate>(value);
        case AcqValueType::Bool:
            return std::holds_alternative<bool>(value);
        case AcqValueType::Int:
            return std::holds_alternative<std::int64_t>(value) &&
                static_cast<double>(std::get<std::int64_t>(value)) >= info.minimum &&
                static_cast<double>(std::get<std::int64_t>(value)) <= info.maximum;
        case AcqValueType::Double: {
            if (!std::holds_alternative<double>(value) && !std::holds_alternative<std::int64_t>(value)) {
                return false;
            }
            const double number = std::holds_alternative<double>(value) ? std::get<double>(value)
                                                                        : static_cast<double>(std::get<std::int64_t>(value));
            return number >= info.minimum && number <= info.maximum; // NaN fails
        }
        case AcqValueType::String:
            return std::holds_alternative<std::string>(value);
        case AcqValueType::Enum:
            return std::holds_alternative<std::string>(value) &&
                std::find(info.options.begin(), info.options.end(), std::get<std::string>(value)) != info.options.end();
    }
    return false;
}

/** @return @p value as the type the control declares (an integer given for a Double becomes a double). */
AcqValue normalized(AcqValueType type, const AcqValue& value)
{
    if (type == AcqValueType::Double && std::holds_alternative<std::int64_t>(value)) {
        return static_cast<double>(std::get<std::int64_t>(value));
    }
    return value;
}

/** @return Whether a control that may run in states @p when may run in @p state. */
bool allowed(std::uint8_t when, AcqDeviceState state)
{
    switch (state) {
        case AcqDeviceState::Opened:
            return (when & AcqWhenOpened) != 0;
        case AcqDeviceState::Running:
            return (when & AcqWhenRunning) != 0;
        case AcqDeviceState::Stopped:
            return (when & AcqWhenStopped) != 0;
        default:
            return false;
    }
}

} // namespace

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
    m_pImpl->controls = backend->controls();
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
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex); // a control call never sees the state change under it
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
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex);
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
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex); // no control call is inside the backend
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

//=============================================================================================================

std::span<const AcqControlInfo> AcqDevice::controls() const noexcept
{
    return m_pImpl ? std::span<const AcqControlInfo>(m_pImpl->controls) : std::span<const AcqControlInfo>();
}

//=============================================================================================================

std::optional<std::size_t> AcqDevice::findControl(std::string_view id) const noexcept
{
    const auto all = controls();
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].id == id) {
            return i;
        }
    }
    return std::nullopt;
}

//=============================================================================================================

AcqExpected<AcqValue> AcqDevice::read(std::string_view id)
{
    const auto index = findControl(id);
    if (!index) {
        return AcqError(AcqErrorCode::InvalidParameter, "no control " + std::string(id));
    }
    if (m_pImpl->controls[*index].kind == AcqControlKind::Command) {
        return AcqError(AcqErrorCode::InvalidParameter, std::string(id) + " is a command; invoke it");
    }
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex);
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mutex);
        const AcqDeviceState state = m_pImpl->effectiveState();
        if (!m_pImpl->backend || state == AcqDeviceState::Failed) {
            return AcqErrorCode::InvalidState;
        }
    }
    return m_pImpl->backend->readControl(*index);
}

//=============================================================================================================

AcqError AcqDevice::write(std::string_view id, const AcqValue& value)
{
    const auto index = findControl(id);
    if (!index) {
        return {AcqErrorCode::InvalidParameter, "no control " + std::string(id)};
    }
    const AcqControlInfo& info = m_pImpl->controls[*index];
    if (info.kind != AcqControlKind::Parameter || !info.writable) {
        return {AcqErrorCode::InvalidParameter, std::string(id) + " cannot be set"};
    }
    if (!fits(info, info.type, value)) {
        return {AcqErrorCode::InvalidParameter, std::string(id) + ": the value has the wrong type or is out of range"};
    }
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex);
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mutex);
        if (!m_pImpl->backend || !allowed(info.when, m_pImpl->effectiveState())) {
            return {AcqErrorCode::InvalidState, std::string(id) + " cannot be set in this state"};
        }
    }
    return m_pImpl->backend->writeControl(*index, normalized(info.type, value));
}

//=============================================================================================================

AcqExpected<AcqValue> AcqDevice::invoke(std::string_view id, std::span<const AcqValue> arguments,
                                        std::chrono::milliseconds timeout)
{
    const auto index = findControl(id);
    if (!index) {
        return AcqError(AcqErrorCode::InvalidParameter, "no control " + std::string(id));
    }
    const AcqControlInfo& info = m_pImpl->controls[*index];
    if (info.kind != AcqControlKind::Command) {
        return AcqError(AcqErrorCode::InvalidParameter, std::string(id) + " is not a command");
    }
    if (arguments.size() != info.arguments.size()) {
        return AcqError(AcqErrorCode::InvalidParameter,
                        std::string(id) + " takes " + std::to_string(info.arguments.size()) + " arguments");
    }
    std::vector<AcqValue> checked;
    checked.reserve(arguments.size());
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (!fits(info, info.arguments[i], arguments[i])) {
            return AcqError(AcqErrorCode::InvalidParameter, std::string(id) + ": argument " + std::to_string(i + 1) + " has the wrong type or is out of range");
        }
        checked.push_back(normalized(info.arguments[i], arguments[i]));
    }
    std::lock_guard<std::mutex> control(m_pImpl->controlMutex);
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mutex);
        if (!m_pImpl->backend || !allowed(info.when, m_pImpl->effectiveState())) {
            return AcqError(AcqErrorCode::InvalidState, std::string(id) + " cannot run in this state");
        }
    }
    return m_pImpl->backend->invokeControl(*index, checked, timeout);
}

//=============================================================================================================

std::size_t AcqDevice::pollControlEvents(std::span<AcqControlEvent> out)
{
    if (!m_pImpl) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    if (!m_pImpl->backend) {
        return 0;
    }
    auto& queue = detail::BackendAccess::state(*m_pImpl->backend).controlEvents;
    std::size_t n = 0;
    while (n < out.size() && queue.pop(out[n])) {
        ++n;
    }
    return n;
}

//=============================================================================================================

std::uint64_t AcqDevice::droppedControlEvents() const
{
    if (!m_pImpl) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    return m_pImpl->backend ? detail::BackendAccess::state(*m_pImpl->backend).controlEvents.dropped() : 0;
}
