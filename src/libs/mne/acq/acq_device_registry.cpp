//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_device_registry.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AcqDeviceRegistry definitions: scheme registration, opening by URI and parallel discovery.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_device_registry.h"

#include "detail/acq_synthetic_backend.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <map>
#include <mutex>
#include <thread>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

namespace
{

bool validScheme(std::string_view scheme) noexcept
{
    if (scheme.empty() || scheme[0] < 'a' || scheme[0] > 'z') {
        return false;
    }
    for (const char c : scheme) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == '_';
        if (!ok) {
            return false;
        }
    }
    return true;
}

} // namespace

//=============================================================================================================
// DEFINE PRIVATE IMPLEMENTATION
//=============================================================================================================

struct AcqDeviceRegistry::Impl
{
    struct Entry
    {
        AcqSchemeInfo info;
        Factory factory;
        Discoverer discoverer;
    };

    mutable std::mutex mutex;
    std::map<std::string, Entry, std::less<>> entries;
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AcqDeviceRegistry::AcqDeviceRegistry()
: m_pImpl(std::make_unique<Impl>())
{
    // Built-in schemes are registered here rather than by static initialisers, which a static link may drop.
    static_cast<void>(detail::registerSyntheticScheme(*this));
}

AcqDeviceRegistry::~AcqDeviceRegistry() = default;

AcqDeviceRegistry& AcqDeviceRegistry::instance()
{
    static AcqDeviceRegistry registry;
    return registry;
}

//=============================================================================================================

AcqError AcqDeviceRegistry::registerScheme(AcqSchemeInfo info, Factory factory, Discoverer discoverer)
{
    if (!validScheme(info.scheme)) {
        return {AcqErrorCode::InvalidParameter, "scheme '" + info.scheme + "' is not lower-case [a-z][a-z0-9+.-]*"};
    }
    if (!factory) {
        return {AcqErrorCode::InvalidParameter, "scheme '" + info.scheme + "' has no factory"};
    }
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    if (m_pImpl->entries.count(info.scheme) != 0) {
        return {AcqErrorCode::InvalidParameter, "scheme '" + info.scheme + "' is already registered"};
    }
    std::string scheme = info.scheme;
    m_pImpl->entries.emplace(std::move(scheme), Impl::Entry{std::move(info), std::move(factory), std::move(discoverer)});
    return {};
}

//=============================================================================================================

AcqExpected<AcqDevice> AcqDeviceRegistry::open(std::string_view uri) const
{
    auto parsed = AcqUri::parse(uri);
    if (!parsed) {
        return parsed.error();
    }
    Factory factory;
    AcqSchemeInfo info;
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mutex);
        const auto it = m_pImpl->entries.find(parsed->scheme);
        if (it == m_pImpl->entries.end()) {
            std::string known;
            for (const auto& [name, entry] : m_pImpl->entries) {
                known.append(known.empty() ? "" : ", ").append(name);
            }
            return AcqError(AcqErrorCode::UnknownScheme, "'" + parsed->scheme + "://'; registered: " + known);
        }
        factory = it->second.factory;
        info = it->second.info;
    }
    for (const auto& [key, value] : parsed->params) {
        const bool known = std::any_of(info.parameters.begin(), info.parameters.end(),
                                       [&key](const AcqParameterInfo& parameter) { return parameter.name == key; });
        if (!known) {
            std::string message = "unknown parameter '" + key + "' for " + info.scheme + "://; accepted:";
            for (const AcqParameterInfo& parameter : info.parameters) {
                message.append(" ").append(parameter.name);
            }
            return AcqError(AcqErrorCode::InvalidParameter, std::move(message));
        }
    }
    auto backend = factory(*parsed);
    if (!backend) {
        return backend.error();
    }
    if (!*backend) {
        return AcqError(AcqErrorCode::Internal, "the " + info.scheme + ":// factory returned no backend");
    }
    return AcqDevice(std::move(*backend));
}

//=============================================================================================================

std::vector<AcqSchemeInfo> AcqDeviceRegistry::schemes() const
{
    std::lock_guard<std::mutex> lock(m_pImpl->mutex);
    std::vector<AcqSchemeInfo> out;
    out.reserve(m_pImpl->entries.size());
    for (const auto& [name, entry] : m_pImpl->entries) {
        out.push_back(entry.info);
    }
    return out;
}

//=============================================================================================================

std::vector<AcqDiscoveredDevice> AcqDeviceRegistry::discover(std::chrono::milliseconds timeout) const
{
    std::vector<Discoverer> discoverers;
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mutex);
        for (const auto& [name, entry] : m_pImpl->entries) {
            if (entry.discoverer) {
                discoverers.push_back(entry.discoverer);
            }
        }
    }
    std::vector<std::vector<AcqDiscoveredDevice>> found(discoverers.size());
    std::vector<std::thread> threads;
    threads.reserve(discoverers.size());
    for (std::size_t i = 0; i < discoverers.size(); ++i) {
        threads.emplace_back([&found, &discoverers, i, timeout] { found[i] = discoverers[i](timeout); });
    }
    std::vector<AcqDiscoveredDevice> out;
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
        out.insert(out.end(), found[i].begin(), found[i].end());
    }
    return out;
}
