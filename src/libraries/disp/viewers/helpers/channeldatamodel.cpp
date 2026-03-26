//=============================================================================================================
/**
 * @file     channeldatamodel.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the ChannelDataModel class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channeldatamodel.h"
#include <fiff/fiff_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtMath>
#include <QWriteLocker>
#include <QReadLocker>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// Default amplitude scales (physical units) per channel kind.
// Keys match FIFF channel kind constants.
//=============================================================================================================

namespace {
    constexpr float kScaleMEGGrad = 400e-13f;   // T/m
    constexpr float kScaleMEGMag  = 1.2e-12f;   // T
    constexpr float kScaleEEG     = 30e-6f;     // V
    constexpr float kScaleEOG     = 150e-6f;    // V
    constexpr float kScaleEMG     = 1e-3f;      // V
    constexpr float kScaleECG     = 1e-3f;      // V
    constexpr float kScaleSTIM    = 5.0f;       // AU
    constexpr float kScaleMISC    = 1.0f;       // AU
    constexpr float kScaleFallback = 1.0f;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChannelDataModel::ChannelDataModel(QObject *parent)
    : QObject(parent)
{
    // Default scales
    m_scaleMap[FIFFV_MEG_CH]  = kScaleMEGGrad;
    m_scaleMap[FIFFV_EEG_CH]  = kScaleEEG;
    m_scaleMap[FIFFV_EOG_CH]  = kScaleEOG;
    m_scaleMap[FIFFV_EMG_CH]  = kScaleEMG;
    m_scaleMap[FIFFV_ECG_CH]  = kScaleECG;
    m_scaleMap[FIFFV_STIM_CH] = kScaleSTIM;
    m_scaleMap[FIFFV_MISC_CH] = kScaleMISC;
}

//=============================================================================================================

void ChannelDataModel::init(QSharedPointer<FiffInfo> pFiffInfo)
{
    QWriteLocker lk(&m_lock);
    m_pFiffInfo = pFiffInfo;

    int nCh = pFiffInfo ? pFiffInfo->nchan : 0;
    m_channelData.resize(nCh);
    for (auto &ch : m_channelData)
        ch.clear();
    m_firstSample = 0;

    lk.unlock();
    rebuildDisplayInfo();
    emit metaChanged();
}

//=============================================================================================================

void ChannelDataModel::setData(const MatrixXd &data, int firstSample)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;

    QWriteLocker lk(&m_lock);
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

void ChannelDataModel::appendData(const MatrixXd &data)
{
    if (data.rows() == 0 || data.cols() == 0)
        return;

    QWriteLocker lk(&m_lock);
    int nCh   = static_cast<int>(data.rows());
    int nNew  = static_cast<int>(data.cols());

    if (m_channelData.size() != nCh)
        m_channelData.resize(nCh);

    for (int ch = 0; ch < nCh; ++ch) {
        auto &buf = m_channelData[ch];
        int oldSize = buf.size();
        buf.resize(oldSize + nNew);
        for (int s = 0; s < nNew; ++s)
            buf[oldSize + s] = static_cast<float>(data(ch, s));

        // Drop oldest samples when capacity exceeded
        if (m_maxStoredSamples > 0 && buf.size() > m_maxStoredSamples) {
            int drop = buf.size() - m_maxStoredSamples;
            buf.remove(0, drop);
            if (ch == 0)
                m_firstSample += drop;
        }
    }
    lk.unlock();

    emit dataChanged();
}

//=============================================================================================================

void ChannelDataModel::setScaleMap(const QMap<qint32, float> &scaleMap)
{
    {
        QWriteLocker lk(&m_lock);
        m_scaleMap = scaleMap;
    }
    rebuildDisplayInfo();
    emit metaChanged();
}

//=============================================================================================================

void ChannelDataModel::setSignalColor(const QColor &color)
{
    {
        QWriteLocker lk(&m_lock);
        m_signalColor = color;
    }
    rebuildDisplayInfo();
    emit metaChanged();
}

//=============================================================================================================

void ChannelDataModel::setMaxStoredSamples(int n)
{
    QWriteLocker lk(&m_lock);
    m_maxStoredSamples = (n > 0) ? n : 0;
}

//=============================================================================================================

void ChannelDataModel::setChannelBad(int channelIdx, bool bad)
{
    QWriteLocker lk(&m_lock);
    if (channelIdx >= 0 && channelIdx < m_displayInfo.size())
        m_displayInfo[channelIdx].bad = bad;
    lk.unlock();
    emit metaChanged();
}

//=============================================================================================================

int ChannelDataModel::channelCount() const
{
    QReadLocker lk(&m_lock);
    return m_channelData.size();
}

//=============================================================================================================

int ChannelDataModel::firstSample() const
{
    QReadLocker lk(&m_lock);
    return m_firstSample;
}

//=============================================================================================================

int ChannelDataModel::totalSamples() const
{
    QReadLocker lk(&m_lock);
    return m_channelData.isEmpty() ? 0 : m_channelData[0].size();
}

//=============================================================================================================

ChannelDisplayInfo ChannelDataModel::channelInfo(int channelIdx) const
{
    QReadLocker lk(&m_lock);
    if (channelIdx >= 0 && channelIdx < m_displayInfo.size())
        return m_displayInfo[channelIdx];
    return {};
}

//=============================================================================================================

QVector<float> ChannelDataModel::decimatedVertices(int   channelIdx,
                                                    int   firstSample,
                                                    int   lastSample,
                                                    int   pixelWidth,
                                                    int  &vboFirstSample) const
{
    QReadLocker lk(&m_lock);

    vboFirstSample = firstSample;

    if (channelIdx < 0 || channelIdx >= m_channelData.size()
        || pixelWidth <= 0 || firstSample >= lastSample) {
        return {};
    }

    const QVector<float> &src = m_channelData[channelIdx];
    // Map absolute sample indices to buffer indices
    int bufFirst = firstSample - m_firstSample;
    int bufLast  = lastSample  - m_firstSample;

    // Clamp to available data
    bufFirst = qBound(0, bufFirst, src.size());
    bufLast  = qBound(0, bufLast,  src.size());

    if (bufLast <= bufFirst)
        return {};

    int nSamples = bufLast - bufFirst;

    if (nSamples <= pixelWidth * 2) {
        // ── Raw path: one vertex per sample ────────────────────────────
        QVector<float> result;
        result.reserve(nSamples * 2);
        for (int i = 0; i < nSamples; ++i) {
            result.append(static_cast<float>(i));        // x offset
            result.append(src[bufFirst + i]);             // y amplitude
        }
        return result;
    }

    // ── Decimation path: min/max envelope, 2 vertices per pixel ─────────
    // Interleaved as (x_at_col, max), (x_at_col, min) per column.
    // Connected as LINE_STRIP this draws: vertical spike at each column,
    // diagonals between columns — the classic oscilloscope envelope look.

    QVector<float> result;
    result.reserve(pixelWidth * 4); // 2 vertices × 2 floats

    float spp = static_cast<float>(nSamples) / pixelWidth; // samples per pixel

    for (int px = 0; px < pixelWidth; ++px) {
        int sBegin = bufFirst + static_cast<int>(px       * spp);
        int sEnd   = bufFirst + static_cast<int>((px + 1) * spp);
        sEnd = qMin(sEnd, bufLast);
        if (sBegin >= sEnd) sBegin = qMax(sEnd - 1, bufFirst);

        float minV = src[sBegin];
        float maxV = src[sBegin];
        for (int s = sBegin + 1; s < sEnd; ++s) {
            if (src[s] < minV) minV = src[s];
            if (src[s] > maxV) maxV = src[s];
        }

        float xOffset = px * spp; // relative to bufFirst (= firstSample - m_firstSample)

        // Emit max first, then min: draws ascending spike first
        result.append(xOffset); result.append(maxV);
        result.append(xOffset); result.append(minV);
    }

    return result;
}

//=============================================================================================================
// Private
//=============================================================================================================

void ChannelDataModel::rebuildDisplayInfo()
{
    QWriteLocker lk(&m_lock);
    int nCh = m_channelData.size();
    m_displayInfo.resize(nCh);
    for (int ch = 0; ch < nCh; ++ch) {
        m_displayInfo[ch].amplitudeMax = amplitudeMaxForChannel(ch);
        m_displayInfo[ch].color        = colorForChannel(ch);
        if (m_pFiffInfo && ch < m_pFiffInfo->nchan)
            m_displayInfo[ch].name = m_pFiffInfo->ch_names[ch];
        else
            m_displayInfo[ch].name = QString("CH %1").arg(ch + 1);
        m_displayInfo[ch].bad = false;
    }
}

//=============================================================================================================

float ChannelDataModel::amplitudeMaxForChannel(int ch) const
{
    if (!m_pFiffInfo || ch >= m_pFiffInfo->nchan)
        return kScaleFallback;

    const auto &info = m_pFiffInfo->chs[ch];
    qint32 kind = info.kind;

    // MEG: distinguish gradiometer vs. magnetometer by unit
    if (kind == FIFFV_MEG_CH) {
        if (info.unit == FIFF_UNIT_T_M)
            return m_scaleMap.value(FIFFV_MEG_CH, kScaleMEGGrad);
        else
            return m_scaleMap.value(FIFFV_MEG_CH, kScaleMEGMag);
    }
    if (m_scaleMap.contains(kind))
        return m_scaleMap.value(kind);
    return kScaleFallback;
}

//=============================================================================================================

QColor ChannelDataModel::colorForChannel(int ch) const
{
    if (!m_pFiffInfo || ch >= m_pFiffInfo->nchan)
        return m_signalColor;

    switch (m_pFiffInfo->chs[ch].kind) {
    case FIFFV_MEG_CH:  return QColor(0, 160, 220);   // teal-blue for MEG
    case FIFFV_EEG_CH:  return QColor(220, 80, 20);   // orange for EEG
    case FIFFV_EOG_CH:  return QColor(180, 0, 180);   // purple for EOG
    case FIFFV_ECG_CH:  return QColor(220, 20, 60);   // crimson for ECG
    case FIFFV_EMG_CH:  return QColor(34, 139, 34);   // forest-green for EMG
    case FIFFV_STIM_CH: return QColor(255, 165, 0);   // orange for STIM
    default:            return m_signalColor;
    }
}
