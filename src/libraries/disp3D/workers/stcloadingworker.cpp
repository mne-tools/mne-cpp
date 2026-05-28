//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file stcloadingworker.cpp
 * @since March 2026
 * @brief Background STC file parse plus per-hemisphere sparse interpolation matrix construction.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "stcloadingworker.h"
#include "renderable/brainsurface.h"

#include "helpers/interpolation.h"
#include "helpers/geometryinfo.h"

#include <QFile>
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StcLoadingWorker::StcLoadingWorker(const QString &lhPath,
                                   const QString &rhPath,
                                   BrainSurface *lhSurface,
                                   BrainSurface *rhSurface,
                                   double cancelDist,
                                   QObject *parent)
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

QSharedPointer<Eigen::SparseMatrix<float>> StcLoadingWorker::computeInterpolationMatrix(
    const Eigen::MatrixX3f &matVertices,
    Eigen::VectorXi &vecSourceVertices,
    double cancelDist,
    const QString &hemiLabel,
    int progressStart,
    int progressEnd)
{
    emit progress(progressStart, QString("Computing %1 geodesic interpolation...").arg(hemiLabel));

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
        emit progress(pct, QString("%1 geodesic interpolation %2/%3").arg(hemiLabel).arg(current).arg(total));
    };

    auto interpMat = DISP3DLIB::GeometryInfo::scdcInterpolationMat(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        DISP3DLIB::Interpolation::cubic,
        cancelDist,
        progressCallback,
        &m_cancelled
    );

    if (m_cancelled.load(std::memory_order_relaxed))
        return QSharedPointer<Eigen::SparseMatrix<float>>();

    emit progress(progressEnd, QString("%1 interpolation matrix ready").arg(hemiLabel));
    return interpMat;
}

//=============================================================================================================

void StcLoadingWorker::process()
{
    emit progress(0, "Loading STC files...");

    // Load LH STC file
    if (!m_lhPath.isEmpty()) {
        QFile file(m_lhPath);
        INVLIB::InvSourceEstimate stc;
        if (INVLIB::InvSourceEstimate::read(file, stc)) {
            m_stcLh = stc;
            m_hasLh = true;
        } else {
            emit error(QString("Failed to read LH STC file: %1").arg(m_lhPath));
        }
    }

    emit progress(5, "Loading RH STC file...");

    // Load RH STC file
    if (!m_rhPath.isEmpty()) {
        QFile file(m_rhPath);
        INVLIB::InvSourceEstimate stc;
        if (INVLIB::InvSourceEstimate::read(file, stc)) {
            m_stcRh = stc;
            m_hasRh = true;
        } else {
            emit error(QString("Failed to read RH STC file: %1").arg(m_rhPath));
        }
    }

    if (!m_hasLh && !m_hasRh) {
        emit error("No STC data could be loaded.");
        emit finished(false);
        return;
    }

    if (m_cancelled.load(std::memory_order_relaxed)) { emit finished(false); return; }

    // Compute LH interpolation matrix
    if (m_hasLh && m_lhSurface) {
        Eigen::MatrixX3f matVertices = m_lhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcLh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatLh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "LH", 10, 45);
            if (m_cancelled.load(std::memory_order_relaxed)) { emit finished(false); return; }
        }
    }

    // Compute RH interpolation matrix
    if (m_hasRh && m_rhSurface) {
        Eigen::MatrixX3f matVertices = m_rhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcRh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatRh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "RH", 50, 90);
            if (m_cancelled.load(std::memory_order_relaxed)) { emit finished(false); return; }
        }
    }

    emit progress(95, "Finalizing...");
    emit progress(100, "Complete");
    emit finished(true);
}
