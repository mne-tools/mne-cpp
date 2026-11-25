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
 * @brief    Implementation of the ChannelDataModel circular-buffer table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldatamodel.h"

#include <mne/base/string_utils.h>
#include <mne/scene/trace_geometry.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <format>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>

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
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataModel::ChannelDataModel(QObject* parent)
: QObject(parent)
{
    // Default scales
    m_scaleMap[kMEGGradKind] = kScaleMEGGrad;
    m_scaleMap[kMEGMagKind] = kScaleMEGMag;
    m_scaleMap[FIFFV_EEG_CH] = kScaleEEG;
    m_scaleMap[FIFFV_EOG_CH] = kScaleEOG;
    m_scaleMap[FIFFV_EMG_CH] = kScaleEMG;
    m_scaleMap[FIFFV_ECG_CH] = kScaleECG;
    m_scaleMap[FIFFV_STIM_CH] = kScaleSTIM;
    m_scaleMap[FIFFV_MISC_CH] = kScaleMISC;
}

//=============================================================================================================

void ChannelDataModel::init(const std::shared_ptr<FiffInfo>& pFiffInfo)
{
    std::unique_lock lk(m_lock);
    m_pFiffInfo = pFiffInfo;

    int nCh = (pFiffInfo ? pFiffInfo->nchan : 0) + static_cast<int>(m_virtualDisplayInfo.size());
    m_channelData.resize(nCh);
    for (auto& ch : m_channelData)
        ch.clear();
    m_firstSample = 0;

    lk.unlock();
    rebuildDisplayInfo();
    emit metaChanged();
}

//=============================================================================================================

