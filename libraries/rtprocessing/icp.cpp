//=============================================================================================================
/**
 * @file     icp.cpp
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
 * @brief    ICP class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "icp.h"
#include <iostream>

#include "fiff/fiff_coord_trans.h"
#include "mne/mne_project_to_surface.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

bool RTPROCESSINGLIB::performIcp(const MNEProjectToSurface::SPtr mneSurfacePoints,
                                 const Eigen::MatrixXf& matPointCloud,
                                 FiffCoordTrans& transFromTo,
                                 float& fRMSE,
                                 bool bScale,
                                 int iMaxIter,
                                 float fTol,
                                 const VectorXf& vecWeitgths)
/**
 * Follow notation of P.J. Besl and N.D. McKay, A Method for
 * Registration of 3-D Shapes, IEEE Trans. Patt. Anal. Machine Intell., 14,
 * 239 - 255, 1992.
 */
{
    if(matPointCloud.rows() == 0){
        qWarning() << "[RTPROCESSINGLIB::icp] Passed point cloud is empty.";
        return false;
    }

    // Initialization
    int iNP = matPointCloud.rows();             // The number of points
    float fMSEPrev = 0.0f;                       // The mean square error of previous iteration.
    float fMSE = 0.0f;                           // The mean square error
    float fScale = 1.0f;
    MatrixXf matP0 = matPointCloud;             // Initial Set of points
    MatrixXf matPk = matP0;                     // Transformed Set of points
    MatrixXf matYk(matPk.rows(),matPk.cols());  // Iterative closest points on the surface
    MatrixXf matDiff = matYk;
    VectorXf vecSE(matDiff.rows());
    Matrix4f matTrans;                          // the transformation matrix
    VectorXi vecNearest;                        // Triangle of the new point
    VectorXf vecDist;                           // The Distance between matX and matP

    // Initial transformation - From point cloud To surface
    FiffCoordTrans transICP = transFromTo;
    matPk = transICP.apply_trans(matPk);

    // Icp algorithm:
    for(int iIter = 0; iIter < iMaxIter; ++iIter) {

        // Step a: compute the closest point on the surface; eq 29
        if(!mneSurfacePoints->mne_find_closest_on_surface(matPk, iNP, matYk, vecNearest, vecDist)) {
            qWarning() << "[RTPROCESSINGLIB::icp] mne_find_closest_on_surface was not sucessfull.";
            return false;
        }

        // Step b: compute the registration; eq 30
        if(!fitMatchedPoints(matP0, matYk, matTrans, fScale, bScale, vecWeitgths)) {
            qWarning() << "[RTPROCESSINGLIB::icp] point cloud registration not succesfull";
        }

        // Step c: apply registration
        transICP.trans = matTrans;
        matPk = transICP.apply_trans(matP0);

        // step d: compute mean-square-error and terminate if below fTol
        vecDist = vecDist.cwiseProduct(vecDist);
        fMSE = vecDist.sum() / iNP;
        fRMSE = std::sqrt(fMSE);

        if(std::sqrt(std::fabs(fMSE - fMSEPrev)) < fTol) {
            transFromTo = transICP;
            qInfo() << "[RTPROCESSINGLIB::icp] ICP was succesfull and exceeded after " << iIter +1 << " Iterations with RMSE dist: " << fRMSE * 1000 << " mm.";
            return true;
        }
        fMSEPrev = fMSE;
        qInfo() << "[RTPROCESSINGLIB::icp] ICP iteration " << iIter + 1 << " with RMSE: " << fRMSE * 1000 << " mm.";
    }
    transFromTo = transICP;

    qWarning() << "[RTPROCESSINGLIB::icp] Maximum number of " << iMaxIter << " Iterations exceeded with RMSE: " << fRMSE * 1000 << " mm.";
    return true;
}

//=============================================================================================================

bool RTPROCESSINGLIB::fitMatchedPoints(const MatrixXf& matSrcPoint,
                                       const MatrixXf& matDstPoint,
                                       Eigen::Matrix4f& matTrans,
                                       float fScale,
                                       bool bScale,
                                       const VectorXf& vecWeitgths)
