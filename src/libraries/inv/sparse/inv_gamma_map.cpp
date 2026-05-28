//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_gamma_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the Gamma-MAP / SBL solver.
 *
 * Implements the EM / fixed-point update loop for the per-source
 * variance hyperparameters @f$\gamma_{i}@f$: at each step build the
 * model covariance @f$\Sigma_{b} = \sigma_{n}^{2} I + G\Gamma G^{T}@f$,
 * compute the closed-form posterior mean for the source amplitudes,
 * update @f$\gamma_{i}@f$ from the row-norm contributions and prune
 * hyperparameters that have collapsed below threshold. The pruned
 * active-source list and surviving @f$\gamma@f$ vector are assembled
 * into the final @ref InvGammaMapResult.
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
