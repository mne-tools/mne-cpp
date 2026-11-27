//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     stcloadingworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Background STC file parse plus per-hemisphere sparse interpolation matrix construction.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stcloadingworker.h"

#include "../brainsurface.h"
#include "../interpolation.h"
#include "../geometryinfo.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <format>
#include <string>

namespace mne::viz
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StcLoadingWorker::StcLoadingWorker(const std::filesystem::path& lhPath,
                                   const std::filesystem::path& rhPath,
                                   BrainSurface* lhSurface,
                                   BrainSurface* rhSurface,
                                   double cancelDist)
: m_lhPath(lhPath)
, m_rhPath(rhPath)
, m_lhSurface(lhSurface)
, m_rhSurface(rhSurface)
, m_cancelDist(cancelDist)
{
}

//=============================================================================================================

StcLoadingWorker::~StcLoadingWorker()
{
}

//=============================================================================================================

std::shared_ptr<Eigen::SparseMatrix<float>> StcLoadingWorker::computeInterpolationMatrix(
    const Eigen::MatrixX3f& matVertices,
    Eigen::VectorXi& vecSourceVertices,
    double cancelDist,
    const std::string& hemiLabel,
    int progressStart,
    int progressEnd)
{
    if (m_progressCallback) {
        m_progressCallback(progressStart, std::format("Computing {} geodesic interpolation...", hemiLabel));
    }

    // Compute neighbors from the appropriate surface
    std::vector<Eigen::VectorXi> vecNeighbors;
    if (hemiLabel == "LH" && m_lhSurface) {
        vecNeighbors = m_lhSurface->computeNeighbors();
    } else if (hemiLabel == "RH" && m_rhSurface) {
        vecNeighbors = m_rhSurface->computeNeighbors();
    }

    // Use sparse SCDC+interpolation: runs Dijkstra per source vertex and directly
    // builds the sparse interpolation matrix without a dense intermediate distance table.
    // This produces identical results to the old scdc() + createInterpolationMat() pipeline
    // but uses ~18 MB instead of ~8.7 GB for a typical fsaverage source space.
    const int progressRange = progressEnd - progressStart;
    auto progressCallback = [this, &hemiLabel, progressStart, progressRange](int current, int total) {
        if (m_cancelled.load(std::memory_order_relaxed))
            return;
        int pct = progressStart + (progressRange * current) / total;
        if (m_progressCallback) {
            m_progressCallback(pct, std::format("{} geodesic interpolation {}/{}", hemiLabel, current, total));
        }
    };

    auto interpMat = mne::viz::GeometryInfo::scdcInterpolationMat(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        mne::viz::Interpolation::cubic,
        cancelDist,
        progressCallback,
        &m_cancelled);

    if (m_cancelled.load(std::memory_order_relaxed))
        return std::shared_ptr<Eigen::SparseMatrix<float>>();

    if (m_progressCallback) {
        m_progressCallback(progressEnd, std::format("{} interpolation matrix ready", hemiLabel));
    }
    return interpMat;
}

//=============================================================================================================

void StcLoadingWorker::process()
{
    if (m_progressCallback) {
        m_progressCallback(0, std::string("Loading STC files..."));
    }

    // Load LH STC file
    if (!m_lhPath.empty()) {
        if (auto stc = mne::InvSourceEstimate::read(m_lhPath)) {
            m_stcLh = std::move(*stc);
            m_hasLh = true;
        } else {
            if (m_errorCallback) {
                m_errorCallback(std::format("Failed to read LH STC file: {}", stc.error().what()));
            }
        }
    }

    if (m_progressCallback) {
        m_progressCallback(5, std::string("Loading RH STC file..."));
    }

    // Load RH STC file
    if (!m_rhPath.empty()) {
        if (auto stc = mne::InvSourceEstimate::read(m_rhPath)) {
            m_stcRh = std::move(*stc);
            m_hasRh = true;
        } else {
            if (m_errorCallback) {
                m_errorCallback(std::format("Failed to read RH STC file: {}", stc.error().what()));
            }
        }
    }

    if (!m_hasLh && !m_hasRh) {
        if (m_errorCallback) {
            m_errorCallback(std::string("No STC data could be loaded."));
        }
        if (m_finishedCallback) {
            m_finishedCallback(false);
        }
        return;
    }

    if (m_cancelled.load(std::memory_order_relaxed)) {
        if (m_finishedCallback) {
            m_finishedCallback(false);
        }
        return;
    }

    // Compute LH interpolation matrix
    if (m_hasLh && m_lhSurface) {
        Eigen::MatrixX3f matVertices = m_lhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcLh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatLh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "LH", 10, 45);
            if (m_cancelled.load(std::memory_order_relaxed)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false);
                }
                return;
            }
        }
    }

    // Compute RH interpolation matrix
    if (m_hasRh && m_rhSurface) {
        Eigen::MatrixX3f matVertices = m_rhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcRh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatRh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "RH", 50, 90);
            if (m_cancelled.load(std::memory_order_relaxed)) {
                if (m_finishedCallback) {
                    m_finishedCallback(false);
                }
                return;
            }
        }
    }

    if (m_progressCallback) {
        m_progressCallback(95, std::string("Finalizing..."));
    }
    if (m_progressCallback) {
        m_progressCallback(100, std::string("Complete"));
    }
    if (m_finishedCallback) {
        m_finishedCallback(true);
    }
}


//=============================================================================================================

void StcLoadingWorker::setProgressCallback(std::function<void(int, const std::string&)> callback)
{
    m_progressCallback = std::move(callback);
}

//=============================================================================================================

void StcLoadingWorker::setFinishedCallback(std::function<void(bool)> callback)
{
    m_finishedCallback = std::move(callback);
}

//=============================================================================================================

void StcLoadingWorker::setErrorCallback(std::function<void(const std::string&)> callback)
{
    m_errorCallback = std::move(callback);
}

} // namespace mne::viz
