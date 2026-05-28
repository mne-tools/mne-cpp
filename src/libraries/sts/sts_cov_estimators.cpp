//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_cov_estimators.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the regularised covariance estimators declared in sts_cov_estimators.h.
 *
 * Ledoit-Wolf and OAS forward to @c Skigen::LedoitWolf and @c Skigen::OAS
 * after transposing the input so the @c (n_channels, n_samples) STSLIB
 * convention is mapped onto the @c (n_samples, n_features) layout that
 * Skigen expects; the analytic shrinkage coefficients are returned
 * alongside the resulting covariance.
 *
 * The diagonal-fixed estimator adds
 * @f$\lambda \cdot \mathrm{tr}(C)/p \cdot I@f$ to the sample covariance.
 * The PCA estimator projects onto the top-@c k eigenvectors of the
 * sample covariance and reconstructs a low-rank approximation. The
 * Factor-Analysis estimator runs the Rubin & Thayer (1982) EM updates
 * until the log-likelihood plateaus and returns @c W*W^T + Psi.
 *
 * The auto-selector splits the samples into @c nFolds, fits every
 * estimator on the training samples, scores the held-out fold with the
 * Gaussian log-likelihood @f$-\tfrac12(p\log 2\pi + \log|\Sigma| +
 * \mathrm{tr}(\Sigma^{-1} S_\text{test}))@f$ and returns the covariance
 * with the highest average score along with a numeric index identifying
 * the winning method.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_cov_estimators.h"

//=============================================================================================================
// SKIGEN INCLUDES
//=============================================================================================================

