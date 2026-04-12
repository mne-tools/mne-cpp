//=============================================================================================================
/**
 * @file     ica.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Implementation of the ICA class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ica.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC MEMBER DEFINITIONS
//=============================================================================================================

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace {

//=============================================================================================================
/**
 * @brief Gram–Schmidt orthogonalise w against the first iComp rows of W.
 */
void gramSchmidt(VectorXd& w, const MatrixXd& W, int iComp)
{
    for (int j = 0; j < iComp; ++j) {
        w -= w.dot(W.row(j)) * W.row(j).transpose();
    }
}

} // anonymous namespace

//=============================================================================================================
// MEMBER DEFINITIONS
//=============================================================================================================

IcaResult ICA::run(const MatrixXd& matData,
                   int    nComponents,
                   int    maxIter,
                   double tol,
                   int    randomSeed)
{
    const int nCh      = static_cast<int>(matData.rows());
    const int nSamples = static_cast<int>(matData.cols());

    if (nComponents <= 0 || nComponents > nCh) {
        nComponents = nCh;
    }

    //----------------------------------------------------------------------------------------------------------
    // 1. Center: subtract per-channel mean
    //----------------------------------------------------------------------------------------------------------
    VectorXd vecMean = matData.rowwise().mean();
    MatrixXd matCentered = matData.colwise() - vecMean;

    //----------------------------------------------------------------------------------------------------------
    // 2. Whiten
    //----------------------------------------------------------------------------------------------------------
    MatrixXd matWhitening, matDewhitening;
    MatrixXd matWhite = whiten(matCentered, nComponents, matWhitening, matDewhitening);

    //----------------------------------------------------------------------------------------------------------
    // 3. FastICA — deflationary approach with logcosh (tanh) nonlinearity
    //
    //    For each component i, iterate:
    //      g   = tanh(W[i]^T * X_white)                    [1 x nSamples]
    //      g'  = 1 - g^2                                    [1 x nSamples]
    //      w_new = (1/n) * X_white * g^T - mean(g') * w_i
    //      Gram-Schmidt orthogonalise against already-found rows
    //      Normalise
    //      Convergence: |w_new · w_old| ≥ 1 - tol
    //----------------------------------------------------------------------------------------------------------
    std::mt19937 rng(static_cast<unsigned>(randomSeed));
    std::normal_distribution<double> dist(0.0, 1.0);

    MatrixXd W_ica(nComponents, nComponents);   // unmixing in whitened space
    bool bConverged = true;

    for (int i = 0; i < nComponents; ++i) {
        // Random unit-vector initialisation
        VectorXd w(nComponents);
        for (int k = 0; k < nComponents; ++k) {
            w(k) = dist(rng);
        }
        w.normalize();

        bool compConverged = false;
        for (int iter = 0; iter < maxIter; ++iter) {
            // g = tanh(w^T * X_white)  →  row vector (1 x nSamples)
            RowVectorXd u = w.transpose() * matWhite;
            RowVectorXd g     = u.array().tanh();
            RowVectorXd gPrime = 1.0 - g.array().square();    // derivative of tanh

            // Newton update
            VectorXd wNew = (matWhite * g.transpose()) / nSamples
                            - gPrime.mean() * w;

            // Deflation: orthogonalise against already-converged components
            gramSchmidt(wNew, W_ica, i);

            // Normalise
            double norm = wNew.norm();
            if (norm < 1e-12) {
                // Degenerate — reinitialise randomly and restart this component
                for (int k = 0; k < nComponents; ++k) {
                    w(k) = dist(rng);
                }
                w.normalize();
                continue;
            }
            wNew /= norm;

            // Convergence check: |w_new · w_old| should approach 1
            double delta = std::abs(wNew.dot(w)) - 1.0;
            w = wNew;

            if (std::abs(delta) < tol) {
                compConverged = true;
                break;
            }
        }

        if (!compConverged) {
            bConverged = false;
            qWarning() << "ICA::run: component" << i << "did not converge within" << maxIter << "iterations.";
        }

        W_ica.row(i) = w.transpose();
    }

    //----------------------------------------------------------------------------------------------------------
    // 4. Compose full (sensor-space) unmixing and mixing matrices
    //
    //    W_full = W_ica * W_whitening      (n_comp x n_ch)
    //    A_full = W_dewhitening * W_ica^T  (n_ch x n_comp)  — exact inverse when n_comp == n_ch,
    //                                                          pseudo-inverse otherwise
    //----------------------------------------------------------------------------------------------------------
    MatrixXd matUnmixing = W_ica * matWhitening;                     // n_comp x n_ch
    MatrixXd matMixing   = matDewhitening * W_ica.transpose();       // n_ch   x n_comp

    //----------------------------------------------------------------------------------------------------------
    // 5. Compute source time series
    //----------------------------------------------------------------------------------------------------------
    MatrixXd matSources = matUnmixing * matCentered;                 // n_comp x n_samples

    IcaResult result;
    result.matMixing   = std::move(matMixing);
    result.matUnmixing = std::move(matUnmixing);
    result.matSources  = std::move(matSources);
    result.vecMean     = std::move(vecMean);
    result.bConverged  = bConverged;

    return result;
}

