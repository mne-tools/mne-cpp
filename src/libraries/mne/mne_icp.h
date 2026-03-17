//=============================================================================================================
/**
 * @file     mne_icp.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.5
 * @date     July, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     ICP declarations.
 *
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
