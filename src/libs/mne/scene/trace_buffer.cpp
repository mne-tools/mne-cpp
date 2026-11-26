//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trace_buffer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Definition of TraceBuffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "trace_buffer.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <iterator>
#include <mutex>
#include <span>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::scene;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int TraceBuffer::add_listener(Listener listener)
{
    std::unique_lock lk(m_lock);
    const int id = m_nextListenerId++;
    m_listeners.emplace_back(id, std::move(listener));
    return id;
}

//=============================================================================================================

void TraceBuffer::remove_listener(int id)
{
    std::unique_lock lk(m_lock);
    std::erase_if(m_listeners, [id](const auto& entry) { return entry.first == id; });
}

//=============================================================================================================

void TraceBuffer::set_spec_provider(SpecProvider provider)
{
    std::unique_lock lk(m_lock);
    m_specProvider = std::move(provider);
}

//=============================================================================================================

void TraceBuffer::reload_channels(bool clear_data)
{
    SpecProvider provider;
    {
        std::shared_lock lk(m_lock);
        provider = m_specProvider;
    }
    // The provider runs outside the lock: it may read host state that the host guards itself
    std::vector<ChannelSpec> specs = provider ? provider() : std::vector<ChannelSpec>{};
    {
        std::unique_lock lk(m_lock);
        if (provider)
            m_specs = std::move(specs);
        m_data.resize(m_specs.size());
        if (clear_data) {
            for (std::vector<float>& channel : m_data)
                channel.clear();
            m_firstSample = 0;
        }
        resolve();
    }
    notify(Change::Channels);
    if (!clear_data)
        notify(Change::Data);
}

//=============================================================================================================

void TraceBuffer::set_channels(std::vector<ChannelSpec> channels)
{
    set_spec_provider([channels = std::move(channels)] { return channels; });
    reload_channels(false);
}

//=============================================================================================================

void TraceBuffer::set_data(const Eigen::MatrixXd& data, int first_sample)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;
    {
        std::unique_lock lk(m_lock);
        const auto rows = static_cast<std::size_t>(data.rows());
        const auto cols = static_cast<std::size_t>(data.cols());
        m_data.resize(rows);
        for (std::size_t ch = 0; ch < rows; ++ch) {
            m_data[ch].resize(cols);
            for (std::size_t s = 0; s < cols; ++s)
                m_data[ch][s] = static_cast<float>(data(static_cast<Eigen::Index>(ch), static_cast<Eigen::Index>(s)));
        }
        m_firstSample = first_sample;
    }
    notify(Change::Data);
}

//=============================================================================================================

void TraceBuffer::append_data(const Eigen::MatrixXd& data)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;
    {
        std::unique_lock lk(m_lock);
        const auto rows = static_cast<std::size_t>(data.rows());
        const auto added = static_cast<std::size_t>(data.cols());
        if (m_data.size() != rows)
            m_data.resize(rows);

        for (std::size_t ch = 0; ch < rows; ++ch) {
            std::vector<float>& samples = m_data[ch];
            const std::size_t old_size = samples.size();
            samples.resize(old_size + added);
            for (std::size_t s = 0; s < added; ++s)
                samples[old_size + s] = static_cast<float>(data(static_cast<Eigen::Index>(ch), static_cast<Eigen::Index>(s)));

            if (m_capacity > 0 && std::ssize(samples) > m_capacity) {
                const std::ptrdiff_t drop = std::ssize(samples) - m_capacity;
                samples.erase(samples.begin(), samples.begin() + drop);
                if (ch == 0)
                    m_firstSample += static_cast<int>(drop);
            }
        }
    }
    notify(Change::Data);
}

//=============================================================================================================

void TraceBuffer::clear_data()
{
    {
        std::unique_lock lk(m_lock);
        for (std::vector<float>& channel : m_data)
            channel.clear();
        m_firstSample = 0;
    }
    notify(Change::Data);
}

//=============================================================================================================

void TraceBuffer::set_capacity(int samples)
{
    std::unique_lock lk(m_lock);
    m_capacity = std::max(samples, 0);
}

//=============================================================================================================

void TraceBuffer::set_scale_map(std::map<int, float> scales)
{
    {
        std::unique_lock lk(m_lock);
        m_scales = std::move(scales);
    }
    reload_scales_and_colors();
}

//=============================================================================================================

std::map<int, float> TraceBuffer::scale_map() const
{
    std::shared_lock lk(m_lock);
    return m_scales;
}

//=============================================================================================================

void TraceBuffer::set_signal_color(Color color)
{
    {
        std::unique_lock lk(m_lock);
        m_signalColor = color;
    }
    reload_scales_and_colors();
}

//=============================================================================================================

void TraceBuffer::reload_scales_and_colors()
{
    SpecProvider provider;
    {
        std::shared_lock lk(m_lock);
        provider = m_specProvider;
    }
    std::vector<ChannelSpec> specs = provider ? provider() : std::vector<ChannelSpec>{};
    {
        std::unique_lock lk(m_lock);
        if (provider)
            m_specs = std::move(specs);
        resolve();
    }
    notify(Change::Channels);
}

//=============================================================================================================

void TraceBuffer::set_channel_bad(int index, bool bad)
{
    {
        std::unique_lock lk(m_lock);
        if (index >= 0 && index < std::ssize(m_channels))
            m_channels[static_cast<std::size_t>(index)].bad = bad;
    }
    notify(Change::Channels);
}

//=============================================================================================================

void TraceBuffer::set_detrend(Detrend detrend)
{
    {
        std::unique_lock lk(m_lock);
        m_detrend = detrend;
    }
    notify(Change::Data);
}

