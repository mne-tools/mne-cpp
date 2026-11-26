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

#include "../renderable/brainsurface.h"
#include "../helpers/interpolation.h"
#include "../helpers/geometryinfo.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StcLoadingWorker::StcLoadingWorker(const std::filesystem::path& lhPath,
                                   const std::filesystem::path& rhPath,
                                   BrainSurface* lhSurface,
                                   BrainSurface* rhSurface,
                                   double cancelDist,
                                   QObject* parent)
: QObject(parent)
, m_lhPath(lhPath)
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
    emit progress(progressStart, std::format("Computing {} geodesic interpolation...", hemiLabel));

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
        emit progress(pct, std::format("{} geodesic interpolation {}/{}", hemiLabel, current, total));
    };

    auto interpMat = mne::viz::qt::GeometryInfo::scdcInterpolationMat(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        mne::viz::qt::Interpolation::cubic,
        cancelDist,
        progressCallback,
        &m_cancelled);

    if (m_cancelled.load(std::memory_order_relaxed))
        return std::shared_ptr<Eigen::SparseMatrix<float>>();

    emit progress(progressEnd, std::format("{} interpolation matrix ready", hemiLabel));
    return interpMat;
}

//=============================================================================================================

void StcLoadingWorker::process()
{
    emit progress(0, std::string("Loading STC files..."));

    // Load LH STC file
    if (!m_lhPath.empty()) {
        if (auto stc = mne::InvSourceEstimate::read(m_lhPath)) {
            m_stcLh = std::move(*stc);
            m_hasLh = true;
        } else {
            emit error(std::format("Failed to read LH STC file: {}", stc.error().what()));
        }
    }

    emit progress(5, std::string("Loading RH STC file..."));

    // Load RH STC file
    if (!m_rhPath.empty()) {
        if (auto stc = mne::InvSourceEstimate::read(m_rhPath)) {
            m_stcRh = std::move(*stc);
            m_hasRh = true;
        } else {
            emit error(std::format("Failed to read RH STC file: {}", stc.error().what()));
        }
    }

    if (!m_hasLh && !m_hasRh) {
        emit error(std::string("No STC data could be loaded."));
        emit finished(false);
        return;
    }

    if (m_cancelled.load(std::memory_order_relaxed)) {
        emit finished(false);
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
                emit finished(false);
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
                emit finished(false);
                return;
            }
        }
    }

    emit progress(95, std::string("Finalizing..."));
    emit progress(100, std::string("Complete"));
    emit finished(true);
}

} // namespace mne::viz::qt
