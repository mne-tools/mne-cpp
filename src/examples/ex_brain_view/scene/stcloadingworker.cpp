//=============================================================================================================
/**
 * @file     stcloadingworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
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
#include "brainsurface.h"

#include <disp3D/helpers/interpolation/interpolation.h>
#include <disp3D/helpers/geometryinfo/geometryinfo.h>

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

void StcLoadingWorker::process()
{
    emit progress(0, "Loading STC files...");

    // Load LH STC file
    if (!m_lhPath.isEmpty()) {
        QFile file(m_lhPath);
        MNELIB::MNESourceEstimate stc;
        if (MNELIB::MNESourceEstimate::read(file, stc)) {
            m_stcLh = stc;
            m_hasLh = true;
            qDebug() << "StcLoadingWorker: Loaded LH with" << stc.data.rows() << "vertices,"
                     << stc.data.cols() << "time points";
        } else {
            emit error(QString("Failed to read LH STC file: %1").arg(m_lhPath));
        }
    }

    emit progress(5, "Loading RH STC file...");

    // Load RH STC file
    if (!m_rhPath.isEmpty()) {
        QFile file(m_rhPath);
        MNELIB::MNESourceEstimate stc;
        if (MNELIB::MNESourceEstimate::read(file, stc)) {
            m_stcRh = stc;
            m_hasRh = true;
            qDebug() << "StcLoadingWorker: Loaded RH with" << stc.data.rows() << "vertices,"
                     << stc.data.cols() << "time points";
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
        emit progress(10, "Computing LH interpolation matrix...");
        
        Eigen::MatrixX3f matVertices = m_lhSurface->verticesAsMatrix();
        QVector<QVector<int>> vecNeighbors = m_lhSurface->computeNeighbors();

        QVector<int> vecSourceVertices;
        vecSourceVertices.reserve(m_stcLh.vertices.size());
        for (int i = 0; i < m_stcLh.vertices.size(); ++i) {
            vecSourceVertices.append(m_stcLh.vertices(i));
        }

        qDebug() << "StcLoadingWorker: LH surface has" << matVertices.rows() << "vertices,"
                 << vecSourceVertices.size() << "sources";

        if (!vecSourceVertices.isEmpty()) {
            // Compute distance table (SCDC - geodesic distance)
            emit progress(15, "Computing LH geodesic distances (SCDC)...");
            QSharedPointer<Eigen::MatrixXd> distTable = DISP3DLIB::GeometryInfo::scdc(
                matVertices,
                vecNeighbors,
                vecSourceVertices,
                m_cancelDist
            );

            if (distTable && distTable->rows() > 0) {
                emit progress(35, "Creating LH interpolation matrix...");
                m_interpMatLh = DISP3DLIB::Interpolation::createInterpolationMat(
                    vecSourceVertices,
                    distTable,
                    DISP3DLIB::Interpolation::cubic,
                    m_cancelDist
                );

                if (m_interpMatLh && m_interpMatLh->rows() > 0) {
                    qDebug() << "StcLoadingWorker: LH interpolation matrix created:"
                             << m_interpMatLh->rows() << "x" << m_interpMatLh->cols();
                }
            }
        }
    }

    // Compute RH interpolation matrix
    if (m_hasRh && m_rhSurface) {
        emit progress(50, "Computing RH interpolation matrix...");

        Eigen::MatrixX3f matVertices = m_rhSurface->verticesAsMatrix();
        QVector<QVector<int>> vecNeighbors = m_rhSurface->computeNeighbors();

        QVector<int> vecSourceVertices;
        vecSourceVertices.reserve(m_stcRh.vertices.size());
        for (int i = 0; i < m_stcRh.vertices.size(); ++i) {
            vecSourceVertices.append(m_stcRh.vertices(i));
        }

        qDebug() << "StcLoadingWorker: RH surface has" << matVertices.rows() << "vertices,"
                 << vecSourceVertices.size() << "sources";

        if (!vecSourceVertices.isEmpty()) {
            // Compute distance table (SCDC - geodesic distance)
            emit progress(55, "Computing RH geodesic distances (SCDC)...");
            QSharedPointer<Eigen::MatrixXd> distTable = DISP3DLIB::GeometryInfo::scdc(
                matVertices,
                vecNeighbors,
                vecSourceVertices,
                m_cancelDist
            );

            if (distTable && distTable->rows() > 0) {
                emit progress(75, "Creating RH interpolation matrix...");
                m_interpMatRh = DISP3DLIB::Interpolation::createInterpolationMat(
                    vecSourceVertices,
                    distTable,
                    DISP3DLIB::Interpolation::cubic,
                    m_cancelDist
                );

                if (m_interpMatRh && m_interpMatRh->rows() > 0) {
                    qDebug() << "StcLoadingWorker: RH interpolation matrix created:"
                             << m_interpMatRh->rows() << "x" << m_interpMatRh->cols();
                }
            }
        }
    }

    emit progress(95, "Finalizing...");
    emit progress(100, "Complete");
    emit finished(true);
}