#include <Skigen/Covariance>
#include <Skigen/Decomposition>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::ledoitWolf(const MatrixXd& matData)
{
    // matData is (p × n), already zero-centred; skigen expects (n × p)
    Skigen::LedoitWolf<double> lw(/*assume_centered=*/true);
    lw.fit(matData.transpose());
    return {lw.covariance(), lw.shrinkage()};
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::oas(const MatrixXd& matData)
{
    // matData is (p × n), already zero-centred; skigen expects (n × p)
    Skigen::OAS<double> oas_est(/*assume_centered=*/true);
    oas_est.fit(matData.transpose());
    return {oas_est.covariance(), oas_est.shrinkage()};
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::diagonalFixed(const MatrixXd& matData,
                                                             double dReg)
{
    const int p = static_cast<int>(matData.rows());
    const int n = static_cast<int>(matData.cols());

    // Sample covariance (1/n)
    MatrixXd cov = (matData * matData.transpose()) / static_cast<double>(n);

    // Add dReg * mean_eigenvalue * I  (mean_eigenvalue = trace / p)
    const double meanEig = cov.trace() / static_cast<double>(p);
    cov.diagonal().array() += dReg * meanEig;

    return {cov, dReg};
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::pca(const MatrixXd& matData,
                                                   int iRank)
{
    const int p = static_cast<int>(matData.rows());
    const int n = static_cast<int>(matData.cols());

    // Sample covariance (1/n)
    const MatrixXd S = (matData * matData.transpose()) / static_cast<double>(n);

    // Eigen decomposition (self-adjoint → eigenvalues in ascending order)
    SelfAdjointEigenSolver<MatrixXd> solver(S);
    VectorXd evals = solver.eigenvalues();
    MatrixXd evecs = solver.eigenvectors();

    // Auto-detect rank: count eigenvalues > max_eval * 1e-10
    if (iRank <= 0) {
        const double maxEval = evals.maxCoeff();
        const double threshold = maxEval * 1e-10;
        iRank = 0;
        for (int i = 0; i < p; ++i) {
            if (evals(i) > threshold)
                ++iRank;
        }
        if (iRank == 0) iRank = 1;  // at least 1
    }
    iRank = std::min(iRank, p);

    // Zero out eigenvalues below rank (keep top-k)
    // Eigenvalues are in ascending order, so zero out [0, p-iRank)
    for (int i = 0; i < p - iRank; ++i) {
        evals(i) = 0.0;
    }

    // Reconstruct: V * diag(evals) * V^T
    MatrixXd covPca = evecs * evals.asDiagonal() * evecs.transpose();

    return {covPca, static_cast<double>(iRank)};
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::factorAnalysis(const MatrixXd& matData,
                                                              int iNFactors,
                                                              int iMaxIter,
                                                              double dTol)
{
    // matData is (p × n), already zero-centred; skigen expects (n × p)
    Skigen::FactorAnalysis<double> fa(iNFactors, iMaxIter, dTol);
    fa.fit(matData.transpose());
    return {fa.covariance(), fa.log_likelihood()};
}

//=============================================================================================================

double StsCovEstimators::gaussianLogLikelihood(const MatrixXd& matTestData,
                                                const MatrixXd& matCov)
{
    const int p = static_cast<int>(matTestData.rows());
    const int n = static_cast<int>(matTestData.cols());

    // Eigen decomposition of covariance
    SelfAdjointEigenSolver<MatrixXd> solver(matCov);
    VectorXd evals = solver.eigenvalues().array().max(1e-30);
    MatrixXd evecs = solver.eigenvectors();

    // log|Σ|
    double logDet = evals.array().log().sum();

    // Σ^{-1}
    MatrixXd covInv = evecs * evals.array().inverse().matrix().asDiagonal() * evecs.transpose();

    // Test sample covariance (1/n)
    MatrixXd Stest = (matTestData * matTestData.transpose()) / static_cast<double>(n);

    // trace(Σ^{-1} * S_test)
    double trInvS = (covInv * Stest).trace();

    // Average log-likelihood per sample
    return -0.5 * (static_cast<double>(p) * std::log(2.0 * M_PI) + logDet + trInvS);
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::autoSelect(const MatrixXd& matData,
                                                          int iNFolds)
{
    const int n = static_cast<int>(matData.cols());

    if (iNFolds < 2) iNFolds = 2;
    if (iNFolds > n) iNFolds = n;

    // Create fold indices (simple sequential split)
    std::vector<int> indices(static_cast<size_t>(n));
    std::iota(indices.begin(), indices.end(), 0);

    // Shuffle for randomised folds
    std::mt19937 gen(42);
    std::shuffle(indices.begin(), indices.end(), gen);

    // Method names for indexing: 0=empirical, 1=shrunk, 2=oas, 3=diag_fixed, 4=pca, 5=fa
    const int nMethods = 6;
    std::vector<double> avgLL(static_cast<size_t>(nMethods), 0.0);

    const int foldSize = n / iNFolds;

    for (int fold = 0; fold < iNFolds; ++fold) {
        // Split into train and test
        int testStart = fold * foldSize;
        int testEnd = (fold == iNFolds - 1) ? n : (fold + 1) * foldSize;
        int nTest = testEnd - testStart;
        int nTrain = n - nTest;

        MatrixXd trainData(matData.rows(), nTrain);
        MatrixXd testData(matData.rows(), nTest);

        int trainIdx = 0;
        int testIdx = 0;
        for (int i = 0; i < n; ++i) {
            int col = indices[static_cast<size_t>(i)];
            if (i >= testStart && i < testEnd) {
                testData.col(testIdx++) = matData.col(col);
            } else {
                trainData.col(trainIdx++) = matData.col(col);
            }
        }

        // Zero-mean train and test independently
        trainData.colwise() -= trainData.rowwise().mean();
        testData.colwise() -= testData.rowwise().mean();

        // Fit each method on train, evaluate on test
        // 0: empirical
        {
            MatrixXd cov = (trainData * trainData.transpose()) / static_cast<double>(nTrain);
            // Small regularisation to avoid singular matrix
            cov.diagonal().array() += 1e-10 * cov.trace() / static_cast<double>(cov.rows());
            avgLL[0] += gaussianLogLikelihood(testData, cov);
        }
        // 1: shrunk (Ledoit-Wolf)
        {
            auto [cov, alpha] = ledoitWolf(trainData);
            avgLL[1] += gaussianLogLikelihood(testData, cov);
        }
        // 2: OAS
        {
            auto [cov, rho] = oas(trainData);
            avgLL[2] += gaussianLogLikelihood(testData, cov);
        }
        // 3: diagonal_fixed
        {
            auto [cov, reg] = diagonalFixed(trainData);
            avgLL[3] += gaussianLogLikelihood(testData, cov);
        }
        // 4: PCA
        {
            auto [cov, rank] = pca(trainData);
            // PCA can produce singular matrix — regularise for LL computation
            cov.diagonal().array() += 1e-10 * cov.trace() / static_cast<double>(cov.rows());
            avgLL[4] += gaussianLogLikelihood(testData, cov);
        }
        // 5: Factor Analysis
        {
            auto [cov, ll] = factorAnalysis(trainData);
            avgLL[5] += gaussianLogLikelihood(testData, cov);
        }
    }

    // Average across folds
    for (int m = 0; m < nMethods; ++m) {
        avgLL[static_cast<size_t>(m)] /= static_cast<double>(iNFolds);
    }

    // Find best method
    int bestMethod = 0;
    double bestLL = avgLL[0];
    for (int m = 1; m < nMethods; ++m) {
        if (avgLL[static_cast<size_t>(m)] > bestLL) {
            bestLL = avgLL[static_cast<size_t>(m)];
            bestMethod = m;
        }
    }

    // Re-fit best method on full data
    std::pair<MatrixXd, double> result;
    switch (bestMethod) {
    case 0: {
        MatrixXd cov = (matData * matData.transpose()) / static_cast<double>(n);
        cov.diagonal().array() += 1e-10 * cov.trace() / static_cast<double>(cov.rows());
        result = {cov, static_cast<double>(bestMethod)};
        break;
    }
    case 1: result = ledoitWolf(matData); result.second = static_cast<double>(bestMethod); break;
    case 2: result = oas(matData); result.second = static_cast<double>(bestMethod); break;
    case 3: result = diagonalFixed(matData); result.second = static_cast<double>(bestMethod); break;
    case 4: result = pca(matData); result.second = static_cast<double>(bestMethod); break;
    case 5: result = factorAnalysis(matData); result.second = static_cast<double>(bestMethod); break;
    default: result = ledoitWolf(matData); result.second = 1.0; break;
    }

    return result;
}