void ChannelDataModel::setData(const MatrixXd& data, int firstSample)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;

    std::unique_lock lk(m_lock);
    int nCh = static_cast<int>(data.rows());
    m_channelData.resize(nCh);
    for (int ch = 0; ch < nCh; ++ch) {
        m_channelData[ch].resize(static_cast<int>(data.cols()));
        for (int s = 0; s < static_cast<int>(data.cols()); ++s)
            m_channelData[ch][s] = static_cast<float>(data(ch, s));
    }
    m_firstSample = firstSample;
    lk.unlock();

    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::appendData(const MatrixXd& data)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;

    std::unique_lock lk(m_lock);
    int nCh = static_cast<int>(data.rows());
    int nNew = static_cast<int>(data.cols());

    if (std::ssize(m_channelData) != nCh)
        m_channelData.resize(nCh);

    for (int ch = 0; ch < nCh; ++ch) {
        auto& buf = m_channelData[ch];
        const int oldSize = static_cast<int>(buf.size());
        buf.resize(oldSize + nNew);
        for (int s = 0; s < nNew; ++s)
            buf[oldSize + s] = static_cast<float>(data(ch, s));

        // Drop oldest samples when capacity exceeded
        if (m_maxStoredSamples > 0 && std::ssize(buf) > m_maxStoredSamples) {
            const int drop = static_cast<int>(buf.size()) - m_maxStoredSamples;
            buf.erase(buf.begin(), buf.begin() + drop);
            if (ch == 0)
                m_firstSample += drop;
        }
    }
    lk.unlock();

    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::clearData()
{
    {
        std::unique_lock lk(m_lock);
        for (auto& ch : m_channelData)
            ch.clear();
        m_firstSample = 0;
    }
    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::setScaleMap(const std::map<int, float>& scaleMap)
{
    {
        std::unique_lock lk(m_lock);
        m_scaleMap = scaleMap;
    }
    rebuildDisplayInfo();
    emit metaChanged();
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
        std::unique_lock lk(m_lock);
        m_virtualDisplayInfo = virtualChannels;

        const int realChannelCount = m_pFiffInfo ? m_pFiffInfo->nchan : 0;
        const int totalChannelCount = realChannelCount + static_cast<int>(m_virtualDisplayInfo.size());
        m_channelData.resize(totalChannelCount);
    }

    rebuildDisplayInfo();
    emit metaChanged();
    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::setSignalColor(const QColor& color)
{
    {
        std::unique_lock lk(m_lock);
        m_signalColor = color;
    }
    rebuildDisplayInfo();
    emit metaChanged();
}

//=============================================================================================================

void ChannelDataModel::setMaxStoredSamples(int n)
{
    std::unique_lock lk(m_lock);
    m_maxStoredSamples = (n > 0) ? n : 0;
}

//=============================================================================================================

void ChannelDataModel::setRemoveDC(bool remove)
{
    setDetrendMode(remove ? DetrendMode::Mean : DetrendMode::None);
}

//=============================================================================================================

void ChannelDataModel::setDetrendMode(DetrendMode mode)
{
    {
        std::unique_lock lk(m_lock);
        m_detrendMode = mode;
    }
    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::setChannelBad(int channelIdx, bool bad)
{
    std::unique_lock lk(m_lock);
    if (channelIdx >= 0 && channelIdx < std::ssize(m_displayInfo))
        m_displayInfo[channelIdx].bad = bad;

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

    lk.unlock();
    emit metaChanged();
}

//=============================================================================================================

int ChannelDataModel::channelCount() const
{
    std::shared_lock lk(m_lock);
    const int realChannelCount = m_pFiffInfo ? m_pFiffInfo->nchan : 0;
    return std::max(static_cast<int>(m_channelData.size()), realChannelCount + static_cast<int>(m_virtualDisplayInfo.size()));
}

//=============================================================================================================

int ChannelDataModel::firstSample() const
{
    std::shared_lock lk(m_lock);
    return m_firstSample;
}

//=============================================================================================================

int ChannelDataModel::totalSamples() const
{
    std::shared_lock lk(m_lock);
    return m_channelData.empty() ? 0 : static_cast<int>(m_channelData[0].size());
}

//=============================================================================================================

ChannelDisplayInfo ChannelDataModel::channelInfo(int channelIdx) const
{
    std::shared_lock lk(m_lock);
    if (channelIdx >= 0 && channelIdx < std::ssize(m_displayInfo))
        return m_displayInfo[channelIdx];
    return {};
}

//=============================================================================================================

float ChannelDataModel::channelRms(int channelIdx, int first, int last) const
{
    std::shared_lock lk(m_lock);
    if (channelIdx < 0 || channelIdx >= std::ssize(m_channelData))
        return 0.f;
    const std::vector<float>& src = m_channelData[channelIdx];
    int bufFirst = std::clamp(first - m_firstSample, 0, static_cast<int>(src.size()));
    int bufLast = std::clamp(last - m_firstSample, 0, static_cast<int>(src.size()));
    if (bufLast <= bufFirst)
        return 0.f;
    // Cap at 1000 samples: use the last kMax samples of the window for speed
    constexpr int kMax = 1000;
    if (bufLast - bufFirst > kMax)
        bufFirst = bufLast - kMax;
    double sum = 0.0;
    for (int i = bufFirst; i < bufLast; ++i)
        sum += static_cast<double>(src[i]) * src[i];
    return static_cast<float>(std::sqrt(sum / (bufLast - bufFirst)));
}

//=============================================================================================================

float ChannelDataModel::sampleValueAt(int channelIdx, int sample) const
{
    std::shared_lock lk(m_lock);
    if (channelIdx < 0 || channelIdx >= std::ssize(m_channelData))
        return 0.f;
    const std::vector<float>& src = m_channelData[channelIdx];
    int bufIdx = sample - m_firstSample;
    if (bufIdx < 0 || bufIdx >= std::ssize(src))
        return 0.f;
    return src[bufIdx];
}

//=============================================================================================================

std::vector<float> ChannelDataModel::decimatedVertices(int channelIdx,
                                                       int firstSample,
                                                       int lastSample,
                                                       int pixelWidth,
                                                       int& vboFirstSample) const
{
    std::shared_lock lk(m_lock);

    vboFirstSample = firstSample; // may be updated below after clamping

    if (channelIdx < 0 || channelIdx >= std::ssize(m_channelData) || pixelWidth <= 0 || firstSample >= lastSample) {
        return {};
    }

    const std::vector<float>& src = m_channelData[channelIdx];
    // Map absolute sample indices to buffer indices
    int bufFirst = firstSample - m_firstSample;
    int bufLast = lastSample - m_firstSample;

    // Clamp to available data
    bufFirst = std::clamp(bufFirst, 0, static_cast<int>(src.size()));
    bufLast = std::clamp(bufLast, 0, static_cast<int>(src.size()));

    // Update to the actual absolute sample index at vertex 0 after clamping.
    // When firstSample < m_firstSample (tile extends before buffer start),
    // bufFirst is clamped to 0 so vboFirstSample must reflect m_firstSample,
    // not the original (possibly negative) firstSample.
    vboFirstSample = m_firstSample + bufFirst;

    if (bufLast <= bufFirst)
        return {};

    const scene::Detrend detrend = m_detrendMode == DetrendMode::Mean ? scene::Detrend::Mean
        : m_detrendMode == DetrendMode::Linear                        ? scene::Detrend::Linear
                                                                      : scene::Detrend::None;
    return scene::decimate_trace(std::span<const float>(src).subspan(static_cast<std::size_t>(bufFirst), static_cast<std::size_t>(bufLast - bufFirst)),
                                 pixelWidth, detrend);
}

//=============================================================================================================
// Private
//=============================================================================================================

void ChannelDataModel::rebuildDisplayInfo()
{
    std::unique_lock lk(m_lock);
    const int realChannelCount = m_pFiffInfo ? m_pFiffInfo->nchan : 0;
    const int nVirtual = static_cast<int>(m_virtualDisplayInfo.size());
    const int nCh = std::max(static_cast<int>(m_channelData.size()), realChannelCount + nVirtual);
    m_displayInfo.resize(nCh);
    for (int ch = 0; ch < nCh; ++ch) {
        if (ch >= realChannelCount && ch - realChannelCount < nVirtual) {
            ChannelDisplayInfo info = m_virtualDisplayInfo.at(ch - realChannelCount);
            if (info.name.empty())
                info.name = std::format("Virtual {}", ch - realChannelCount + 1);
            if (info.typeLabel.empty())
                info.typeLabel = "MISC";
            if (!info.color.isValid())
                info.color = m_signalColor;
            if (info.amplitudeMax <= 0.f)
                info.amplitudeMax = kScaleFallback;
            m_displayInfo[ch] = info;
            m_displayInfo[ch].isVirtualChannel = true;
            continue;
        }

        m_displayInfo[ch].amplitudeMax = amplitudeMaxForChannel(ch);
        m_displayInfo[ch].color = colorForChannel(ch);
        if (m_pFiffInfo && ch < realChannelCount)
            m_displayInfo[ch].name = m_pFiffInfo->ch_names[ch];
        else
            m_displayInfo[ch].name = std::format("CH {}", ch + 1);
        m_displayInfo[ch].typeLabel = typeLabelForChannel(ch);
        m_displayInfo[ch].bad = (m_pFiffInfo && ch < realChannelCount)
            ? std::ranges::contains(m_pFiffInfo->bads, m_displayInfo[ch].name)
            : false;
        m_displayInfo[ch].isVirtualChannel = false;
    }
}

//=============================================================================================================

float ChannelDataModel::amplitudeMaxForChannel(int ch) const
{
    if (!m_pFiffInfo || ch >= m_pFiffInfo->nchan)
        return kScaleFallback;

    const auto& info = m_pFiffInfo->chs[ch];
    qint32 kind = info.kind;
    const auto scaleOr = [this](int key, float fallback) {
        const auto it = m_scaleMap.find(key);
        return it != m_scaleMap.end() ? it->second : fallback;
    };

    // MEG: distinguish gradiometer vs. magnetometer by unit
    if (kind == FIFFV_MEG_CH) {
        if (info.unit == FIFF_UNIT_T_M)
            return scaleOr(kMEGGradKind, scaleOr(FIFFV_MEG_CH, kScaleMEGGrad));
        else
            return scaleOr(kMEGMagKind, scaleOr(FIFFV_MEG_CH, kScaleMEGMag));
    }
    if (m_scaleMap.contains(kind))
        return m_scaleMap.at(kind);
    return kScaleFallback;
}

//=============================================================================================================

QColor ChannelDataModel::colorForChannel(int ch) const
{
    if (!m_pFiffInfo || ch >= m_pFiffInfo->nchan)
        return m_signalColor;

    // Dark colours chosen to be readable on a light (near-white) background
    switch (m_pFiffInfo->chs[ch].kind) {
        case FIFFV_MEG_CH:
            return QColor(20, 90, 180); // dark blue for MEG
        case FIFFV_EEG_CH:
            return QColor(170, 55, 10); // dark orange for EEG
        case FIFFV_EOG_CH:
            return QColor(130, 0, 130); // dark purple for EOG
        case FIFFV_ECG_CH:
            return QColor(190, 15, 45); // dark crimson for ECG
        case FIFFV_EMG_CH:
            return QColor(20, 110, 20); // dark green for EMG
        case FIFFV_STIM_CH:
            return QColor(180, 100, 0); // dark amber for STIM
        default:
            return m_signalColor;
    }
}

//=============================================================================================================

std::string ChannelDataModel::typeLabelForChannel(int ch) const
{
    if (!m_pFiffInfo || ch >= m_pFiffInfo->nchan)
        return "MISC";
    switch (m_pFiffInfo->chs[ch].kind) {
        case FIFFV_MEG_CH:
            if (m_pFiffInfo->chs[ch].unit == FIFF_UNIT_T_M)
                return "MEG grad";
            return "MEG mag";
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

//=============================================================================================================

float ChannelDataModel::sfreq() const
{
    std::shared_lock lk(m_lock);
    return m_pFiffInfo ? static_cast<float>(m_pFiffInfo->sfreq) : 0.f;
}
