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

#include "../../../../brain/interpolation.h"
#include "../../../../brain/geometryinfo.h"
#include <mne/base/log.h>
#include <mne/fs/fs_label.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

using namespace mne::viz;
using namespace mne::viz::qt;

//=============================================================================================================
// MEMBER METHODS
//=============================================================================================================

RtSourceInterpolationMatWorker::RtSourceInterpolationMatWorker(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationFunction(const std::string& sInterpolationFunction)
{
    std::scoped_lock locker(m_mutex);
    m_sInterpolationFunction = sInterpolationFunction;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setCancelDistance(double dCancelDist)
{
    std::scoped_lock locker(m_mutex);
    m_dCancelDist = dCancelDist;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfoLeft(
    const Eigen::MatrixX3f& matVertices,
    const std::vector<Eigen::VectorXi>& vecNeighborVertices,
    const Eigen::VectorXi& vecSourceVertices)
{
    std::scoped_lock locker(m_mutex);
    m_matVerticesLh = matVertices;
    m_vecNeighborsLh = vecNeighborVertices;
    m_vecSourceVerticesLh = vecSourceVertices;
    m_hasLh = (!matVertices.isZero(0) && matVertices.rows() > 0 && vecSourceVertices.size() > 0);
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfoRight(
    const Eigen::MatrixX3f& matVertices,
    const std::vector<Eigen::VectorXi>& vecNeighborVertices,
    const Eigen::VectorXi& vecSourceVertices)
{
    std::scoped_lock locker(m_mutex);
    m_matVerticesRh = matVertices;
    m_vecNeighborsRh = vecNeighborVertices;
    m_vecSourceVerticesRh = vecSourceVertices;
    m_hasRh = (!matVertices.isZero(0) && matVertices.rows() > 0 && vecSourceVertices.size() > 0);
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setVisualizationType(int iVisType)
{
    std::scoped_lock locker(m_mutex);
    m_iVisualizationType = iVisType;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setAnnotationInfoLeft(const Eigen::VectorXi& vecLabelIds,
                                                           const std::vector<mne::FsLabel>& lLabels,
                                                           const Eigen::VectorXi& vecVertNo)
{
    if (vecLabelIds.rows() == 0 || lLabels.empty()) {
        mne::Log::debug("RtSourceInterpolationMatWorker::setAnnotationInfoLeft - FsAnnotation data is empty.");
        return;
    }

    std::scoped_lock locker(m_mutex);
    m_lLabelsLh = lLabels;
    m_nAnnotationVerticesLh = static_cast<int>(vecLabelIds.rows());
    m_mapLabelIdSourcesLh.clear();
    m_vertNosLh.clear();

    for (std::int32_t i = 0; i < vecVertNo.rows(); ++i) {
        m_mapLabelIdSourcesLh.insert_or_assign(vecVertNo(i), vecLabelIds(vecVertNo(i)));
        m_vertNosLh.push_back(vecVertNo(i));
    }

    m_bAnnotationLhInit = true;
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setAnnotationInfoRight(const Eigen::VectorXi& vecLabelIds,
                                                            const std::vector<mne::FsLabel>& lLabels,
                                                            const Eigen::VectorXi& vecVertNo)
{
    if (vecLabelIds.rows() == 0 || lLabels.empty()) {
        mne::Log::debug("RtSourceInterpolationMatWorker::setAnnotationInfoRight - FsAnnotation data is empty.");
        return;
    }

    std::scoped_lock locker(m_mutex);
    m_lLabelsRh = lLabels;
    m_nAnnotationVerticesRh = static_cast<int>(vecLabelIds.rows());
    m_mapLabelIdSourcesRh.clear();
    m_vertNosRh.clear();

    for (std::int32_t i = 0; i < vecVertNo.rows(); ++i) {
        m_mapLabelIdSourcesRh.insert_or_assign(vecVertNo(i), vecLabelIds(vecVertNo(i)));
        m_vertNosRh.push_back(vecVertNo(i));
    }

    m_bAnnotationRhInit = true;
}

//=============================================================================================================

double (*RtSourceInterpolationMatWorker::resolveInterpolationFunction(const std::string& name))(double)
{
    if (name == "linear")
        return Interpolation::linear;
    if (name == "gaussian")
        return Interpolation::gaussian;
    if (name == "square")
        return Interpolation::square;
    if (name == "cubic")
        return Interpolation::cubic;

    // Default to cubic
    return Interpolation::cubic;
}

//=============================================================================================================

std::shared_ptr<Eigen::SparseMatrix<float>> RtSourceInterpolationMatWorker::computeHemi(
    const Eigen::MatrixX3f& matVertices,
    const std::vector<Eigen::VectorXi>& vecNeighborVertices,
    Eigen::VectorXi vecSourceVertices,
    double dCancelDist,
    double (*interpFunc)(double))
{
    if (matVertices.rows() == 0 || vecSourceVertices.size() == 0) {
        return std::shared_ptr<Eigen::SparseMatrix<float>>();
    }

    // Compute surface-constrained distances (SCDC / geodesic)
    std::shared_ptr<Eigen::MatrixXd> distTable = GeometryInfo::scdc(
        matVertices,
        vecNeighborVertices,
        vecSourceVertices,
        dCancelDist);

    if (!distTable || distTable->rows() == 0) {
        mne::Log::warning("RtSourceInterpolationMatWorker: SCDC computation failed.");
        return std::shared_ptr<Eigen::SparseMatrix<float>>();
    }

    // Create interpolation matrix
    auto interpMat = Interpolation::createInterpolationMat(
        vecSourceVertices,
        distTable,
        interpFunc,
        dCancelDist);

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
    std::string interpFuncName;
    int visType;

    // FsAnnotation data snapshots
    std::vector<mne::FsLabel> labelsLh, labelsRh;
    std::map<std::int32_t, std::int32_t> mapLabelIdSrcLh, mapLabelIdSrcRh;
    std::vector<int> vertNosLh, vertNosRh;
    bool annotLhInit, annotRhInit;
    int nAnnotVertsLh, nAnnotVertsRh;

    {
        std::scoped_lock locker(m_mutex);
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
        nAnnotVertsLh = m_nAnnotationVerticesLh;
        nAnnotVertsRh = m_nAnnotationVerticesRh;
    }

    // ── FsAnnotation-based mode ──────────────────────────────────────────
    if (visType == AnnotationBased) {
        mne::Log::debug("RtSourceInterpolationMatWorker: Computing annotation matrices...");

        if (annotLhInit) {
            auto mat = computeAnnotationOperator(labelsLh, mapLabelIdSrcLh, vertNosLh, nAnnotVertsLh);
            if (mat && mat->rows() > 0) {
                mne::Log::debug("RtSourceInterpolationMatWorker: LH annotation matrix: {} x {}", mat->rows(), mat->cols());
                emit newInterpolationMatrixLeftAvailable(mat);
            }
        }

        if (annotRhInit) {
            auto mat = computeAnnotationOperator(labelsRh, mapLabelIdSrcRh, vertNosRh, nAnnotVertsRh);
            if (mat && mat->rows() > 0) {
                mne::Log::debug("RtSourceInterpolationMatWorker: RH annotation matrix: {} x {}", mat->rows(), mat->cols());
                emit newInterpolationMatrixRightAvailable(mat);
            }
        }

        mne::Log::debug("RtSourceInterpolationMatWorker: FsAnnotation matrix computation complete.");
        return;
    }

    // ── Interpolation-based mode (default) ─────────────────────────────
    if (!hasLh && !hasRh) {
        mne::Log::debug("RtSourceInterpolationMatWorker: No hemisphere data set, skipping.");
        return;
    }

    auto interpFunc = resolveInterpolationFunction(interpFuncName);

    mne::Log::debug("RtSourceInterpolationMatWorker: Computing interpolation matrices (cancelDist={}, func={})...", cancelDist, interpFuncName);

    // ── LH ─────────────────────────────────────────────────────────────
    if (hasLh) {
        mne::Log::debug("RtSourceInterpolationMatWorker: Computing LH matrix ({} verts, {} sources)...", vertsLh.rows(), srcVertsLh.size());

        auto mat = computeHemi(vertsLh, neighborsLh, srcVertsLh, cancelDist, interpFunc);

        if (mat && mat->rows() > 0) {
            mne::Log::debug("RtSourceInterpolationMatWorker: LH matrix computed: {} x {}", mat->rows(), mat->cols());
            emit newInterpolationMatrixLeftAvailable(mat);
        } else {
            mne::Log::warning("RtSourceInterpolationMatWorker: LH interpolation matrix computation failed.");
        }
    }

    // ── RH ─────────────────────────────────────────────────────────────
    if (hasRh) {
        mne::Log::debug("RtSourceInterpolationMatWorker: Computing RH matrix ({} verts, {} sources)...", vertsRh.rows(), srcVertsRh.size());

        auto mat = computeHemi(vertsRh, neighborsRh, srcVertsRh, cancelDist, interpFunc);

        if (mat && mat->rows() > 0) {
            mne::Log::debug("RtSourceInterpolationMatWorker: RH matrix computed: {} x {}", mat->rows(), mat->cols());
            emit newInterpolationMatrixRightAvailable(mat);
        } else {
            mne::Log::warning("RtSourceInterpolationMatWorker: RH interpolation matrix computation failed.");
        }
    }

    mne::Log::debug("RtSourceInterpolationMatWorker: Interpolation matrix computation complete.");
}

//=============================================================================================================

std::shared_ptr<Eigen::SparseMatrix<float>> RtSourceInterpolationMatWorker::computeAnnotationOperator(
    const std::vector<mne::FsLabel>& lLabels,
    const std::map<std::int32_t, std::int32_t>& mapLabelIdSrc,
    const std::vector<int>& vertNos,
    int nVertices)
{
    if (lLabels.empty() || vertNos.empty() || nVertices <= 0) {
        return std::shared_ptr<Eigen::SparseMatrix<float>>();
    }

    // One row per surface vertex: labels need not cover the surface, and their vertex indices are surface indices
    auto mat = std::make_shared<Eigen::SparseMatrix<float>>(nVertices, static_cast<Eigen::Index>(vertNos.size()));

    // For each label: assign uniform weight to all its vertices from sources in that label
    for (int i = 0; i < std::ssize(lLabels); ++i) {
        const mne::FsLabel& label = lLabels.at(i);
        std::vector<std::int32_t> listSourcesVertNoLabel;
        for (const auto& [vertNo, labelId] : mapLabelIdSrc) {
            if (labelId == label.label_id) {
                listSourcesVertNoLabel.push_back(vertNo);
            }
        }

        for (int j = 0; j < label.vertices.rows(); ++j) {
            for (const std::int32_t sourceVertNo : listSourcesVertNoLabel) {
                const auto col = std::ranges::find(vertNos, sourceVertNo);
                const int colIdx = col == vertNos.end() ? -1 : static_cast<int>(col - vertNos.begin());
                if (colIdx >= 0 && label.vertices(j) >= 0 && label.vertices(j) < nVertices) {
                    mat->coeffRef(label.vertices(j), colIdx) = 1.0f / static_cast<float>(listSourcesVertNoLabel.size());
                }
            }
        }
    }

    return mat;
}
