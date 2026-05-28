//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_icp.h
 * @since 2026
 * @date  March 2026
 * @brief Iterative Closest Point alignment between MEG digitiser points and an MRI head surface.
 *
 * @ref MNELIB::MNEIcp wraps the classical point-to-surface ICP used by
 * mne-cpp's coregistration tools to refine an initial @c head <-> @c MRI
 * transformation. It projects each digitiser point onto the nearest
 * triangle of an @ref MNEBemSurface (via @ref MNEProjectToSurface),
 * solves a rigid-body least squares with SVD and iterates until
 * convergence.
 */

#ifndef MNE_ICP_H
#define MNE_ICP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB{
    class FiffCoordTrans;
}

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB {

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEProjectToSurface;

const Eigen::VectorXf vecDefaultWeights;

//=========================================================================================================
/**
 * The ICP algorithm to register a point cloud with a surface.
 *
 * @param[in] mneSurfacePoints    The MNEProjectToSurface object that contains the surface triangles etc. (To).
 * @param[in] matPointCloud       The point cloud to be registered (From).
 * @param[in, out] transFromTo    The forward transformation matrix. It can contain an initial transformation (e.g. from fiducial alignment).
 * @param[in, out] fRMSE         The resulting Root-Mean-Square-Error in m.
 * @param[in] bScale              Whether to apply scaling or not. Should be false for matching data sets, defaults to false.
 * @param[in] iMaxIter            The maximum number of iterations for the ICP algorithm, defaults to 20.
 * @param[in] fTol                The convergence tolerance, defaults to 0.001.
 * @param[in] vecWeights          The weights to apply, defaults to zeros.
 *
 * @return Whether the registration was successful.
 */

MNESHARED_EXPORT bool performIcp(const QSharedPointer<MNELIB::MNEProjectToSurface> mneSurfacePoints,
                                 const Eigen::MatrixXf& matPointCloud,
                                 FIFFLIB::FiffCoordTrans& transFromTo,
                                 float& fRMSE,
                                 bool bScale = false,
                                 int iMaxIter = 20,
                                 float fTol = 0.001,
                                 const Eigen::VectorXf& vecWeights = vecDefaultWeights);

//=========================================================================================================

/**
 * Corresponding point set registration using quaternions.
 *
 * @param[in] matSrcPoint         The source point set.
 * @param[in] matDstPoint         The destination point set.
 * @param[in, out] matTrans       The forward transformation matrix.
 * @param[in, out] fScale         The scaling parameter, defaults to 1.0.
 * @param[in] bScale              Whether to apply scaling or not. Should be false for matching data sets, defaults to false.
 * @param[in] vecWeights          The weights to apply, defaults to zeros.
 *
 * @return Whether the matching was successful.
 */

MNESHARED_EXPORT bool fitMatchedPoints(const Eigen::MatrixXf& matSrcPoint,
                                       const Eigen::MatrixXf& matDstPoint,
                                       Eigen::Matrix4f& matTrans,
                                       float fScale = 1.0,
                                       bool bScale=false,
                                       const Eigen::VectorXf& vecWeights = vecDefaultWeights);

//=========================================================================================================

/**
 * Discard outliers compared to a given 3D surface.
 *
 * @param[in] mneSurfacePoints    The MNEProjectToSurface object that contains the surface triangles etc. (To).
 * @param[in] matPointCloud       The point cloud to be registered (From).
 * @param[in, out] transFromTo    The forward transformation matrix.
 * @param[in] vecTake             The index of taken digitizers.
 * @param[in] matTakePoint        The digitizer points to take.
 * @param[in] fMaxDist            The maximum distance to the surface in mm, defaults to 0 mm.
 *
 * @return Whether the discarding was successful.
 */

MNESHARED_EXPORT bool discard3DPointOutliers(const QSharedPointer<MNELIB::MNEProjectToSurface> mneSurfacePoints,
                                             const Eigen::MatrixXf& matPointCloud,
                                             const FIFFLIB::FiffCoordTrans& transFromTo,
                                             Eigen::VectorXi& vecTake,
                                             Eigen::MatrixXf& matTakePoint,
                                             float fMaxDist = 0.0);

} // namespace MNELIB

#endif // MNE_ICP_H
