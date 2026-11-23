//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourcedataworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-tick sparse source-to-vertex mat-vec, colormap mapping and ABGR colour packing per hemisphere.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedataworker.h"

#include <mne/base/log.h>

#include "core/rendertypes.h"
#include <mne/disp/plots/helpers/colormap.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

using namespace mne::disp;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataWorker::RtSourceDataWorker(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

void RtSourceDataWorker::addData(const Eigen::VectorXd& data)
{
    std::scoped_lock locker(m_mutex);

    // Cap queue size at sampling frequency (1 second of data)
    if (std::ssize(m_lDataQ) < static_cast<int>(m_dSFreq)) {
        m_lDataQ.push_back(data);
    }

    // Also store for looping
    if (m_bIsLooping) {
        m_lDataLoopQ.push_back(data);
        // Cap loop queue too
        if (std::ssize(m_lDataLoopQ) > static_cast<int>(m_dSFreq) * 10) {
            m_lDataLoopQ.pop_front();
        }
    }
}

//=============================================================================================================

void RtSourceDataWorker::clear()
{
    std::scoped_lock locker(m_mutex);
    m_lDataQ.clear();
    m_lDataLoopQ.clear();
    m_vecAverage = Eigen::VectorXd();
    m_iSampleCtr = 0;
    m_iCurrentSample = 0;
}

//=============================================================================================================

void RtSourceDataWorker::setInterpolationMatrixLeft(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    std::scoped_lock locker(m_mutex);
    m_interpMatLh = std::move(mat);
}

//=============================================================================================================

void RtSourceDataWorker::setInterpolationMatrixRight(std::shared_ptr<Eigen::SparseMatrix<float>> mat)
{
    std::scoped_lock locker(m_mutex);
    m_interpMatRh = std::move(mat);
}

//=============================================================================================================

void RtSourceDataWorker::setNumberAverages(int numAvr)
{
    std::scoped_lock locker(m_mutex);
    m_iNumAverages = std::max(1, numAvr);
}

//=============================================================================================================

void RtSourceDataWorker::setColormapType(const std::string& name)
{
    std::scoped_lock locker(m_mutex);
    m_sColormapType = QString::fromStdString(name);
}

//=============================================================================================================

void RtSourceDataWorker::setThresholds(double min, double mid, double max)
{
    std::scoped_lock locker(m_mutex);
    m_dThreshMin = min;
    m_dThreshMid = mid;
    m_dThreshMax = max;
}

//=============================================================================================================

void RtSourceDataWorker::setLoopState(bool enabled)
{
    std::scoped_lock locker(m_mutex);
    m_bIsLooping = enabled;
}

//=============================================================================================================

void RtSourceDataWorker::setSFreq(double sFreq)
{
    std::scoped_lock locker(m_mutex);
    m_dSFreq = std::max(1.0, sFreq);
}

//=============================================================================================================

void RtSourceDataWorker::setSurfaceColor(const std::vector<uint32_t>& baseColorsLh,
                                         const std::vector<uint32_t>& baseColorsRh)
{
    std::scoped_lock locker(m_mutex);
    m_baseColorsLh = baseColorsLh;
    m_baseColorsRh = baseColorsRh;
}

//=============================================================================================================

void RtSourceDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    std::scoped_lock locker(m_mutex);
    m_bStreamSmoothedData = bStreamSmoothedData;
}

//=============================================================================================================

