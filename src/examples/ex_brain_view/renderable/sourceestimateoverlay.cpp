//=============================================================================================================
/**
 * @file     sourceestimateoverlay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     January, 2026
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
 * @brief    SourceEstimateOverlay class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimateoverlay.h"
#include "brainsurface.h"

#include <disp/plots/helpers/colormap.h>
#include "helpers/interpolation.h"
#include "helpers/geometryinfo.h"

#include <QFile>
#include <QDebug>
#include <cmath>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateOverlay::SourceEstimateOverlay()
{
}

//=============================================================================================================

SourceEstimateOverlay::~SourceEstimateOverlay()
{
}

//=============================================================================================================

bool SourceEstimateOverlay::loadStc(const QString &path, int hemi)
{
    QFile file(path);
    // Note: MNESourceEstimate::read() opens the file internally, don't open it here

    MNELIB::MNESourceEstimate stc;
    if (!MNELIB::MNESourceEstimate::read(file, stc)) {
        qWarning() << "SourceEstimateOverlay::loadStc - Failed to read STC file:" << path;
        return false;
    }

    if (hemi == 0) {
        m_stcLh = stc;
        m_hasLh = true;
        qDebug() << "SourceEstimateOverlay: Loaded LH with" << stc.data.rows() << "vertices," 
                 << stc.data.cols() << "time points";
    } else {
        m_stcRh = stc;
        m_hasRh = true;
        qDebug() << "SourceEstimateOverlay: Loaded RH with" << stc.data.rows() << "vertices," 
                 << stc.data.cols() << "time points";
    }

    // Auto-set thresholds based on data range
    if (m_hasLh || m_hasRh) {
        double minVal, maxVal;
        getDataRange(minVal, maxVal);
        m_threshMin = minVal;
        m_threshMax = maxVal;
        m_threshMid = (minVal + maxVal) / 2.0;
        qDebug() << "SourceEstimateOverlay: Auto thresholds set to" << m_threshMin << m_threshMid << m_threshMax;
    }

    return true;
}

//=============================================================================================================

bool SourceEstimateOverlay::isLoaded() const
{
    return m_hasLh || m_hasRh;
}

//=============================================================================================================

void SourceEstimateOverlay::applyToSurface(BrainSurface *surface, int timeIndex)
{
    if (!surface) return;

    int hemi = surface->hemi();
    const MNELIB::MNESourceEstimate *stc = nullptr;
    QSharedPointer<Eigen::SparseMatrix<float>> interpMat;

    if (hemi == 0 && m_hasLh) {
        stc = &m_stcLh;
        interpMat = m_interpolationMatLh;
    } else if (hemi == 1 && m_hasRh) {
        stc = &m_stcRh;
        interpMat = m_interpolationMatRh;
    } else {
        return; // No data for this hemisphere
    }

    if (stc->isEmpty()) return;

    // Clamp time index
    int tIdx = qBound(0, timeIndex, static_cast<int>(stc->data.cols()) - 1);

    // Get source data for this time point
    Eigen::VectorXf sourceData = stc->data.col(tIdx).cwiseAbs().cast<float>();

    // Create color array for all surface vertices
    uint32_t vertexCount = surface->vertexCount();
    QVector<uint32_t> colors(vertexCount, 0xFF808080); // Default gray

    // Determine if we have an interpolation matrix
    Eigen::VectorXf interpolatedData;
    
    if (interpMat && interpMat->rows() == static_cast<int>(vertexCount) && 
        interpMat->cols() == sourceData.size()) {
        // Use interpolation to spread values to all vertices
        // Note: interpolateSignal returns by value, not QSharedPointer, so assignment matches
        interpolatedData = BRAINVIEWLIB::Interpolation::interpolateSignal(interpMat, QSharedPointer<Eigen::VectorXf>::create(sourceData));
    } else {
        // Fall back to sparse visualization (direct mapping)
        interpolatedData = Eigen::VectorXf::Zero(vertexCount);
        const Eigen::VectorXi &srcVertices = stc->vertices;
        for (int i = 0; i < srcVertices.size() && i < sourceData.size(); ++i) {
            int vertIdx = srcVertices(i);
            if (vertIdx >= 0 && vertIdx < static_cast<int>(vertexCount)) {
                interpolatedData(vertIdx) = sourceData(i);
            }
        }
    }

    // Convert interpolated values to colors
    for (int i = 0; i < static_cast<int>(vertexCount); ++i) {
        float value = interpolatedData(i);

        // Normalize based on thresholds
        double normalized = 0.0;
        if (m_threshMax > m_threshMin) {
            normalized = (value - m_threshMin) / (m_threshMax - m_threshMin);
            normalized = qBound(0.0, normalized, 1.0);
        }

        // Calculate alpha based on threshold
        uint8_t alpha = 255;
        if (value < m_threshMin) {
            alpha = 0; // Fully transparent below minimum
        } else if (value < m_threshMid) {
            // Fade in from min to mid
            float range = m_threshMid - m_threshMin;
            if (range > 0) {
                alpha = static_cast<uint8_t>(255.0f * (value - m_threshMin) / range);
            }
        }

        colors[i] = valueToColor(normalized, alpha);
    }

    surface->applySourceEstimateColors(colors);
}

//=============================================================================================================

void SourceEstimateOverlay::setColormap(const QString &name)
{
    m_colormap = name;
}

//=============================================================================================================

void SourceEstimateOverlay::setThresholds(float min, float mid, float max)
{
    m_threshMin = min;
    m_threshMid = mid;
    m_threshMax = max;
}

//=============================================================================================================

int SourceEstimateOverlay::numTimePoints() const
{
    if (m_hasLh) return m_stcLh.data.cols();
    if (m_hasRh) return m_stcRh.data.cols();
    return 0;
}

//=============================================================================================================

float SourceEstimateOverlay::timeAtIndex(int idx) const
{
    if (m_hasLh && idx < m_stcLh.times.size()) {
        return m_stcLh.times(idx);
    }
    if (m_hasRh && idx < m_stcRh.times.size()) {
        return m_stcRh.times(idx);
    }
    return 0.0f;
}

//=============================================================================================================

float SourceEstimateOverlay::tmin() const
{
    if (m_hasLh) return m_stcLh.tmin;
    if (m_hasRh) return m_stcRh.tmin;
    return 0.0f;
}

//=============================================================================================================

float SourceEstimateOverlay::tstep() const
{
    if (m_hasLh) return m_stcLh.tstep;
    if (m_hasRh) return m_stcRh.tstep;
    return 0.0f;
}

//=============================================================================================================

void SourceEstimateOverlay::getDataRange(double &minVal, double &maxVal) const
{
    minVal = std::numeric_limits<double>::max();
    maxVal = std::numeric_limits<double>::lowest();

    if (m_hasLh) {
        double lhMin = m_stcLh.data.minCoeff();
        double lhMax = m_stcLh.data.maxCoeff();
        minVal = qMin(minVal, std::abs(lhMin));
        maxVal = qMax(maxVal, std::abs(lhMax));
    }

    if (m_hasRh) {
        double rhMin = m_stcRh.data.minCoeff();
        double rhMax = m_stcRh.data.maxCoeff();
        minVal = qMin(minVal, std::abs(rhMin));
        maxVal = qMax(maxVal, std::abs(rhMax));
    }

    // If no data, set defaults
    if (minVal > maxVal) {
        minVal = 0.0;
        maxVal = 1.0;
    }
}

//=============================================================================================================

uint32_t SourceEstimateOverlay::valueToColor(double value, uint8_t alpha) const
{
    QRgb rgb = DISPLIB::ColorMap::valueToColor(value, m_colormap);

    uint32_t r = qRed(rgb);
    uint32_t g = qGreen(rgb);
    uint32_t b = qBlue(rgb);

    // Pack as ABGR (same format as BrainSurface uses)
    return (static_cast<uint32_t>(alpha) << 24) | (b << 16) | (g << 8) | r;
}

//=============================================================================================================

void SourceEstimateOverlay::computeInterpolationMatrix(BrainSurface *surface, int hemi, double cancelDist)
{
    if (!surface) return;

    const MNELIB::MNESourceEstimate *stc = nullptr;
    QSharedPointer<Eigen::SparseMatrix<float>> *pMatPtr = nullptr;

    if (hemi == 0 && m_hasLh) {
        stc = &m_stcLh;
        pMatPtr = &m_interpolationMatLh;
    } else if (hemi == 1 && m_hasRh) {
        stc = &m_stcRh;
        pMatPtr = &m_interpolationMatRh;
    } else {
        return;
    }

    if (stc->isEmpty()) return;

    qDebug() << "SourceEstimateOverlay: Computing interpolation matrix for hemi" << hemi;

    // Get vertices and neighbor information needed for Dijkstra (SCDC)
    Eigen::MatrixX3f matVertices = surface->verticesAsMatrix();
    QVector<QVector<int>> vecNeighbors = surface->computeNeighbors();

    // Build source vertex subset from STC
    QVector<int> vecSourceVertices;
    vecSourceVertices.reserve(stc->vertices.size());
    for (int i = 0; i < stc->vertices.size(); ++i) {
        vecSourceVertices.append(stc->vertices(i));
    }

    qDebug() << "SourceEstimateOverlay: Surface has" << matVertices.rows() << "vertices,"
             << vecSourceVertices.size() << "sources";
    
    if (vecSourceVertices.isEmpty()) {
        qWarning() << "SourceEstimateOverlay: No source vertices found";
        return;
    }

    // 1. Calculate Distance Table (Geodesic distance on surface)
    // This uses Dijkstra's algorithm via GeometryInfo::scdc
    // Note: This can be slow for many sources!
    qDebug() << "SourceEstimateOverlay: Computing distance table (SCDC)...";
    QSharedPointer<Eigen::MatrixXd> distTable = BRAINVIEWLIB::GeometryInfo::scdc(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        cancelDist
    );

    if (!distTable || distTable->rows() == 0) {
        qWarning() << "SourceEstimateOverlay: Failed to compute distance table";
        return;
    }

    // 2. Create Interpolation Matrix
    qDebug() << "SourceEstimateOverlay: Creating interpolation matrix...";
    *pMatPtr = BRAINVIEWLIB::Interpolation::createInterpolationMat(
        vecSourceVertices,
        distTable,
        BRAINVIEWLIB::Interpolation::cubic,  // Use cubic interpolation function
        cancelDist
    );

    if (*pMatPtr && (*pMatPtr)->rows() > 0) {
        qDebug() << "SourceEstimateOverlay: Interpolation matrix created:" 
                 << (*pMatPtr)->rows() << "x" << (*pMatPtr)->cols();
    } else {
        qWarning() << "SourceEstimateOverlay: Failed to compute interpolation matrix";
    }
}

//=============================================================================================================

void SourceEstimateOverlay::setStcData(const MNELIB::MNESourceEstimate &stc, int hemi)
{
    if (hemi == 0) {
        m_stcLh = stc;
        m_hasLh = true;
    } else {
        m_stcRh = stc;
        m_hasRh = true;
    }
}

//=============================================================================================================

void SourceEstimateOverlay::setInterpolationMatrix(QSharedPointer<Eigen::SparseMatrix<float>> mat, int hemi)
{
    if (hemi == 0) {
        m_interpolationMatLh = mat;
    } else {
        m_interpolationMatRh = mat;
    }
}

//=============================================================================================================

void SourceEstimateOverlay::updateThresholdsFromData()
{
    if (m_hasLh || m_hasRh) {
        double minVal, maxVal;
        getDataRange(minVal, maxVal);
        m_threshMin = minVal;
        m_threshMax = maxVal;
        m_threshMid = (minVal + maxVal) / 2.0;
        qDebug() << "SourceEstimateOverlay: Auto thresholds set to" << m_threshMin << m_threshMid << m_threshMax;
    }
}
