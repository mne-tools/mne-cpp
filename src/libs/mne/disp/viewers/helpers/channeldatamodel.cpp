//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     channeldatamodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of ChannelDataModel, the Qt adapter of scene::TraceBuffer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldatamodel.h"

#include "../../plots/helpers/scenepainter.h"

#include <mne/base/string_utils.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// Default amplitude scales (physical units) per channel kind.
// Keys match FIFF channel kind constants.
//=============================================================================================================

namespace
{
constexpr float kScaleMEGGrad = 400e-13f; // T/m
constexpr float kScaleMEGMag = 1.2e-12f;  // T
constexpr float kScaleEEG = 30e-6f;       // V
constexpr float kScaleEOG = 150e-6f;      // V
constexpr float kScaleEMG = 1e-3f;        // V
constexpr float kScaleECG = 1e-3f;        // V
constexpr float kScaleSTIM = 5.0f;        // AU
constexpr float kScaleMISC = 1.0f;        // AU
constexpr float kScaleFallback = 1.0f;

// Internal pseudo-kind keys for separate MEG grad/mag scales
constexpr qint32 kMEGGradKind = -1;
constexpr qint32 kMEGMagKind = -2;

QColor toQColor(const scene::Color& color)
{
    return QColor(color.r, color.g, color.b, color.a);
}

// Dark colours chosen to be readable on a light (near-white) background; none uses the signal colour
std::optional<scene::Color> colorForKind(int kind)
{
    switch (kind) {
        case FIFFV_MEG_CH:
            return scene::Color{20, 90, 180, 255}; // dark blue for MEG
        case FIFFV_EEG_CH:
            return scene::Color{170, 55, 10, 255}; // dark orange for EEG
        case FIFFV_EOG_CH:
            return scene::Color{130, 0, 130, 255}; // dark purple for EOG
        case FIFFV_ECG_CH:
            return scene::Color{190, 15, 45, 255}; // dark crimson for ECG
        case FIFFV_EMG_CH:
            return scene::Color{20, 110, 20, 255}; // dark green for EMG
        case FIFFV_STIM_CH:
            return scene::Color{180, 100, 0, 255}; // dark amber for STIM
        default:
            return std::nullopt;
    }
}

std::string typeLabelFor(const FiffChInfo& info)
{
    switch (info.kind) {
        case FIFFV_MEG_CH:
            return info.unit == FIFF_UNIT_T_M ? "MEG grad" : "MEG mag";
        case FIFFV_EEG_CH:
            return "EEG";
        case FIFFV_EOG_CH:
            return "EOG";
        case FIFFV_ECG_CH:
            return "ECG";
        case FIFFV_EMG_CH:
            return "EMG";
        case FIFFV_STIM_CH:
            return "STIM";
        default:
            return "MISC";
    }
}

scene::Detrend toSceneDetrend(DetrendMode mode)
{
    switch (mode) {
        case DetrendMode::Mean:
            return scene::Detrend::Mean;
        case DetrendMode::Linear:
            return scene::Detrend::Linear;
        default:
            return scene::Detrend::None;
    }
}
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataModel::ChannelDataModel(QObject* parent)
: QObject(parent)
{
    m_buffer.set_scale_map({{kMEGGradKind, kScaleMEGGrad},
                            {kMEGMagKind, kScaleMEGMag},
                            {FIFFV_EEG_CH, kScaleEEG},
                            {FIFFV_EOG_CH, kScaleEOG},
                            {FIFFV_EMG_CH, kScaleEMG},
                            {FIFFV_ECG_CH, kScaleECG},
                            {FIFFV_STIM_CH, kScaleSTIM},
                            {FIFFV_MISC_CH, kScaleMISC}});
    m_buffer.set_signal_color(toSceneColor(QColor(Qt::darkGreen)));
    m_buffer.set_spec_provider([this] { return channelSpecs(); });
    // The buffer may notify from any writing thread; the signals are delivered as before
    m_buffer.add_listener([this](scene::TraceBuffer::Change change) {
        if (change == scene::TraceBuffer::Change::Data)
            emit dataChanged();
        else
            emit metaChanged();
    });
}

//=============================================================================================================

void ChannelDataModel::init(const std::shared_ptr<FiffInfo>& pFiffInfo)
{
    {
        std::scoped_lock lk(m_infoLock);
        m_pFiffInfo = pFiffInfo;
    }
    m_buffer.reload_channels(true);
}

//=============================================================================================================

void ChannelDataModel::setData(const MatrixXd& data, int firstSample)
{
    m_buffer.set_data(data, firstSample);
}

//=============================================================================================================

void ChannelDataModel::appendData(const MatrixXd& data)
{
    m_buffer.append_data(data);
}

//=============================================================================================================

void ChannelDataModel::clearData()
{
    m_buffer.clear_data();
}

//=============================================================================================================

void ChannelDataModel::setScaleMap(const std::map<int, float>& scaleMap)
{
    m_buffer.set_scale_map(scaleMap);
}

//=============================================================================================================

void ChannelDataModel::setScaleMapFromStrings(const std::map<std::string, double>& scaleMap)
{
    std::map<int, float> intMap;
    const auto copy = [&](const char* key, int kind) {
        if (const auto it = scaleMap.find(key); it != scaleMap.end()) {
            intMap[kind] = static_cast<float>(it->second);
        }
    };
    copy("MEG_grad", kMEGGradKind);
    copy("MEG_mag", kMEGMagKind);
    copy("MEG_EEG", FIFFV_EEG_CH);
    copy("MEG_EOG", FIFFV_EOG_CH);
    copy("MEG_EMG", FIFFV_EMG_CH);
    copy("MEG_ECG", FIFFV_ECG_CH);
    copy("MEG_MISC", FIFFV_MISC_CH);
    copy("MEG_STIM", FIFFV_STIM_CH);
    setScaleMap(intMap);
}

//=============================================================================================================

void ChannelDataModel::setVirtualChannels(const std::vector<ChannelDisplayInfo>& virtualChannels)
{
    {
        std::scoped_lock lk(m_infoLock);
        m_virtualDisplayInfo = virtualChannels;
    }
    m_buffer.reload_channels(false);
}

//=============================================================================================================

void ChannelDataModel::setSignalColor(const QColor& color)
{
    m_buffer.set_signal_color(toSceneColor(color));
}

//=============================================================================================================

void ChannelDataModel::setMaxStoredSamples(int n)
{
    m_buffer.set_capacity(n);
}

//=============================================================================================================

void ChannelDataModel::setRemoveDC(bool remove)
{
    setDetrendMode(remove ? DetrendMode::Mean : DetrendMode::None);
}

//=============================================================================================================

void ChannelDataModel::setDetrendMode(DetrendMode mode)
{
    m_buffer.set_detrend(toSceneDetrend(mode));
}

//=============================================================================================================

DetrendMode ChannelDataModel::detrendMode() const
{
    switch (m_buffer.detrend()) {
        case scene::Detrend::Mean:
            return DetrendMode::Mean;
        case scene::Detrend::Linear:
            return DetrendMode::Linear;
        default:
            return DetrendMode::None;
    }
}

//=============================================================================================================

void ChannelDataModel::setChannelBad(int channelIdx, bool bad)
{
    {
        std::scoped_lock lk(m_infoLock);
        if (m_pFiffInfo && channelIdx >= 0 && channelIdx < m_pFiffInfo->nchan) {
            const std::string& channelName = m_pFiffInfo->ch_names[channelIdx];
            const int badIndex = mne::index_of(m_pFiffInfo->bads, channelName);

            if (bad) {
                if (badIndex < 0)
                    m_pFiffInfo->bads.push_back(channelName);
            } else if (badIndex >= 0) {
                m_pFiffInfo->bads.erase(m_pFiffInfo->bads.begin() + badIndex);
            }
        }
    }
    m_buffer.set_channel_bad(channelIdx, bad);
}

//=============================================================================================================

int ChannelDataModel::channelCount() const
{
    return m_buffer.channel_count();
}

//=============================================================================================================

int ChannelDataModel::firstSample() const
{
    return m_buffer.first_sample();
}

//=============================================================================================================

int ChannelDataModel::totalSamples() const
{
    return m_buffer.sample_count();
}

//=============================================================================================================

ChannelDisplayInfo ChannelDataModel::channelInfo(int channelIdx) const
{
    const std::optional<scene::TraceSource::Channel> channel = m_buffer.channel(channelIdx);
    if (!channel)
        return {};
    return {channel->name, channel->type_label, toQColor(channel->color), channel->amplitude_max, channel->bad,
            m_buffer.is_virtual(channelIdx)};
}

//=============================================================================================================

float ChannelDataModel::channelRms(int channelIdx, int first, int last) const
{
    return m_buffer.rms(channelIdx, first, last);
}

//=============================================================================================================

float ChannelDataModel::sampleValueAt(int channelIdx, int sample) const
{
    return m_buffer.value_at(channelIdx, sample);
}

//=============================================================================================================

std::vector<float> ChannelDataModel::decimatedVertices(int channelIdx,
                                                       int firstSample,
                                                       int lastSample,
                                                       int pixelWidth,
                                                       int& vboFirstSample) const
{
    scene::TraceVertices vertices = m_buffer.vertices(channelIdx, firstSample, lastSample, pixelWidth);
    vboFirstSample = vertices.first_sample;
    return std::move(vertices.xy);
}

//=============================================================================================================

float ChannelDataModel::sfreq() const
{
    std::scoped_lock lk(m_infoLock);
    return m_pFiffInfo ? static_cast<float>(m_pFiffInfo->sfreq) : 0.f;
}

//=============================================================================================================
// Private
//=============================================================================================================

std::vector<scene::TraceBuffer::ChannelSpec> ChannelDataModel::channelSpecs() const
{
    std::scoped_lock lk(m_infoLock);
    std::vector<scene::TraceBuffer::ChannelSpec> specs;
    const int realChannelCount = m_pFiffInfo ? m_pFiffInfo->nchan : 0;
    specs.reserve(static_cast<std::size_t>(realChannelCount) + m_virtualDisplayInfo.size());

    for (int ch = 0; ch < realChannelCount; ++ch) {
        const FiffChInfo& info = m_pFiffInfo->chs[ch];
        scene::TraceBuffer::ChannelSpec spec;
        spec.name = m_pFiffInfo->ch_names[ch];
        spec.type_label = typeLabelFor(info);
        spec.color = colorForKind(info.kind);
        // MEG: gradiometer or magnetometer scale by unit, then the common MEG scale
        if (info.kind == FIFFV_MEG_CH) {
            const bool grad = info.unit == FIFF_UNIT_T_M;
            spec.scale_keys = {grad ? kMEGGradKind : kMEGMagKind, FIFFV_MEG_CH};
            spec.default_scale = grad ? kScaleMEGGrad : kScaleMEGMag;
        } else {
            spec.scale_keys = {info.kind};
            spec.default_scale = kScaleFallback;
        }
        spec.bad = std::ranges::contains(m_pFiffInfo->bads, spec.name);
        specs.push_back(std::move(spec));
    }

    for (const ChannelDisplayInfo& info : m_virtualDisplayInfo) {
        scene::TraceBuffer::ChannelSpec spec;
        spec.name = info.name;
        spec.type_label = info.typeLabel;
        if (info.color.isValid())
            spec.color = toSceneColor(info.color);
        spec.default_scale = info.amplitudeMax;
        spec.bad = info.bad;
        spec.is_virtual = true;
        specs.push_back(std::move(spec));
    }
    return specs;
}
