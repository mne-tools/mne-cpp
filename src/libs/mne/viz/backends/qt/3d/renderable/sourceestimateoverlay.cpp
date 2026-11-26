//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourceestimateoverlay.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    STC file loading, per-vertex activation interpolation and colormap packing onto the target cortical mesh.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimateoverlay.h"

#include <mne/base/log.h>

#include "brainsurface.h"
#include "../core/rendertypes.h"
#include "../helpers/interpolation.h"
#include "../helpers/geometryinfo.h"
#include <mne/viz/backends/qt/plots/helpers/colormap.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <vector>
#include <utility>

namespace mne::viz::qt
{

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

bool SourceEstimateOverlay::loadStc(const std::filesystem::path& path, int hemi)
{
    auto read = mne::InvSourceEstimate::read(path);
    if (!read) {
        mne::Log::warning("SourceEstimateOverlay::loadStc - {}", read.error().what());
        return false;
    }
    mne::InvSourceEstimate& stc = *read;

    if (hemi == 0) {
        m_stcLh = stc;
        m_hasLh = true;
        mne::Log::debug("SourceEstimateOverlay: Loaded LH with {} vertices, {} time points", stc.data.rows(), stc.data.cols());
    } else {
        m_stcRh = stc;
        m_hasRh = true;
        mne::Log::debug("SourceEstimateOverlay: Loaded RH with {} vertices, {} time points", stc.data.rows(), stc.data.cols());
    }
    invalidateColorCache();

    // Auto-set thresholds based on data range
    if (m_hasLh || m_hasRh) {
        double minVal, maxVal;
        getDataRange(minVal, maxVal);
        m_threshMin = minVal;
        m_threshMax = maxVal;
        m_threshMid = (minVal + maxVal) / 2.0;
        mne::Log::debug("SourceEstimateOverlay: Auto thresholds set to {} {} {}", m_threshMin, m_threshMid, m_threshMax);
    }

    return true;
}

//=============================================================================================================

bool SourceEstimateOverlay::isLoaded() const
{
    return m_hasLh || m_hasRh;
}

//=============================================================================================================

void SourceEstimateOverlay::applyToSurface(BrainSurface* surface, int timeIndex)
{
    if (!surface)
        return;

    int hemi = surface->hemi();
    const mne::InvSourceEstimate* stc = nullptr;
    std::shared_ptr<Eigen::SparseMatrix<float>> interpMat;

    if (hemi == 0 && m_hasLh) {
        stc = &m_stcLh;
        interpMat = m_interpolationMatLh;
    } else if (hemi == 1 && m_hasRh) {
        stc = &m_stcRh;
        interpMat = m_interpolationMatRh;
    } else {
        return; // No data for this hemisphere
    }

    if (stc->empty())
        return;

    // Clamp time index
    int tIdx = std::clamp(timeIndex, 0, std::max(0, static_cast<int>(stc->data.cols()) - 1));
    const uint32_t vertexCount = surface->vertexCount();

    // ── Color cache fast-path ──────────────────────────────────────
    // Hot loop while scrubbing or auto-looping: avoid the sparse
    // interpolation matvec and per-vertex colormap evaluation by
    // returning the pre-computed buffer for this (hemi, vertexCount,
    // timeIndex). The cache is invalidated whenever colormap,
    // thresholds or source data change.
    const ColorCacheKey cacheKey(hemi, static_cast<int>(vertexCount));
    auto bucketIt = m_colorCache.find(cacheKey);
    if (bucketIt != m_colorCache.end()) {
        auto frameIt = bucketIt->second.find(tIdx);
        if (frameIt != bucketIt->second.end()) {
            surface->applySourceEstimateColors(frameIt->second);
            return;
        }
    }

    // Get source data for this time point
    Eigen::VectorXf sourceData = stc->data.col(tIdx).cwiseAbs().cast<float>();

    // Create color array for all surface vertices
    std::vector<uint32_t> colors(vertexCount, 0xFF808080); // Default gray

    // Determine if we have an interpolation matrix
    Eigen::VectorXf interpolatedData;

    if (interpMat && interpMat->rows() == static_cast<int>(vertexCount) &&
        interpMat->cols() == sourceData.size()) {
        // Use interpolation to spread values to all vertices
        // Note: interpolateSignal returns by value, not a shared pointer, so assignment matches
        interpolatedData = mne::viz::qt::Interpolation::interpolateSignal(interpMat, std::make_shared<Eigen::VectorXf>(sourceData));
    } else {
        // Fall back to sparse visualization (direct mapping)
        interpolatedData = Eigen::VectorXf::Zero(vertexCount);
        const Eigen::VectorXi& srcVertices = stc->vertices;
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
            normalized = std::clamp(normalized, 0.0, 1.0);
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

    // Store in cache before handing off to the surface so back-scrubs
    // and auto-loop iterations are free.
    m_colorCache[cacheKey].insert_or_assign(tIdx, colors);

    surface->applySourceEstimateColors(colors);
}

//=============================================================================================================

void SourceEstimateOverlay::setColormap(const std::string& name)
{
    if (m_colormap == name)
        return;
    m_colormap = name;
    m_colormapQt = QString::fromStdString(name);
    invalidateColorCache();
}

//=============================================================================================================

void SourceEstimateOverlay::setThresholds(float min, float mid, float max)
{
    if (m_threshMin == min && m_threshMid == mid && m_threshMax == max)
        return;
    m_threshMin = min;
    m_threshMid = mid;
    m_threshMax = max;
    invalidateColorCache();
}

//=============================================================================================================

int SourceEstimateOverlay::numTimePoints() const
{
    if (m_hasLh)
        return m_stcLh.data.cols();
    if (m_hasRh)
        return m_stcRh.data.cols();
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
    if (m_hasLh)
        return m_stcLh.tmin;
    if (m_hasRh)
        return m_stcRh.tmin;
    return 0.0f;
}

//=============================================================================================================

float SourceEstimateOverlay::tstep() const
{
    if (m_hasLh)
        return m_stcLh.tstep;
    if (m_hasRh)
        return m_stcRh.tstep;
    return 0.0f;
}

//=============================================================================================================

void SourceEstimateOverlay::getDataRange(double& minVal, double& maxVal) const
{
    minVal = std::numeric_limits<double>::max();
    maxVal = std::numeric_limits<double>::lowest();

    // applyToSurface() colours |data|, so the range is that of the magnitudes
    for (const mne::InvSourceEstimate* stc : {m_hasLh ? &m_stcLh : nullptr, m_hasRh ? &m_stcRh : nullptr}) {
        if (stc && stc->data.size() > 0) {
            minVal = std::min(minVal, stc->data.cwiseAbs().minCoeff());
            maxVal = std::max(maxVal, stc->data.cwiseAbs().maxCoeff());
        }
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
    QRgb rgb = mne::viz::qt::ColorMap::valueToColor(value, m_colormapQt);

    uint32_t r = qRed(rgb);
    uint32_t g = qGreen(rgb);
    uint32_t b = qBlue(rgb);

    // Pack as ABGR (same format as BrainSurface uses)
    return packABGR(r, g, b, static_cast<uint32_t>(alpha));
}

//=============================================================================================================

void SourceEstimateOverlay::computeInterpolationMatrix(BrainSurface* surface, int hemi, double cancelDist)
{
    if (!surface)
        return;

    const mne::InvSourceEstimate* stc = nullptr;
    std::shared_ptr<Eigen::SparseMatrix<float>>* pMatPtr = nullptr;

    if (hemi == 0 && m_hasLh) {
        stc = &m_stcLh;
        pMatPtr = &m_interpolationMatLh;
    } else if (hemi == 1 && m_hasRh) {
        stc = &m_stcRh;
        pMatPtr = &m_interpolationMatRh;
    } else {
        return;
    }

    if (stc->empty())
        return;

    mne::Log::debug("SourceEstimateOverlay: Computing interpolation matrix for hemi {}", hemi);

    // Get vertices and neighbor information needed for Dijkstra (SCDC)
    Eigen::MatrixX3f matVertices = surface->verticesAsMatrix();
    std::vector<Eigen::VectorXi> vecNeighbors = surface->computeNeighbors();

    // Source vertex subset from STC (already a VectorXi)
    Eigen::VectorXi vecSourceVertices = stc->vertices;

    mne::Log::debug("SourceEstimateOverlay: FsSurface has {} vertices, {} sources", matVertices.rows(), vecSourceVertices.size());

    if (vecSourceVertices.size() == 0) {
        mne::Log::warning("SourceEstimateOverlay: No source vertices found");
        return;
    }

    // 1. Calculate Distance Table (Geodesic distance on surface)
    // This uses Dijkstra's algorithm via GeometryInfo::scdc
    // Note: This can be slow for many sources!
    mne::Log::debug("SourceEstimateOverlay: Computing distance table (SCDC)...");
    std::shared_ptr<Eigen::MatrixXd> distTable = mne::viz::qt::GeometryInfo::scdc(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        cancelDist);

    if (!distTable || distTable->rows() == 0) {
        mne::Log::warning("SourceEstimateOverlay: Failed to compute distance table");
        return;
    }

    // 2. Create Interpolation Matrix
    mne::Log::debug("SourceEstimateOverlay: Creating interpolation matrix...");
    *pMatPtr = mne::viz::qt::Interpolation::createInterpolationMat(
        vecSourceVertices,
        distTable,
        mne::viz::qt::Interpolation::cubic, // Use cubic interpolation function
        cancelDist);

    if (*pMatPtr && (*pMatPtr)->rows() > 0) {
        mne::Log::debug("SourceEstimateOverlay: Interpolation matrix created: {} x {}", (*pMatPtr)->rows(), (*pMatPtr)->cols());
    } else {
        mne::Log::warning("SourceEstimateOverlay: Failed to compute interpolation matrix");
    }
}

//=============================================================================================================

void SourceEstimateOverlay::setStcData(const mne::InvSourceEstimate& stc, int hemi)
{
    if (hemi == 0) {
        m_stcLh = stc;
        m_hasLh = true;
    } else {
        m_stcRh = stc;
        m_hasRh = true;
    }
    invalidateColorCache();
}

//=============================================================================================================

void SourceEstimateOverlay::setInterpolationMatrix(std::shared_ptr<Eigen::SparseMatrix<float>> mat, int hemi)
{
    if (hemi == 0) {
        m_interpolationMatLh = std::move(mat);
    } else {
        m_interpolationMatRh = std::move(mat);
    }
    invalidateColorCache();
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
        invalidateColorCache();
        mne::Log::debug("SourceEstimateOverlay: Auto thresholds set to {} {} {}", m_threshMin, m_threshMid, m_threshMax);
    }
}

//=============================================================================================================

Eigen::VectorXd SourceEstimateOverlay::sourceDataColumn(int timeIndex) const
{
    int nLh = m_hasLh ? m_stcLh.data.rows() : 0;
    int nRh = m_hasRh ? m_stcRh.data.rows() : 0;

    if (nLh == 0 && nRh == 0) {
        return Eigen::VectorXd();
    }

    Eigen::VectorXd result(nLh + nRh);

    if (m_hasLh) {
        int tIdx = std::clamp(timeIndex, 0, std::max(0, static_cast<int>(m_stcLh.data.cols()) - 1));
        result.head(nLh) = m_stcLh.data.col(tIdx);
    }

    if (m_hasRh) {
        int tIdx = std::clamp(timeIndex, 0, std::max(0, static_cast<int>(m_stcRh.data.cols()) - 1));
        result.segment(nLh, nRh) = m_stcRh.data.col(tIdx);
    }

    return result;
}

} // namespace mne::viz::qt
