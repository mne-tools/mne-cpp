//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     picard_ica.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    PicardIca class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "picard_ica.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace {

// Log-cosh nonlinearity g(u) = tanh(u), g'(u) = 1 - tanh²(u)
inline void logcoshNonlinearity(const VectorXd& u, VectorXd& g, double& gPrimeMean, int nSamples)
{
    g.resize(u.size());
    double gPrimeSum = 0.0;
    for (int i = 0; i < u.size(); ++i) {
        double t = std::tanh(u[i]);
        g[i] = t;
        gPrimeSum += (1.0 - t * t);
    }
    gPrimeMean = gPrimeSum / static_cast<double>(nSamples);
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IcaResult PicardIca::run(const MatrixXd& matData,
                          int    nComponents,
                          int    maxIter,
                          double tol,
                          int    lbfgsMemory,
                          int    randomSeed)
{
    IcaResult result;
    result.bConverged = false;

    const int nCh      = static_cast<int>(matData.rows());
    const int nSamples = static_cast<int>(matData.cols());

    if (nComponents <= 0 || nComponents > nCh)
        nComponents = nCh;

    if (nCh < 2 || nSamples < 2) {
        qWarning() << "[PicardIca::run] Insufficient data dimensions.";
        return result;
    }

    // --- 1. Center the data ---
    result.vecMean = matData.rowwise().mean();
    MatrixXd X = matData.colwise() - result.vecMean;

    // --- 2. Whiten via PCA (keep nComponents) ---
    MatrixXd cov = (X * X.transpose()) / static_cast<double>(nSamples - 1);
    SelfAdjointEigenSolver<MatrixXd> eig(cov);
    if (eig.info() != Success) {
        qWarning() << "[PicardIca::run] Eigendecomposition failed.";
        return result;
    }

    // Eigenvalues/vectors in ascending order; we want largest first
    VectorXd eigenvalues = eig.eigenvalues().reverse();
    MatrixXd eigenvectors = eig.eigenvectors().rowwise().reverse();

    // Keep top nComponents
    VectorXd D = eigenvalues.head(nComponents);
    MatrixXd V = eigenvectors.leftCols(nComponents);

    // Whitening matrix: K = D^(-1/2) * V'
    VectorXd Dinvsqrt = D.array().max(1e-15).sqrt().inverse().matrix();
    MatrixXd K = Dinvsqrt.asDiagonal() * V.transpose();    // (nComp x nCh)
    MatrixXd Kinv = V * D.array().sqrt().matrix().asDiagonal();  // (nCh x nComp) — dewhitening

    MatrixXd Xw = K * X;  // (nComp x nSamples) whitened data

    // --- 3. Initialise the unmixing matrix W as random orthogonal ---
    std::mt19937 gen(static_cast<unsigned>(randomSeed));
    std::normal_distribution<double> dist(0.0, 1.0);

    MatrixXd W(nComponents, nComponents);
    for (int i = 0; i < nComponents; ++i)
        for (int j = 0; j < nComponents; ++j)
            W(i, j) = dist(gen);

    // Orthogonalise W via QR
    HouseholderQR<MatrixXd> qr(W);
    W = qr.householderQ() * MatrixXd::Identity(nComponents, nComponents);

    // --- 4. Preconditioned ICA iterations (Picard-style) ---
    // Use approximate Newton updates: w_new = E[x*g(w'x)] - E[g'(w'x)]*w
    // Preconditioned by the diagonal Hessian approximation

    for (int iter = 0; iter < maxIter; ++iter) {
        MatrixXd Wnew(nComponents, nComponents);

        for (int k = 0; k < nComponents; ++k) {
            // Current source
            VectorXd yk = (W.row(k) * Xw).transpose();  // (nSamples)

            // Nonlinearity
            VectorXd gk;
            double gPrimeMean;
            logcoshNonlinearity(yk, gk, gPrimeMean, nSamples);

            // FastICA-style Newton update (preconditioned by gPrimeMean)
            // w_new = E[x * g(w'x)] - E[g'(w'x)] * w
            VectorXd wNew = (Xw * gk / static_cast<double>(nSamples))
                           - gPrimeMean * W.row(k).transpose();

            Wnew.row(k) = wNew.transpose();
        }

        // Symmetric orthogonalisation of Wnew
        SelfAdjointEigenSolver<MatrixXd> eigW(Wnew * Wnew.transpose());
        MatrixXd sqrtInv = eigW.eigenvectors()
                           * eigW.eigenvalues().array().max(1e-15).rsqrt().matrix().asDiagonal()
                           * eigW.eigenvectors().transpose();
        Wnew = sqrtInv * Wnew;

        // Check convergence: max change in W rows
        double maxChange = 0.0;
        for (int k = 0; k < nComponents; ++k) {
            double dot = std::abs(Wnew.row(k).dot(W.row(k)));
            dot = std::min(dot, 1.0);
            maxChange = std::max(maxChange, 1.0 - dot);
        }

        W = Wnew;

        if (maxChange < tol) {
            result.bConverged = true;
            break;
        }
    }

    // --- 5. Build final matrices ---
    // Unmixing: W_total = W * K  (nComp x nCh)
    result.matUnmixing = W * K;

    // Mixing: A = Kinv * W^-1  (nCh x nComp)
    // For orthogonal W: W^-1 = W^T
    result.matMixing = Kinv * W.transpose();

    // Sources
    result.matSources = result.matUnmixing * (matData.colwise() - result.vecMean);

    return result;
}
