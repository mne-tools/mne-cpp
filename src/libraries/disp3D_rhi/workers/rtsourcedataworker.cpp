//=============================================================================================================
/**
 * @file     rtsourcedataworker.cpp
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
 * @brief    RtSourceDataWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourcedataworker.h"
#include "core/rendertypes.h"

#include <disp/plots/helpers/colormap.h>
#include <QMutexLocker>
#include <QDebug>
#include <cmath>

using namespace DISP3DRHILIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceDataWorker::RtSourceDataWorker(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

void RtSourceDataWorker::addData(const Eigen::VectorXd &data)
{
    QMutexLocker locker(&m_mutex);

    // Cap queue size at sampling frequency (1 second of data)
    if (m_lDataQ.size() < static_cast<int>(m_dSFreq)) {
        m_lDataQ.append(data);
    }

    // Also store for looping
    if (m_bIsLooping) {
        m_lDataLoopQ.append(data);
        // Cap loop queue too
        if (m_lDataLoopQ.size() > static_cast<int>(m_dSFreq) * 10) {
            m_lDataLoopQ.removeFirst();
        }
    }
}

//=============================================================================================================

void RtSourceDataWorker::clear()
{
    QMutexLocker locker(&m_mutex);
    m_lDataQ.clear();
    m_lDataLoopQ.clear();
    m_vecAverage = Eigen::VectorXd();
    m_iSampleCtr = 0;
    m_iCurrentSample = 0;
}

//=============================================================================================================

void RtSourceDataWorker::setInterpolationMatrixLeft(QSharedPointer<Eigen::SparseMatrix<float>> mat)
{
    QMutexLocker locker(&m_mutex);
    m_interpMatLh = mat;
}

//=============================================================================================================

void RtSourceDataWorker::setInterpolationMatrixRight(QSharedPointer<Eigen::SparseMatrix<float>> mat)
{
    QMutexLocker locker(&m_mutex);
    m_interpMatRh = mat;
}

//=============================================================================================================

void RtSourceDataWorker::setNumberAverages(int numAvr)
{
    QMutexLocker locker(&m_mutex);
    m_iNumAverages = qMax(1, numAvr);
}

//=============================================================================================================

void RtSourceDataWorker::setColormapType(const QString &name)
{
    QMutexLocker locker(&m_mutex);
    m_sColormapType = name;
}

//=============================================================================================================

void RtSourceDataWorker::setThresholds(double min, double mid, double max)
{
    QMutexLocker locker(&m_mutex);
    m_dThreshMin = min;
    m_dThreshMid = mid;
    m_dThreshMax = max;
}

//=============================================================================================================

void RtSourceDataWorker::setLoopState(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_bIsLooping = enabled;
}

//=============================================================================================================

void RtSourceDataWorker::setSFreq(double sFreq)
{
    QMutexLocker locker(&m_mutex);
    m_dSFreq = qMax(1.0, sFreq);
}

//=============================================================================================================

void RtSourceDataWorker::setSurfaceColor(const QVector<uint32_t> &baseColorsLh,
                                          const QVector<uint32_t> &baseColorsRh)
{
    QMutexLocker locker(&m_mutex);
    m_baseColorsLh = baseColorsLh;
    m_baseColorsRh = baseColorsRh;
}

//=============================================================================================================

void RtSourceDataWorker::setStreamSmoothedData(bool bStreamSmoothedData)
{
    QMutexLocker locker(&m_mutex);
    m_bStreamSmoothedData = bStreamSmoothedData;
}

//=============================================================================================================

void RtSourceDataWorker::streamData()
{
    QMutexLocker locker(&m_mutex);

    // Try to get data from queue
    Eigen::VectorXd vecCurrentData;

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
            qWarning() << "RtSourceDataWorker: Data size" << vecAbsData.size()
                       << "< expected" << nTotalExpected;
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
    QVector<uint32_t> colorsLh = computeHemiColors(dataLh, m_interpMatLh, m_baseColorsLh);
    QVector<uint32_t> colorsRh = computeHemiColors(dataRh, m_interpMatRh, m_baseColorsRh);

    // Unlock before emitting (avoid deadlock if slot is direct connection)
    locker.unlock();

    emit newRtSmoothedData(colorsLh, colorsRh);
}

//=============================================================================================================

QVector<uint32_t> RtSourceDataWorker::computeHemiColors(
    const Eigen::VectorXf &sourceData,
    const QSharedPointer<Eigen::SparseMatrix<float>> &interpMat,
    const QVector<uint32_t> &baseColors) const
{
    if (sourceData.size() == 0 || !interpMat || interpMat->rows() == 0) {
        return QVector<uint32_t>();
    }

    // Interpolate source data to all surface vertices
    Eigen::VectorXf interpolated = (*interpMat) * sourceData;

    int nVertices = interpolated.size();
    QVector<uint32_t> colors(nVertices);

    bool hasBaseColors = (baseColors.size() == nVertices);

    double range = m_dThreshMax - m_dThreshMin;
    if (range <= 0.0) range = 1.0;

    for (int i = 0; i < nVertices; ++i) {
        float value = interpolated(i);

        // Normalize based on thresholds
        double normalized = (static_cast<double>(value) - m_dThreshMin) / range;
        normalized = qBound(0.0, normalized, 1.0);

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
    QRgb rgb = DISPLIB::ColorMap::valueToColor(value, m_sColormapType);

    uint32_t r = qRed(rgb);
    uint32_t g = qGreen(rgb);
    uint32_t b = qBlue(rgb);

    // Pack as ABGR (same format as BrainSurface uses)
    return packABGR(r, g, b, static_cast<uint32_t>(alpha));
}
