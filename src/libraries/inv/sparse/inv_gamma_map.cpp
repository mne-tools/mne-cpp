//=============================================================================================================
/**
 * @file     inv_gamma_map.cpp
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
 * @brief    InvGammaMap class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_gamma_map.h"

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

InvGammaMapResult InvGammaMap::compute(
    const MatrixXd& matGain,
    const MatrixXd& matData,
    const MatrixXd& matNoiseCov,
    int nIterations,
    double tolerance,
    double gammaThreshold)
{
    const int nChannels = static_cast<int>(matGain.rows());
    const int nSources = static_cast<int>(matGain.cols());
    const int nTimes = static_cast<int>(matData.cols());

    // Compute noise covariance inverse once
    MatrixXd matNoiseCovInv = matNoiseCov.ldlt().solve(MatrixXd::Identity(nChannels, nChannels));

    // Initialize gamma (source variance hyperparameters)
    VectorXd vecGamma = VectorXd::Ones(nSources);
    VectorXd vecGammaOld = vecGamma;

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

        // Extract active columns of G
        MatrixXd matG_active(nChannels, nActive);
        VectorXd vecGamma_active(nActive);
        for (int i = 0; i < nActive; ++i) {
            matG_active.col(i) = matGain.col(activeIdx[i]);
            vecGamma_active(i) = vecGamma(activeIdx[i]);
        }

        // Gamma as diagonal matrix: Gamma_active = diag(gamma_active)
        // Data covariance model: C_M = G_active * Gamma_active * G_active^T + NoiseCov
        MatrixXd matCm = matG_active * vecGamma_active.asDiagonal() * matG_active.transpose() + matNoiseCov;

        // Solve C_M^{-1} * M
        MatrixXd matCmInvM = matCm.ldlt().solve(matData);

        // Posterior mean: X_active = Gamma_active * G_active^T * C_M^{-1} * M
        MatrixXd matX_active = vecGamma_active.asDiagonal() * matG_active.transpose() * matCmInvM;

        // Write back to full solution
        matX.setZero();
        for (int i = 0; i < nActive; ++i) {
            matX.row(activeIdx[i]) = matX_active.row(i);
        }

        // Update gamma: gamma_i = ||X_i||^2_2 / T
        vecGammaOld = vecGamma;
        for (int i = 0; i < nActive; ++i) {
            int srcIdx = activeIdx[i];
            vecGamma(srcIdx) = matX_active.row(i).squaredNorm() / static_cast<double>(nTimes);
        }

        // Prune sources with gamma below threshold
        std::vector<int> newActive;
        newActive.reserve(nActive);
        for (int i = 0; i < nActive; ++i) {
            int srcIdx = activeIdx[i];
            if (vecGamma(srcIdx) >= gammaThreshold) {
                newActive.push_back(srcIdx);
            } else {
                vecGamma(srcIdx) = 0.0;
            }
        }
        activeIdx = newActive;

        // Check convergence: max|gamma_new - gamma_old| / max(max|gamma_old|, 1e-10) < tolerance
        double maxGammaOld = std::max(vecGammaOld.cwiseAbs().maxCoeff(), 1e-10);
        double maxRelChange = 0.0;
        for (int idx : activeIdx) {
            double relChange = std::abs(vecGamma(idx) - vecGammaOld(idx)) / maxGammaOld;
            maxRelChange = std::max(maxRelChange, relChange);
        }
        if (maxRelChange < tolerance)
            break;
    }

    // Build result
    InvGammaMapResult result;
    result.nIterations = actualIterations;
    result.vecGamma = vecGamma;

    // Collect active vertices
    QVector<int> finalActive;
    for (int i = 0; i < nSources; ++i) {
        if (vecGamma(i) >= gammaThreshold) {
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
    result.stc.method = InvEstimateMethod::GammaMAP;

    // Compute residual norm ||M - G*X||_F
    MatrixXd matResidual = matData - matGain * matX;
    result.residualNorm = matResidual.norm();

    return result;
}
