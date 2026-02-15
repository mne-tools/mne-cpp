//=============================================================================================================
/**
 * @file     rtsensordataworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 * @brief    RtSensorDataWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsensordataworker.h"

#include <disp/plots/helpers/colormap.h>
#include <QMutexLocker>
#include <QDebug>
#include <cmath>

using namespace DISP3DRHILIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSensorDataWorker::RtSensorDataWorker(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

void RtSensorDataWorker::addData(const Eigen::VectorXf &data)
{
    QMutexLocker locker(&m_mutex);

    // Cap queue size at sampling frequency (1 second of data)
    if (m_lDataQ.size() < static_cast<int>(m_dSFreq)) {
        m_lDataQ.append(data);
    }

    // Also store for looping
    if (m_bIsLooping) {
        m_lDataLoopQ.append(data);
        // Cap loop queue at 10 seconds of data
        if (m_lDataLoopQ.size() > static_cast<int>(m_dSFreq) * 10) {
            m_lDataLoopQ.removeFirst();
        }
    }
}

//=============================================================================================================

void RtSensorDataWorker::clear()
{
    QMutexLocker locker(&m_mutex);
    m_lDataQ.clear();
    m_lDataLoopQ.clear();
    m_vecAverage = Eigen::VectorXf();
    m_iSampleCtr = 0;
    m_iCurrentSample = 0;
}

//=============================================================================================================

void RtSensorDataWorker::setMappingMatrix(QSharedPointer<Eigen::MatrixXf> mat)
{
    QMutexLocker locker(&m_mutex);
    m_mappingMat = mat;
}

//=============================================================================================================

void RtSensorDataWorker::setNumberAverages(int numAvr)
{
    QMutexLocker locker(&m_mutex);
    m_iNumAverages = qMax(1, numAvr);
}

//=============================================================================================================

void RtSensorDataWorker::setColormapType(const QString &name)
{
    QMutexLocker locker(&m_mutex);
    m_sColormapType = name;
}

//=============================================================================================================

void RtSensorDataWorker::setThresholds(double min, double max)
{
    QMutexLocker locker(&m_mutex);
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
    QMutexLocker locker(&m_mutex);
    m_bIsLooping = enabled;
}

//=============================================================================================================

void RtSensorDataWorker::setSFreq(double sFreq)
{
    QMutexLocker locker(&m_mutex);
    m_dSFreq = qMax(1.0, sFreq);
}

//=============================================================================================================

void RtSensorDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    QMutexLocker locker(&m_mutex);
    m_bStreamSmoothedData = bStreamSmoothedData;
}

//=============================================================================================================

void RtSensorDataWorker::streamData()
{
    QMutexLocker locker(&m_mutex);

    // Try to get data from queue
    Eigen::VectorXf vecCurrentData;

    if (!m_lDataQ.isEmpty()) {
        vecCurrentData = m_lDataQ.takeFirst();
    } else if (m_bIsLooping && !m_lDataLoopQ.isEmpty()) {
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
    QVector<uint32_t> colors = computeSurfaceColors(vecCurrentData);

    // Unlock before emitting (avoid deadlock if slot is direct connection)
    locker.unlock();

    if (!colors.isEmpty()) {
        emit newRtSensorColors(m_sSurfaceKey, colors);
    }
}

//=============================================================================================================

QVector<uint32_t> RtSensorDataWorker::computeSurfaceColors(const Eigen::VectorXf &sensorData) const
{
    if (sensorData.size() == 0 || !m_mappingMat || m_mappingMat->rows() == 0) {
        return QVector<uint32_t>();
    }

    // Validate dimensions
    if (m_mappingMat->cols() != sensorData.size()) {
        qWarning() << "RtSensorDataWorker: Mapping matrix cols" << m_mappingMat->cols()
                   << "!= sensor data size" << sensorData.size();
        return QVector<uint32_t>();
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
        if (maxAbs <= 0.0f) maxAbs = 1.0f;
        normMin = -maxAbs;
        normMax = maxAbs;
    } else {
        // Explicit thresholds
        normMin = static_cast<float>(m_dThreshMin);
        normMax = static_cast<float>(m_dThreshMax);
        if (normMax <= normMin) normMax = normMin + 1.0f;
    }

    float range = normMax - normMin;

    // Convert to per-vertex ABGR colours
    QVector<uint32_t> colors(nVertices);

    for (int i = 0; i < nVertices; ++i) {
        // Normalisation: map [normMin, normMax] → [0, 1]
        double norm = static_cast<double>(mapped(i) - normMin) / static_cast<double>(range);
        norm = qBound(0.0, norm, 1.0);

        QRgb rgb = DISPLIB::ColorMap::valueToColor(norm, m_sColormapType);

        uint32_t r = qRed(rgb);
        uint32_t g = qGreen(rgb);
        uint32_t b = qBlue(rgb);

        // Pack as ABGR (same format as BrainSurface uses)
        colors[i] = (0xFFu << 24) | (b << 16) | (g << 8) | r;
    }

    return colors;
}
