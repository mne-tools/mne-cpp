//=============================================================================================================
/**
 * @file     stcloadingworker.cpp
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
 * @brief    StcLoadingWorker class definition.
 *
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
        int pct = progressStart + (progressRange * current) / total;
        emit progress(pct, QString("%1 geodesic interpolation %2/%3").arg(hemiLabel).arg(current).arg(total));
    };

    auto interpMat = DISP3DLIB::GeometryInfo::scdcInterpolationMat(
        matVertices,
        vecNeighbors,
        vecSourceVertices,
        DISP3DLIB::Interpolation::cubic,
        cancelDist,
        progressCallback
    );

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

    // Compute LH interpolation matrix
    if (m_hasLh && m_lhSurface) {
        Eigen::MatrixX3f matVertices = m_lhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcLh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatLh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "LH", 10, 45);
        }
    }

    // Compute RH interpolation matrix
    if (m_hasRh && m_rhSurface) {
        Eigen::MatrixX3f matVertices = m_rhSurface->verticesAsMatrix();
        Eigen::VectorXi vecSourceVertices = m_stcRh.vertices;

        if (vecSourceVertices.size() > 0) {
            m_interpMatRh = computeInterpolationMatrix(matVertices, vecSourceVertices, m_cancelDist,
                                                       "RH", 50, 90);
        }
    }

    emit progress(95, "Finalizing...");
    emit progress(100, "Complete");
    emit finished(true);
}
