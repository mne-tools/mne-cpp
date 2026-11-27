//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsensordataworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-tick dense sensor-to-vertex mat-vec, symmetric normalisation and ABGR colour packing.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordataworker.h"

#include <mne/base/log.h>

#include "../../../../brain/rendertypes.h"
#include <mne/viz/backends/qt/plots/helpers/colormap.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <vector>

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataWorker::RtSensorDataWorker(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

void RtSensorDataWorker::addData(const Eigen::VectorXf& data)
{
    std::scoped_lock locker(m_mutex);

    // Cap queue size at sampling frequency (1 second of data)
    if (std::ssize(m_lDataQ) < static_cast<int>(m_dSFreq)) {
        m_lDataQ.push_back(data);
    }

    // Also store for looping
    if (m_bIsLooping) {
        m_lDataLoopQ.push_back(data);
        // Cap loop queue at 10 seconds of data
        if (std::ssize(m_lDataLoopQ) > static_cast<int>(m_dSFreq) * 10) {
            m_lDataLoopQ.pop_front();
        }
    }
}

//=============================================================================================================

void RtSensorDataWorker::clear()
{
    std::scoped_lock locker(m_mutex);
    m_lDataQ.clear();
    m_lDataLoopQ.clear();
    m_vecAverage = Eigen::VectorXf();
    m_iSampleCtr = 0;
    m_iCurrentSample = 0;
}

//=============================================================================================================

void RtSensorDataWorker::setMappingMatrix(const std::string& surfaceKey, std::shared_ptr<Eigen::MatrixXf> mat)
{
    std::scoped_lock locker(m_mutex);
    m_sSurfaceKey = surfaceKey;
    m_mappingMat = mat;
}

//=============================================================================================================

void RtSensorDataWorker::setNumberAverages(int numAvr)
{
    std::scoped_lock locker(m_mutex);
    m_iNumAverages = std::max(1, numAvr);
}

//=============================================================================================================

void RtSensorDataWorker::setColormapType(const std::string& name)
{
    std::scoped_lock locker(m_mutex);
    m_sColormapType = QString::fromStdString(name);
}

//=============================================================================================================

void RtSensorDataWorker::setThresholds(double min, double max)
{
    std::scoped_lock locker(m_mutex);
    if (min == 0.0 && max == 0.0) {
        m_bUseAutoNorm = true;
        m_dThreshMin = 0.0;
        m_dThreshMax = 0.0;
    } else {
        m_bUseAutoNorm = false;
        m_dThreshMin = min;
        m_dThreshMax = max;
    }
}

//=============================================================================================================

void RtSensorDataWorker::setLoopState(bool enabled)
{
    std::scoped_lock locker(m_mutex);
    m_bIsLooping = enabled;
}

//=============================================================================================================

void RtSensorDataWorker::setSFreq(double sFreq)
{
    std::scoped_lock locker(m_mutex);
    m_dSFreq = std::max(1.0, sFreq);
}

//=============================================================================================================

void RtSensorDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    std::scoped_lock locker(m_mutex);
    m_bStreamSmoothedData = bStreamSmoothedData;
}

//=============================================================================================================

void RtSensorDataWorker::streamData()
{
    std::unique_lock locker(m_mutex);

    // Try to get data from queue
    Eigen::VectorXf vecCurrentData;

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

        vecCurrentData = m_vecAverage / static_cast<float>(m_iSampleCtr);
        m_vecAverage = Eigen::VectorXf();
        m_iSampleCtr = 0;
    }

    // Check streaming mode
    if (!m_bStreamSmoothedData) {
        // Raw mode: emit measurement vector without mapping
        Eigen::VectorXf rawData = vecCurrentData;
        locker.unlock();
        emit newRtRawSensorData(rawData);
        return;
    }

    // Compute per-vertex colors using the dense mapping matrix
    std::vector<uint32_t> colors = computeSurfaceColors(vecCurrentData);
    const std::string surfaceKey = m_sSurfaceKey;

    // Unlock before emitting (avoid deadlock if slot is direct connection)
    locker.unlock();

    if (!colors.empty()) {
        emit newRtSensorColors(surfaceKey, colors);
    }
}

//=============================================================================================================

std::vector<uint32_t> RtSensorDataWorker::computeSurfaceColors(const Eigen::VectorXf& sensorData) const
{
    if (sensorData.size() == 0 || !m_mappingMat || m_mappingMat->rows() == 0) {
        return std::vector<uint32_t>();
    }

    // Validate dimensions
    if (m_mappingMat->cols() != sensorData.size()) {
        mne::Log::warning("RtSensorDataWorker: Mapping matrix cols {} != sensor data size {}", m_mappingMat->cols(), sensorData.size());
        return std::vector<uint32_t>();
    }

    // Map sensor data to surface vertices: mapped = M * meas
    Eigen::VectorXf mapped = (*m_mappingMat) * sensorData;

    int nVertices = mapped.size();

    // Determine normalization range
    float normMin, normMax;
    if (m_bUseAutoNorm) {
        // Symmetric auto-normalization: ±maxAbs
        float maxAbs = 0.0f;
        for (int i = 0; i < nVertices; ++i) {
            maxAbs = std::max(maxAbs, std::abs(mapped(i)));
        }
        if (maxAbs <= 0.0f)
            maxAbs = 1.0f;
        normMin = -maxAbs;
        normMax = maxAbs;
    } else {
        // Explicit thresholds
        normMin = static_cast<float>(m_dThreshMin);
        normMax = static_cast<float>(m_dThreshMax);
        if (normMax <= normMin)
            normMax = normMin + 1.0f;
    }

    float range = normMax - normMin;

    // Convert to per-vertex ABGR colours
    std::vector<uint32_t> colors(nVertices);

    for (int i = 0; i < nVertices; ++i) {
        // Normalisation: map [normMin, normMax] → [0, 1]
        double norm = static_cast<double>(mapped(i) - normMin) / static_cast<double>(range);
        norm = std::clamp(norm, 0.0, 1.0);

        QRgb rgb = mne::viz::qt::ColorMap::valueToColor(norm, m_sColormapType);

        uint32_t r = qRed(rgb);
        uint32_t g = qGreen(rgb);
        uint32_t b = qBlue(rgb);

        // Pack as ABGR (same format as BrainSurface uses)
        colors[i] = packABGR(r, g, b);
    }

    return colors;
}
