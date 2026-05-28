//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file extended_infomax.cpp
 * @since 2026
 * @date  April 2026
 * @brief ExtendedInfomax class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "extended_infomax.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <random>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InfomaxResult ExtendedInfomax::compute(
    const MatrixXd& matData,
    int nComponents,
    int maxIterations,
    double learningRate,
    double tolerance,
    bool extendedMode,
    unsigned int seed)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());

    if (nComponents < 0) {
        nComponents = nChannels;
    }

    //=========================================================================================================
    // Step 1: Mean removal
    //=========================================================================================================
    VectorXd vecMean = matData.rowwise().mean();
    MatrixXd matCentered = matData.colwise() - vecMean;

    //=========================================================================================================
    // Step 2: PCA whitening
    //=========================================================================================================
    MatrixXd matCov = (matCentered * matCentered.transpose()) / static_cast<double>(nTimes - 1);

    SelfAdjointEigenSolver<MatrixXd> eigSolver(matCov);
    VectorXd vecEigVals = eigSolver.eigenvalues();
    MatrixXd matEigVecs = eigSolver.eigenvectors();

    // Eigen returns eigenvalues in ascending order; take the top nComponents (largest)
    VectorXd vecTopEigVals = vecEigVals.tail(nComponents).reverse();
    MatrixXd matTopEigVecs = matEigVecs.rightCols(nComponents).rowwise().reverse();

    // Whitening: P = D^{-1/2} * V^T
    VectorXd vecInvSqrtEig = vecTopEigVals.array().sqrt().inverse();
    MatrixXd matWhitening = vecInvSqrtEig.asDiagonal() * matTopEigVecs.transpose();

    // Dewhitening: P_inv = V * D^{1/2}
    VectorXd vecSqrtEig = vecTopEigVals.array().sqrt();
    MatrixXd matDewhitening = matTopEigVecs * vecSqrtEig.asDiagonal();

    // Whitened data
    MatrixXd matWhite = matWhitening * matCentered;

    //=========================================================================================================
    // Step 3: Initialize weights
    //=========================================================================================================
    MatrixXd matW = MatrixXd::Identity(nComponents, nComponents);

    if (seed != 0) {
        std::mt19937 gen(seed);
        std::normal_distribution<double> dist(0.0, 0.01);
        for (int i = 0; i < nComponents; ++i) {
            for (int j = 0; j < nComponents; ++j) {
                if (i != j) {
                    matW(i, j) = dist(gen);
                }
            }
        }
    }

    //=========================================================================================================
    // Step 4: Main iteration loop
    //=========================================================================================================
    InfomaxResult result;
    result.converged = false;
    result.nIterations = 0;

    const double dInvN = 1.0 / static_cast<double>(nTimes);
    MatrixXd matIdentity = MatrixXd::Identity(nComponents, nComponents);

    // Learning rate annealing: the natural gradient has a nonzero steady-state
    // when the assumed nonlinearity does not perfectly match the true source
    // distribution.  Geometric decay lets the step shrink toward zero so the
    // convergence criterion can fire.
    double dCurrentLR = learningRate;
    constexpr double dAnnealFactor = 0.998;

    for (int iter = 0; iter < maxIterations; ++iter) {
        // Compute sources
        MatrixXd matSources = matW * matWhite;

        // Estimate sign vector for extended mode
        VectorXd vecSigns = VectorXd::Ones(nComponents);
        if (extendedMode) {
            vecSigns = estimateSignVector(matSources);
        }

        // Compute nonlinearity
        MatrixXd matY(nComponents, nTimes);
        for (int i = 0; i < nComponents; ++i) {
            if (vecSigns(i) > 0) {
                // Super-Gaussian: g(u) = -tanh(u)
                matY.row(i) = -matSources.row(i).array().tanh();
            } else {
                // Sub-Gaussian: g(u) = tanh(u) - u
                matY.row(i) = matSources.row(i).array().tanh() - matSources.row(i).array();
            }
        }

        // Natural gradient: dW = lr * (I + Y * S^T / n_times) * W
        MatrixXd matGrad = matIdentity + (matY * matSources.transpose()) * dInvN;
        MatrixXd matDW = dCurrentLR * matGrad * matW;

        matW += matDW;
        result.nIterations = iter + 1;

        // Convergence: squared Frobenius norm of the weight update
        // (matches MNE-Python's criterion).  With learning rate annealing
        // the update shrinks each iteration.
        double dChange = matDW.squaredNorm();
        if (dChange < tolerance) {
            result.converged = true;
            break;
        }

        dCurrentLR *= dAnnealFactor;
    }

    //=========================================================================================================
    // Step 5: Compute output matrices
    //=========================================================================================================
    // Unmixing in original sensor space: W_total = W * P
    result.matUnmixing = matW * matWhitening;

    // Mixing matrix: pseudo-inverse of unmixing
    result.matMixing = result.matUnmixing.completeOrthogonalDecomposition().pseudoInverse();

    // Source activations
    result.matSources = result.matUnmixing * matCentered;

    return result;
}

//=============================================================================================================

VectorXd ExtendedInfomax::estimateSignVector(const MatrixXd& matSources)
{
    const int nComponents = static_cast<int>(matSources.rows());
    const int nTimes = static_cast<int>(matSources.cols());
    const double dInvN = 1.0 / static_cast<double>(nTimes);

    VectorXd vecSigns(nComponents);

    for (int i = 0; i < nComponents; ++i) {
        double dMean = matSources.row(i).mean();
        ArrayXd arrCentered = matSources.row(i).array() - dMean;
        double dM2 = (arrCentered.square()).sum() * dInvN;
        double dM4 = (arrCentered.square().square()).sum() * dInvN;

        // Excess kurtosis: m4/m2^2 - 3
        double dKurtosis = (dM2 > 0.0) ? (dM4 / (dM2 * dM2)) - 3.0 : 0.0;

        vecSigns(i) = (dKurtosis > 0.0) ? 1.0 : -1.0;
    }

    return vecSigns;
}
