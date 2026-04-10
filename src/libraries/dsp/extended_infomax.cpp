//=============================================================================================================
/**
 * @file     extended_infomax.cpp
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
 * @brief    ExtendedInfomax class definition.
 *
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
                // Super-Gaussian: g(u) = -2 * tanh(u)
                matY.row(i) = -2.0 * matSources.row(i).array().tanh();
            } else {
                // Sub-Gaussian: g(u) = tanh(u) - u
                matY.row(i) = matSources.row(i).array().tanh() - matSources.row(i).array();
            }
        }

        // Natural gradient: dW = learningRate * (I + Y * S^T / n_times) * W
        MatrixXd matGrad = matIdentity + (matY * matSources.transpose()) * dInvN;
        MatrixXd matDW = learningRate * matGrad * matW;

        // Check convergence
        double dMaxDW = matDW.array().abs().maxCoeff();
        double dMaxW = matW.array().abs().maxCoeff();

        matW += matDW;
        result.nIterations = iter + 1;

        if (dMaxW > 0.0 && (dMaxDW / dMaxW) < tolerance) {
            result.converged = true;
            break;
        }
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
