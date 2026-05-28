//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     rtsourceinterpolationmatworker.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Off-thread SCDC + interpolation-matrix build for one hemisphere with interpolation- or annotation-based modes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourceinterpolationmatworker.h"
#include "helpers/interpolation.h"
#include "helpers/geometryinfo.h"

#include <fs/fs_label.h>

#include <QDebug>

using namespace DISP3DLIB;

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
    const std::vector<Eigen::VectorXi> &vecNeighborVertices,
    const Eigen::VectorXi &vecSourceVertices)
{
    QMutexLocker locker(&m_mutex);
    m_matVerticesLh = matVertices;
    m_vecNeighborsLh = vecNeighborVertices;
    m_vecSourceVerticesLh = vecSourceVertices;
    m_hasLh = (!matVertices.isZero(0) && matVertices.rows() > 0 && vecSourceVertices.size() > 0);
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfoRight(
    const Eigen::MatrixX3f &matVertices,
    const std::vector<Eigen::VectorXi> &vecNeighborVertices,
    const Eigen::VectorXi &vecSourceVertices)
{
    QMutexLocker locker(&m_mutex);
    m_matVerticesRh = matVertices;
    m_vecNeighborsRh = vecNeighborVertices;
    m_vecSourceVerticesRh = vecSourceVertices;
    m_hasRh = (!matVertices.isZero(0) && matVertices.rows() > 0 && vecSourceVertices.size() > 0);
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setVisualizationType(int iVisType)
{
    QMutexLocker locker(&m_mutex);
    m_iVisualizationType = iVisType;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setAnnotationInfoLeft(const Eigen::VectorXi &vecLabelIds,
                                                            const QList<FSLIB::FsLabel> &lLabels,
                                                            const Eigen::VectorXi &vecVertNo)
{
    if (vecLabelIds.rows() == 0 || lLabels.isEmpty()) {
        qDebug() << "RtSourceInterpolationMatWorker::setAnnotationInfoLeft - FsAnnotation data is empty.";
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_lLabelsLh = lLabels;
    m_mapLabelIdSourcesLh.clear();
    m_vertNosLh.clear();

    for (qint32 i = 0; i < vecVertNo.rows(); ++i) {
        m_mapLabelIdSourcesLh.insert(vecVertNo(i), vecLabelIds(vecVertNo(i)));
        m_vertNosLh.append(vecVertNo(i));
    }

    m_bAnnotationLhInit = true;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setAnnotationInfoRight(const Eigen::VectorXi &vecLabelIds,
                                                             const QList<FSLIB::FsLabel> &lLabels,
                                                             const Eigen::VectorXi &vecVertNo)
{
    if (vecLabelIds.rows() == 0 || lLabels.isEmpty()) {
        qDebug() << "RtSourceInterpolationMatWorker::setAnnotationInfoRight - FsAnnotation data is empty.";
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_lLabelsRh = lLabels;
    m_mapLabelIdSourcesRh.clear();
    m_vertNosRh.clear();

    for (qint32 i = 0; i < vecVertNo.rows(); ++i) {
        m_mapLabelIdSourcesRh.insert(vecVertNo(i), vecLabelIds(vecVertNo(i)));
        m_vertNosRh.append(vecVertNo(i));
    }

    m_bAnnotationRhInit = true;
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
    const std::vector<Eigen::VectorXi> &vecNeighborVertices,
    Eigen::VectorXi vecSourceVertices,
    double dCancelDist,
    double (*interpFunc)(double))
{
    if (matVertices.rows() == 0 || vecSourceVertices.size() == 0) {
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
    std::vector<Eigen::VectorXi> neighborsLh, neighborsRh;
    Eigen::VectorXi srcVertsLh, srcVertsRh;
    bool hasLh, hasRh;
    double cancelDist;
    QString interpFuncName;
    int visType;

    // FsAnnotation data snapshots
    QList<FSLIB::FsLabel> labelsLh, labelsRh;
    QMap<qint32, qint32> mapLabelIdSrcLh, mapLabelIdSrcRh;
    QList<int> vertNosLh, vertNosRh;
    bool annotLhInit, annotRhInit;

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
        visType = m_iVisualizationType;

        labelsLh = m_lLabelsLh;
        labelsRh = m_lLabelsRh;
        mapLabelIdSrcLh = m_mapLabelIdSourcesLh;
        mapLabelIdSrcRh = m_mapLabelIdSourcesRh;
        vertNosLh = m_vertNosLh;
        vertNosRh = m_vertNosRh;
        annotLhInit = m_bAnnotationLhInit;
        annotRhInit = m_bAnnotationRhInit;
    }

    // ── FsAnnotation-based mode ──────────────────────────────────────────
    if (visType == AnnotationBased) {
        qDebug() << "RtSourceInterpolationMatWorker: Computing annotation matrices...";

        if (annotLhInit) {
            auto mat = computeAnnotationOperator(labelsLh, mapLabelIdSrcLh, vertNosLh);
            if (mat && mat->rows() > 0) {
                qDebug() << "RtSourceInterpolationMatWorker: LH annotation matrix:"
                         << mat->rows() << "x" << mat->cols();
                emit newInterpolationMatrixLeftAvailable(mat);
            }
        }

        if (annotRhInit) {
            auto mat = computeAnnotationOperator(labelsRh, mapLabelIdSrcRh, vertNosRh);
            if (mat && mat->rows() > 0) {
                qDebug() << "RtSourceInterpolationMatWorker: RH annotation matrix:"
                         << mat->rows() << "x" << mat->cols();
                emit newInterpolationMatrixRightAvailable(mat);
            }
        }

        qDebug() << "RtSourceInterpolationMatWorker: FsAnnotation matrix computation complete.";
        return;
    }

    // ── Interpolation-based mode (default) ─────────────────────────────
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

//=============================================================================================================

QSharedPointer<Eigen::SparseMatrix<float>> RtSourceInterpolationMatWorker::computeAnnotationOperator(
    const QList<FSLIB::FsLabel> &lLabels,
    const QMap<qint32, qint32> &mapLabelIdSrc,
    const QList<int> &vertNos)
{
    if (lLabels.isEmpty() || vertNos.isEmpty()) {
        return QSharedPointer<Eigen::SparseMatrix<float>>();
    }

    // Count total vertices across all labels
    int iNumVert = 0;
    for (int i = 0; i < lLabels.size(); ++i) {
        iNumVert += lLabels.at(i).vertices.rows();
    }

    auto mat = QSharedPointer<Eigen::SparseMatrix<float>>(
        new Eigen::SparseMatrix<float>(iNumVert, vertNos.size()));

    // For each label: assign uniform weight to all its vertices from sources in that label
    for (int i = 0; i < lLabels.size(); ++i) {
        const FSLIB::FsLabel &label = lLabels.at(i);
        QList<qint32> listSourcesVertNoLabel = mapLabelIdSrc.keys(label.label_id);

        for (int j = 0; j < label.vertices.rows(); ++j) {
            for (int k = 0; k < listSourcesVertNoLabel.size(); ++k) {
                int colIdx = vertNos.indexOf(listSourcesVertNoLabel.at(k));
                if (colIdx >= 0 && label.vertices(j) < iNumVert) {
                    mat->coeffRef(label.vertices(j), colIdx) = 1.0f / listSourcesVertNoLabel.size();
                }
            }
        }
    }

    return mat;
}
