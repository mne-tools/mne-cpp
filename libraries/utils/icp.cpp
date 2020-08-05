//=============================================================================================================
/**
 * @file     icp.cpp
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
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

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ICP::ICP()
{
}

//=============================================================================================================

bool UTILSLIB::icp(const Eigen::Matrix3f& matSrcPoint,
                   const Eigen::Matrix3f& matDstPoint,
                   Eigen::Matrix4f& matTrans,
                   const int iNumIter,
                   const float fTol)
{
    int iIter = 0;
    MatrixXf matP = matSrcPoint;
    MatrixXf matX = matDstPoint;

}

//=============================================================================================================

bool UTILSLIB::fit_matched(const Matrix3f& matSrcPoint,
                           const Matrix3f& matDstPoint,
                           Eigen::Matrix4f& matTrans,
                           float fScale,
                           const bool bScale,
                           const VectorXf& vecWeitgths)
/**
 * Follow notation of P.J. Besl and N.D. McKay, A Method for
 * Registration of 3-D Shapes, IEEE Trans. Patt. Anal. Machine Intell., 14,
 * 239 - 255, 1992.
 *
 * The code is further adapted from MNE Python function _fit_matched_points(...).
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
    fScale = 1.0;

    // test size of point clouds
    if(matSrcPoint.size() != matDstPoint.size()) {
        qWarning() << "UTILSLIB::ICP::fit_matched: Point clouds does not match.";
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

        Matrix3f matXWeighted = matX;
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
    return true;
}
