//=============================================================================================================
/**
 * @file     inv_mxne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    InvMxne class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_mxne.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvMxneResult InvMxne::compute(
    const MatrixXd& matGain,
    const MatrixXd& matData,
    double alpha,
    int nIterations,
    double tolerance)
{
    const int nChannels = static_cast<int>(matGain.rows());
    const int nSources = static_cast<int>(matGain.cols());
    const int nTimes = static_cast<int>(matData.cols());

    // Precompute G^T * G and G^T * M
    MatrixXd matGtG = matGain.transpose() * matGain;
    MatrixXd matGtM = matGain.transpose() * matData;

    // Initialize weights to 1
    VectorXd vecWeights = VectorXd::Ones(nSources);
    VectorXd vecWeightsOld = vecWeights;

    // Active set: all sources initially active
    std::vector<int> activeIdx(nSources);
    std::iota(activeIdx.begin(), activeIdx.end(), 0);

    // Full source solution
    MatrixXd matX = MatrixXd::Zero(nSources, nTimes);

    int actualIterations = 0;

    for (int iter = 0; iter < nIterations; ++iter) {
        actualIterations = iter + 1;

        const int nActive = static_cast<int>(activeIdx.size());
        if (nActive == 0)
            break;

        // Extract active columns of G^T*G and G^T*M
        MatrixXd matGtG_active(nActive, nActive);
        MatrixXd matGtM_active(nActive, nTimes);

        for (int i = 0; i < nActive; ++i) {
            matGtM_active.row(i) = matGtM.row(activeIdx[i]);
            for (int j = 0; j < nActive; ++j) {
                matGtG_active(i,j) = matGtG(activeIdx[i], activeIdx[j]);
            }
        }

        // Build diagonal weight matrix W = diag(1/w_i^2)
        VectorXd vecWdiag(nActive);
        for (int i = 0; i < nActive; ++i) {
            double w = vecWeights(activeIdx[i]);
            vecWdiag(i) = 1.0 / (w * w);
        }

        // Solve (G^T*G + alpha*W) * X_active = G^T*M
        MatrixXd matLhs = matGtG_active;
        matLhs.diagonal() += alpha * vecWdiag;

        MatrixXd matX_active = matLhs.ldlt().solve(matGtM_active);

        // Write back to full solution
        matX.setZero();
        for (int i = 0; i < nActive; ++i) {
            matX.row(activeIdx[i]) = matX_active.row(i);
        }

        // Update weights: w_i = max(||X_i||_2, 1e-10)
        vecWeightsOld = vecWeights;
        for (int i = 0; i < nSources; ++i) {
            vecWeights(i) = std::max(matX.row(i).norm(), 1e-10);
        }

        // Active set pruning: keep sources with w_i >= 1e-8
        std::vector<int> newActive;
        newActive.reserve(nActive);
        for (int i = 0; i < nSources; ++i) {
            if (vecWeights(i) >= 1e-8) {
                newActive.push_back(i);
            }
        }
        activeIdx = newActive;

        // Check convergence
        double maxChange = 0.0;
        for (int idx : activeIdx) {
            maxChange = std::max(maxChange, std::abs(vecWeights(idx) - vecWeightsOld(idx)));
        }
        if (maxChange < tolerance)
            break;
    }

    // Build result
    InvMxneResult result;
    result.nIterations = actualIterations;

    // Collect active vertices and build sparse output
    QVector<int> finalActive;
    for (int i = 0; i < nSources; ++i) {
        if (matX.row(i).norm() >= 1e-8) {
            finalActive.append(i);
        }
    }
    result.activeVertices = finalActive;

    // Build source estimate with active rows only
    const int nActiveFinal = finalActive.size();
    MatrixXd matActiveSol(nActiveFinal, nTimes);
    VectorXi vecActiveVerts(nActiveFinal);
    for (int i = 0; i < nActiveFinal; ++i) {
        matActiveSol.row(i) = matX.row(finalActive[i]);
        vecActiveVerts(i) = finalActive[i];
    }

    result.stc = InvSourceEstimate(matActiveSol, vecActiveVerts, 0.0f, 1.0f);
    result.stc.method = InvEstimateMethod::MixedNorm;

    // Compute residual norm ||M - G*X||_F
    MatrixXd matResidual = matData - matGain * matX;
    result.residualNorm = matResidual.norm();

    return result;
}