void RtSourceDataWorker::streamData()
{
    std::unique_lock locker(m_mutex);

    // Try to get data from queue
    Eigen::VectorXd vecCurrentData;

    if (!m_lDataQ.empty()) {
        vecCurrentData = std::move(m_lDataQ.front());
        m_lDataQ.pop_front();
    } else if (m_bIsLooping && !m_lDataLoopQ.empty()) {
        // Loop: replay from stored data
        vecCurrentData = m_lDataLoopQ[m_iCurrentSample % m_lDataLoopQ.size()];
        m_iCurrentSample++;
    } else {
        // No data available
        return;
    }

    // Averaging
    if (m_iNumAverages > 1) {
        if (m_vecAverage.size() != vecCurrentData.size()) {
            m_vecAverage = vecCurrentData;
            m_iSampleCtr = 1;
        } else {
            m_vecAverage += vecCurrentData;
            m_iSampleCtr++;
        }

        if (m_iSampleCtr < m_iNumAverages) {
            return; // Accumulate more samples
        }

        vecCurrentData = m_vecAverage / static_cast<double>(m_iSampleCtr);
        m_vecAverage = Eigen::VectorXd();
        m_iSampleCtr = 0;
    }

    // Take absolute values (source power is always positive)
    Eigen::VectorXf vecAbsData = vecCurrentData.cwiseAbs().cast<float>();

    // Determine split point between LH and RH based on interpolation matrix column counts
    int nSourcesLh = 0;
    int nSourcesRh = 0;

    if (m_interpMatLh) {
        nSourcesLh = m_interpMatLh->cols();
    }
    if (m_interpMatRh) {
        nSourcesRh = m_interpMatRh->cols();
    }

    int nTotalExpected = nSourcesLh + nSourcesRh;

    // Handle case where data doesn't match expected source count
    if (vecAbsData.size() != nTotalExpected && nTotalExpected > 0) {
        // Try to use whatever data we have
        if (vecAbsData.size() < nTotalExpected) {
            mne::Log::warning("RtSourceDataWorker: Data size {} < expected {}", vecAbsData.size(), nTotalExpected);
            return;
        }
    }

    // Split data into LH and RH
    Eigen::VectorXf dataLh, dataRh;
    if (nSourcesLh > 0 && vecAbsData.size() >= nSourcesLh) {
        dataLh = vecAbsData.head(nSourcesLh);
    }
    if (nSourcesRh > 0 && vecAbsData.size() >= nSourcesLh + nSourcesRh) {
        dataRh = vecAbsData.segment(nSourcesLh, nSourcesRh);
    }

    // Check streaming mode
    if (!m_bStreamSmoothedData) {
        // Raw mode: emit source values without interpolation
        Eigen::VectorXd rawLh = dataLh.cast<double>();
        Eigen::VectorXd rawRh = dataRh.cast<double>();

        locker.unlock();
        emit newRtRawData(rawLh, rawRh);
        return;
    }

    // Compute per-vertex colors for each hemisphere
    std::vector<uint32_t> colorsLh = computeHemiColors(dataLh, m_interpMatLh, m_baseColorsLh);
    std::vector<uint32_t> colorsRh = computeHemiColors(dataRh, m_interpMatRh, m_baseColorsRh);

    // Unlock before emitting (avoid deadlock if slot is direct connection)
    locker.unlock();

    emit newRtSmoothedData(colorsLh, colorsRh);
}

//=============================================================================================================

std::vector<uint32_t> RtSourceDataWorker::computeHemiColors(
    const Eigen::VectorXf& sourceData,
    const std::shared_ptr<Eigen::SparseMatrix<float>>& interpMat,
    const std::vector<uint32_t>& baseColors) const
{
    if (sourceData.size() == 0 || !interpMat || interpMat->rows() == 0) {
        return std::vector<uint32_t>();
    }

    // Interpolate source data to all surface vertices
    Eigen::VectorXf interpolated = (*interpMat) * sourceData;

    int nVertices = interpolated.size();
    std::vector<uint32_t> colors(nVertices);

    bool hasBaseColors = (std::ssize(baseColors) == nVertices);

    double range = m_dThreshMax - m_dThreshMin;
    if (range <= 0.0)
        range = 1.0;

    for (int i = 0; i < nVertices; ++i) {
        float value = interpolated(i);

        // Normalize based on thresholds
        double normalized = (static_cast<double>(value) - m_dThreshMin) / range;
        normalized = std::clamp(normalized, 0.0, 1.0);

        // Calculate alpha based on threshold
        uint8_t alpha = 255;
        if (value < m_dThreshMin) {
            // Below threshold: use base surface color or transparent
            if (hasBaseColors) {
                colors[i] = baseColors[i];
            } else {
                colors[i] = 0; // Fully transparent
            }
            continue;
        } else if (value < m_dThreshMid) {
            // Fade in from min to mid
            double fadeRange = m_dThreshMid - m_dThreshMin;
            if (fadeRange > 0.0) {
                alpha = static_cast<uint8_t>(255.0 * (value - m_dThreshMin) / fadeRange);
            }

            // Blend activation color with base color
            if (hasBaseColors && alpha < 255) {
                uint32_t actColor = valueToColor(normalized, 255);
                uint32_t baseColor = baseColors[i];

                // Simple alpha blend: result = act * alpha/255 + base * (255-alpha)/255
                uint32_t aR = actColor & 0xFF;
                uint32_t aG = (actColor >> 8) & 0xFF;
                uint32_t aB = (actColor >> 16) & 0xFF;
                uint32_t bR = baseColor & 0xFF;
                uint32_t bG = (baseColor >> 8) & 0xFF;
                uint32_t bB = (baseColor >> 16) & 0xFF;

                uint32_t rR = (aR * alpha + bR * (255 - alpha)) / 255;
                uint32_t rG = (aG * alpha + bG * (255 - alpha)) / 255;
                uint32_t rB = (aB * alpha + bB * (255 - alpha)) / 255;

                colors[i] = packABGR(rR, rG, rB);
                continue;
            }
        }

        colors[i] = valueToColor(normalized, alpha);
    }

    return colors;
}

//=============================================================================================================

uint32_t RtSourceDataWorker::valueToColor(double value, uint8_t alpha) const
{
    QRgb rgb = mne::disp::ColorMap::valueToColor(value, m_sColormapType);

    uint32_t r = qRed(rgb);
    uint32_t g = qGreen(rgb);
    uint32_t b = qBlue(rgb);

    // Pack as ABGR (same format as BrainSurface uses)
    return packABGR(r, g, b, static_cast<uint32_t>(alpha));
}