/**
 * Follow notation of P.J. Besl and N.D. McKay, A Method for
 * Registration of 3-D Shapes, IEEE Trans. Patt. Anal. Machine Intell., 14,
 * 239 - 255, 1992.
 *
 * The code is further adapted from MNE Python function _fitMatched_points(...).
 */
{
    // init values
    MatrixXf matP = matSrcPoint;
    MatrixXf matX = matDstPoint;
    VectorXf vecW = vecWeitgths;
    VectorXf vecMuP;                                // column wise mean - center of mass
    VectorXf vecMuX;                                // column wise mean - center of mass
    MatrixXf matDot;
    MatrixXf matSigmaPX;                            // cross-covariance
    MatrixXf matAij;                                // Anti-Symmetric matrix
    Vector3f vecDelta;                              // column vector, elements of matAij
    Matrix4f matQ = Matrix4f::Identity(4,4);
    Matrix3f matScale = Matrix3f::Identity(3,3);    // scaling matrix
    Matrix3f matRot = Matrix3f::Identity(3,3);
    Vector3f vecTrans;
    float fTrace = 0.0;

    // test size of point clouds
    if(matSrcPoint.size() != matDstPoint.size()) {
        qWarning() << "[RTPROCESSINGLIB::fitMatched] Point clouds do not match.";
        return false;
    }

    // get center of mass
    if(vecWeitgths.isZero()) {
        vecMuP = matP.colwise().mean(); // eq 23
        vecMuX = matX.colwise().mean();
        matDot = matP.transpose() * matX;
        matDot = matDot / matP.rows();
    } else {
        vecW = vecWeitgths / vecWeitgths.sum();
        vecMuP = vecW.transpose() * matP;
        vecMuX = vecW.transpose() * matX;

        MatrixXf matXWeighted = matX;
        for(int i = 0; i < (vecW.size()); ++i) {
            matXWeighted.row(i) = matXWeighted.row(i) * vecW(i);
        }
        matDot = matP.transpose() * (matXWeighted);
    }

    // get cross-covariance
    matSigmaPX = matDot - (vecMuP * vecMuX.transpose());  // eq 24
    matAij = matSigmaPX - matSigmaPX.transpose();
    vecDelta(0) = matAij(1,2); vecDelta(1) = matAij(2,0); vecDelta(2) = matAij(0,1);
    fTrace = matSigmaPX.trace();
    matQ(0,0) = fTrace; // eq 25
    matQ.block(0,1,1,3) = vecDelta.transpose();
    matQ.block(1,0,3,1) = vecDelta;
    matQ.block(1,1,3,3) = matSigmaPX + matSigmaPX.transpose() - fTrace * MatrixXf::Identity(3,3);

    // unit eigenvector coresponding to maximum eigenvalue of matQ is selected as optimal rotation quaterions q0,q1,q2,q3
    SelfAdjointEigenSolver<MatrixXf> es(matQ);
    Vector4f vecEigVec = es.eigenvectors().col(matQ.cols()-1);  // only take last Eigen-Vector since this corresponds to the maximum Eigenvalue

    // quatRot(w,x,y,z)
    Quaternionf quatRot(vecEigVec(0),vecEigVec(1),vecEigVec(2),vecEigVec(3));
    quatRot.normalize();
    matRot = quatRot.matrix();

    // get scaling factor and matrix
    if(bScale) {
        MatrixXf matDevX = matX.rowwise() - vecMuX.transpose();
        MatrixXf matDevP = matP.rowwise() - vecMuP.transpose();
        matDevX = matDevX.cwiseProduct(matDevX);
        matDevP = matDevP.cwiseProduct(matDevP);

        if(!vecWeitgths.isZero()) {
            for(int i = 0; i < (vecW.size()); ++i) {
                matDevX.row(i) = matDevX.row(i) * vecW(i);
                matDevP.row(i) = matDevP.row(i) * vecW(i);
            }
        }
        // get scaling factor and set scaling matrix
        fScale = std::sqrt(matDevX.sum() / matDevP.sum());
        matScale *= fScale;
    }

    // get translation and Rotation
    vecTrans = vecMuX - fScale * matRot * vecMuP;
    matRot *= matScale;

    matTrans.block<3,3>(0,0) = matRot;
    matTrans.block<3,1>(0,3) = vecTrans;
    matTrans(3,3) = 1.0f;
    matTrans.block<1,3>(3,0) = MatrixXf::Zero(1,3);
    return true;
}

//=========================================================================================================

bool RTPROCESSINGLIB::discard3DPointOutliers(const QSharedPointer<MNELIB::MNEProjectToSurface> mneSurfacePoints,
                                             const MatrixXf& matPointCloud,
                                             const FiffCoordTrans& transFromTo,
                                             VectorXi& vecTake,
                                             MatrixXf& matTakePoint,
                                             float fMaxDist)
{
    // Initialization
    int iNP = matPointCloud.rows();               // The number of points
    MatrixXf matP = matPointCloud;                // Initial Set of points
    MatrixXf matYk(matPointCloud.rows(),matPointCloud.cols());  // Iterative losest points on the surface
    VectorXi vecNearest;                        // Triangle of the new point
    VectorXf vecDist;                           // The Distance between matX and matP

    // Initial transformation - From point cloud To surface
    matP = transFromTo.apply_trans(matP);

    int iDiscarded = 0;

    // discard outliers if necessary
    if(fMaxDist > 0.0) {
        if(!mneSurfacePoints->mne_find_closest_on_surface(matP, iNP, matYk, vecNearest, vecDist)) {
            qWarning() << "[RTPROCESSINGLIB::icp] mne_find_closest_on_surface was not sucessfull.";
            return false;
        }

        for(int i = 0; i < vecDist.size(); ++i) {
            if(std::fabs(vecDist(i)) < fMaxDist) {
                vecTake.conservativeResize(vecTake.size()+1);
                vecTake(vecTake.size()-1) = i;
                matTakePoint.conservativeResize(matTakePoint.rows()+1,3);
                matTakePoint.row(matTakePoint.rows()-1) = matPointCloud.row(i);
            } else {
                iDiscarded++;
            }
        }
    }
    qInfo() << "[RTPROCESSINGLIB::discardOutliers] " << iDiscarded << "digitizers discarded.";
    return true;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================