//=============================================================================================================

MatrixXd ICA::applyUnmixing(const MatrixXd& matData, const IcaResult& result)
{
    // Centre using the training mean, then project
    MatrixXd matCentered = matData.colwise() - result.vecMean;
    return result.matUnmixing * matCentered;
}

//=============================================================================================================

MatrixXd ICA::excludeComponents(const MatrixXd& matData,
                                  const IcaResult& result,
                                  const QVector<int>& excludedComponents)
{
    if (excludedComponents.isEmpty()) {
        return matData;
    }

    // Subtract only the contribution of excluded components from the original data:
    //   artifact = A[:, excluded] * W[excluded, :] * (data - mean)
    //   cleaned  = data - artifact
    //
    // This preserves the full-rank original signal and removes only the estimated
    // artifact, matching the approach used by MNE-Python's ica.apply().

    MatrixXd matCentered = matData.colwise() - result.vecMean;

    // Build partial mixing and partial sources for only the excluded components
    const int nExcl  = excludedComponents.size();
    const int nCh    = static_cast<int>(result.matMixing.rows());
    const int nSamps = static_cast<int>(matCentered.cols());

    MatrixXd partialMixing(nCh, nExcl);
    MatrixXd partialSources(nExcl, nSamps);

    for (int i = 0; i < nExcl; ++i) {
        int idx = excludedComponents[i];
        if (idx >= 0 && idx < static_cast<int>(result.matUnmixing.rows())) {
            partialMixing.col(i)  = result.matMixing.col(idx);
            partialSources.row(i) = result.matUnmixing.row(idx) * matCentered;
        } else {
            partialMixing.col(i).setZero();
            partialSources.row(i).setZero();
        }
    }

    return matData - partialMixing * partialSources;
}

//=============================================================================================================

MatrixXd ICA::whiten(const MatrixXd& matCentered,
                      int             nComponents,
                      MatrixXd&       matWhitening,
                      MatrixXd&       matDewhitening)
{
    const int nSamples = static_cast<int>(matCentered.cols());

    // Sample covariance (unscaled)
    MatrixXd cov = matCentered * matCentered.transpose() / static_cast<double>(nSamples);

    // Eigendecomposition — SelfAdjointEigenSolver returns eigenvalues in ascending order
    SelfAdjointEigenSolver<MatrixXd> eig(cov);

    const int nCh = static_cast<int>(cov.rows());
    // Take the nComponents largest eigenvalues/vectors (rightmost columns)
    VectorXd eigenvalues  = eig.eigenvalues().tail(nComponents).cwiseMax(1e-12);
    MatrixXd eigenvectors = eig.eigenvectors().rightCols(nComponents);   // n_ch x n_comp

    // Whitening  : W_w = D^{-1/2} * V^T   (n_comp x n_ch)
    // Dewhitening: W_d = V * D^{1/2}       (n_ch   x n_comp)
    VectorXd sqrtEig    = eigenvalues.cwiseSqrt();
    VectorXd invSqrtEig = sqrtEig.cwiseInverse();

    matWhitening   = invSqrtEig.asDiagonal() * eigenvectors.transpose();
    matDewhitening = eigenvectors * sqrtEig.asDiagonal();

    return matWhitening * matCentered;
}