//=============================================================================================================

Detrend TraceBuffer::detrend() const
{
    std::shared_lock lk(m_lock);
    return m_detrend;
}

//=============================================================================================================

void TraceBuffer::set_sample_rate(float hz)
{
    std::unique_lock lk(m_lock);
    m_sampleRate = hz;
}

//=============================================================================================================

float TraceBuffer::sample_rate() const
{
    std::shared_lock lk(m_lock);
    return m_sampleRate;
}

//=============================================================================================================

bool TraceBuffer::is_virtual(int index) const
{
    std::shared_lock lk(m_lock);
    return index >= 0 && index < std::ssize(m_virtual) && m_virtual[static_cast<std::size_t>(index)];
}

//=============================================================================================================

float TraceBuffer::rms(int index, int first, int last) const
{
    std::shared_lock lk(m_lock);
    if (index < 0 || index >= std::ssize(m_data))
        return 0.f;
    const std::vector<float>& samples = m_data[static_cast<std::size_t>(index)];
    const int size = static_cast<int>(samples.size());
    int from = std::clamp(first - m_firstSample, 0, size);
    const int to = std::clamp(last - m_firstSample, 0, size);
    if (to <= from)
        return 0.f;
    // At most the last 1000 samples, so that the render thread stays fast
    constexpr int kMax = 1000;
    from = std::max(from, to - kMax);
    double sum = 0.0;
    for (int i = from; i < to; ++i) {
        const double value = samples[static_cast<std::size_t>(i)];
        sum += value * value;
    }
    return static_cast<float>(std::sqrt(sum / (to - from)));
}

//=============================================================================================================

int TraceBuffer::channel_count() const
{
    std::shared_lock lk(m_lock);
    return stored_count();
}

//=============================================================================================================

std::optional<TraceSource::Channel> TraceBuffer::channel(int index) const
{
    std::shared_lock lk(m_lock);
    if (index < 0 || index >= std::ssize(m_channels))
        return std::nullopt;
    return m_channels[static_cast<std::size_t>(index)];
}

//=============================================================================================================

float TraceBuffer::value_at(int index, int sample) const
{
    std::shared_lock lk(m_lock);
    if (index < 0 || index >= std::ssize(m_data))
        return 0.f;
    const std::vector<float>& samples = m_data[static_cast<std::size_t>(index)];
    const int offset = sample - m_firstSample;
    if (offset < 0 || offset >= std::ssize(samples))
        return 0.f;
    return samples[static_cast<std::size_t>(offset)];
}

//=============================================================================================================

int TraceBuffer::first_sample() const
{
    std::shared_lock lk(m_lock);
    return m_firstSample;
}

//=============================================================================================================

int TraceBuffer::sample_count() const
{
    std::shared_lock lk(m_lock);
    return m_data.empty() ? 0 : static_cast<int>(m_data.front().size());
}

//=============================================================================================================

TraceVertices TraceBuffer::vertices(int index, int first, int last, int pixel_width) const
{
    std::shared_lock lk(m_lock);
    TraceVertices result{first, {}};
    if (index < 0 || index >= std::ssize(m_data) || pixel_width <= 0 || first >= last)
        return result;

    const std::vector<float>& samples = m_data[static_cast<std::size_t>(index)];
    const int size = static_cast<int>(samples.size());
    const int from = std::clamp(first - m_firstSample, 0, size);
    const int to = std::clamp(last - m_firstSample, 0, size);

    // A window starting before the stored samples starts at the first stored one
    result.first_sample = m_firstSample + from;
    if (to <= from)
        return result;

    result.xy = decimate_trace(std::span<const float>(samples).subspan(static_cast<std::size_t>(from), static_cast<std::size_t>(to - from)),
                               pixel_width, m_detrend);
    return result;
}

//=============================================================================================================

void TraceBuffer::resolve()
{
    const auto count = static_cast<std::size_t>(stored_count());
    m_channels.assign(count, Channel{});
    m_virtual.assign(count, false);

    const auto scale_of = [this](const ChannelSpec& spec) {
        for (const int key : spec.scale_keys) {
            if (const auto it = m_scales.find(key); it != m_scales.end())
                return it->second;
        }
        return spec.default_scale > 0.f ? spec.default_scale : 1.f;
    };

    int virtual_number = 0;
    for (std::size_t ch = 0; ch < count; ++ch) {
        Channel& channel = m_channels[ch];
        if (ch >= m_specs.size()) {
            channel = {std::format("CH {}", ch + 1), "MISC", 1.f, m_signalColor, false};
            continue;
        }
        const ChannelSpec& spec = m_specs[ch];
        channel = {spec.name, spec.type_label, scale_of(spec), spec.color.value_or(m_signalColor), spec.bad};
        if (spec.is_virtual) {
            ++virtual_number;
            if (channel.name.empty())
                channel.name = std::format("Virtual {}", virtual_number);
            if (channel.type_label.empty())
                channel.type_label = "MISC";
            m_virtual[ch] = true;
        }
    }
}

//=============================================================================================================

void TraceBuffer::notify(Change change) const
{
    std::vector<Listener> listeners;
    {
        std::shared_lock lk(m_lock);
        listeners.reserve(m_listeners.size());
        for (const auto& entry : m_listeners)
            listeners.push_back(entry.second);
    }
    for (const Listener& listener : listeners)
        listener(change);
}

//=============================================================================================================

int TraceBuffer::stored_count() const
{
    return static_cast<int>(std::max(m_data.size(), m_specs.size()));
}
