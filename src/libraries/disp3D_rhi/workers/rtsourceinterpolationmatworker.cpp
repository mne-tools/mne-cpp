//=============================================================================================================
/**
 * @file     rtsourceinterpolationmatworker.cpp
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
 * @brief    RtSourceInterpolationMatWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourceinterpolationmatworker.h"
#include "helpers/interpolation.h"
#include "helpers/geometryinfo.h"

#include <QDebug>

using namespace DISP3DRHILIB;

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

RtSourceInterpolationMatWorker::RtSourceInterpolationMatWorker(QObject *parent)
    : QObject(parent)
{
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationFunction(const QString &sInterpolationFunction)
{
    QMutexLocker locker(&m_mutex);
    m_sInterpolationFunction = sInterpolationFunction;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setCancelDistance(double dCancelDist)
{
    QMutexLocker locker(&m_mutex);
    m_dCancelDist = dCancelDist;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfoLeft(
    const Eigen::MatrixX3f &matVertices,
    const QVector<QVector<int>> &vecNeighborVertices,
    const QVector<int> &vecSourceVertices)
{
    QMutexLocker locker(&m_mutex);
    m_matVerticesLh = matVertices;
    m_vecNeighborsLh = vecNeighborVertices;
    m_vecSourceVerticesLh = vecSourceVertices;
    m_hasLh = (!matVertices.isZero(0) && matVertices.rows() > 0 && !vecSourceVertices.isEmpty());
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfoRight(
    const Eigen::MatrixX3f &matVertices,
    const QVector<QVector<int>> &vecNeighborVertices,
    const QVector<int> &vecSourceVertices)
{
    QMutexLocker locker(&m_mutex);
    m_matVerticesRh = matVertices;
    m_vecNeighborsRh = vecNeighborVertices;
    m_vecSourceVerticesRh = vecSourceVertices;
    m_hasRh = (!matVertices.isZero(0) && matVertices.rows() > 0 && !vecSourceVertices.isEmpty());
}

//=============================================================================================================

double (*RtSourceInterpolationMatWorker::resolveInterpolationFunction(const QString &name))(double)
{
    if (name == QStringLiteral("linear"))   return Interpolation::linear;
    if (name == QStringLiteral("gaussian")) return Interpolation::gaussian;
    if (name == QStringLiteral("square"))   return Interpolation::square;
    if (name == QStringLiteral("cubic"))    return Interpolation::cubic;

    // Default to cubic
    return Interpolation::cubic;
}

//=============================================================================================================

QSharedPointer<Eigen::SparseMatrix<float>> RtSourceInterpolationMatWorker::computeHemi(
    const Eigen::MatrixX3f &matVertices,
    const QVector<QVector<int>> &vecNeighborVertices,
    QVector<int> vecSourceVertices,
    double dCancelDist,
    double (*interpFunc)(double))
{
    if (matVertices.rows() == 0 || vecSourceVertices.isEmpty()) {
        return QSharedPointer<Eigen::SparseMatrix<float>>();
    }

    // Compute surface-constrained distances (SCDC / geodesic)
    QSharedPointer<Eigen::MatrixXd> distTable = GeometryInfo::scdc(
        matVertices,
        vecNeighborVertices,
        vecSourceVertices,
        dCancelDist
    );

    if (!distTable || distTable->rows() == 0) {
        qWarning() << "RtSourceInterpolationMatWorker: SCDC computation failed.";
        return QSharedPointer<Eigen::SparseMatrix<float>>();
    }

    // Create interpolation matrix
    auto interpMat = Interpolation::createInterpolationMat(
        vecSourceVertices,
        distTable,
        interpFunc,
        dCancelDist
    );

    return interpMat;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::computeInterpolationMatrix()
{
    // ── Snapshot parameters under lock ─────────────────────────────────
    Eigen::MatrixX3f vertsLh, vertsRh;
    QVector<QVector<int>> neighborsLh, neighborsRh;
    QVector<int> srcVertsLh, srcVertsRh;
    bool hasLh, hasRh;
    double cancelDist;
    QString interpFuncName;

    {
        QMutexLocker locker(&m_mutex);
        vertsLh = m_matVerticesLh;
        vertsRh = m_matVerticesRh;
        neighborsLh = m_vecNeighborsLh;
        neighborsRh = m_vecNeighborsRh;
        srcVertsLh = m_vecSourceVerticesLh;
        srcVertsRh = m_vecSourceVerticesRh;
        hasLh = m_hasLh;
        hasRh = m_hasRh;
        cancelDist = m_dCancelDist;
        interpFuncName = m_sInterpolationFunction;
    }

    if (!hasLh && !hasRh) {
        qDebug() << "RtSourceInterpolationMatWorker: No hemisphere data set, skipping.";
        return;
    }

    auto interpFunc = resolveInterpolationFunction(interpFuncName);

    qDebug() << "RtSourceInterpolationMatWorker: Computing interpolation matrices"
             << "(cancelDist=" << cancelDist << ", func=" << interpFuncName << ")...";

    // ── LH ─────────────────────────────────────────────────────────────
    if (hasLh) {
        qDebug() << "RtSourceInterpolationMatWorker: Computing LH matrix ("
                 << vertsLh.rows() << "verts," << srcVertsLh.size() << "sources)...";

        auto mat = computeHemi(vertsLh, neighborsLh, srcVertsLh, cancelDist, interpFunc);

        if (mat && mat->rows() > 0) {
            qDebug() << "RtSourceInterpolationMatWorker: LH matrix computed:"
                     << mat->rows() << "x" << mat->cols();
            emit newInterpolationMatrixLeftAvailable(mat);
        } else {
            qWarning() << "RtSourceInterpolationMatWorker: LH interpolation matrix computation failed.";
        }
    }

    // ── RH ─────────────────────────────────────────────────────────────
    if (hasRh) {
        qDebug() << "RtSourceInterpolationMatWorker: Computing RH matrix ("
                 << vertsRh.rows() << "verts," << srcVertsRh.size() << "sources)...";

        auto mat = computeHemi(vertsRh, neighborsRh, srcVertsRh, cancelDist, interpFunc);

        if (mat && mat->rows() > 0) {
            qDebug() << "RtSourceInterpolationMatWorker: RH matrix computed:"
                     << mat->rows() << "x" << mat->cols();
            emit newInterpolationMatrixRightAvailable(mat);
        } else {
            qWarning() << "RtSourceInterpolationMatWorker: RH interpolation matrix computation failed.";
        }
    }

    qDebug() << "RtSourceInterpolationMatWorker: Interpolation matrix computation complete.";
}
