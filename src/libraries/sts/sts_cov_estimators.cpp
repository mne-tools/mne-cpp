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
 * Ledoit-Wolf and OAS compute scikit-learn's analytic shrinkage
 * coefficients directly on the @c (n_channels, n_samples) data and return
 * them alongside the resulting covariance. They, and the Factor-Analysis
 * EM, were adapted from Skigen (MIT, Copyright (c) 2026 The Skigen
 * Contributors), so that mne_sts needs nothing beyond Eigen.
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
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
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

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr double kTiny = 1e-30;

/** Shrinks the sample covariance towards its mean eigenvalue: (1 - alpha) S + alpha mu I. */
MatrixXd shrunk(const MatrixXd& S, double alpha)
{
    MatrixXd cov = (1.0 - alpha) * S;
    cov.diagonal().array() += alpha * S.trace() / static_cast<double>(S.rows());
    return cov;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::ledoitWolf(const MatrixXd& matData)
{
    const auto p = static_cast<double>(matData.rows());
    const auto n = static_cast<double>(matData.cols());

    // Sample covariance (1/n, as in the paper and scikit-learn); matData is zero-centred
    const MatrixXd S = (matData * matData.transpose()) / n;
    const double mu = S.trace() / p;

    // delta = ||S - mu I||_F^2 / p
    MatrixXd diff = S;
    diff.diagonal().array() -= mu;
    const double delta = diff.squaredNorm() / p;
    if (delta < kTiny) {
        return {S, 0.0};
    }

    // beta = sum_k ||x_k x_k^T - S||_F^2 / (n^2 p) = (sum_k |x_k|^4 - n ||S||_F^2) / (n^2 p)
    const double sumSquaredNorms = matData.colwise().squaredNorm().array().square().sum();
    const double beta = (sumSquaredNorms - n * S.squaredNorm()) / (n * n * p);

    const double alpha = std::clamp(beta / delta, 0.0, 1.0);
    return {shrunk(S, alpha), alpha};
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::oas(const MatrixXd& matData)
{
    const auto p = static_cast<double>(matData.rows());
    const auto n = static_cast<double>(matData.cols());

    const MatrixXd S = (matData * matData.transpose()) / n;
    const double mu = S.trace() / p;

    // Chen et al. (2010) as corrected in scikit-learn: with alpha = ||S||_F^2 / p^2,
    // rho = (alpha + mu^2) / ((n + 1) (alpha - mu^2 / p))
    const double alpha = S.squaredNorm() / (p * p);
    const double den = (n + 1.0) * (alpha - mu * mu / p);
    const double rho = std::abs(den) > kTiny ? std::clamp((alpha + mu * mu) / den, 0.0, 1.0) : 1.0;
    return {shrunk(S, rho), rho};
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
        if (iRank == 0)
            iRank = 1; // at least 1
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
    const Index p = matData.rows();
    const auto n = static_cast<double>(matData.cols());

    // Centred like scikit-learn's FactorAnalysis, which does not assume zero-mean data
    const MatrixXd centred = matData.colwise() - matData.rowwise().mean();
    const MatrixXd S = (centred * centred.transpose()) / n;

    int k = iNFactors > 0 ? iNFactors : std::max(1, static_cast<int>(std::min<Index>(matData.cols(), p)) / 2);
    k = std::min(k, static_cast<int>(p) - 1);

    // EM of Rubin & Thayer (1982) in the whitened form: W = Psi^1/2 V_k (Lambda_k - I)^1/2
    VectorXd psi = S.diagonal();
    MatrixXd W;
    MatrixXd sigma;
    double logLikelihood = 0.0;
    double previous = -std::numeric_limits<double>::infinity();
    for (int iter = 0; iter < iMaxIter; ++iter) {
        const VectorXd psiInvSqrt = psi.array().max(kTiny).inverse().sqrt();
        const SelfAdjointEigenSolver<MatrixXd> whitened(psiInvSqrt.asDiagonal() * S * psiInvSqrt.asDiagonal());
        const VectorXd scale = (whitened.eigenvalues().tail(k).array() - 1.0).max(0.0).sqrt();
        W = psi.array().sqrt().matrix().asDiagonal() * whitened.eigenvectors().rightCols(k) * scale.asDiagonal();

        sigma = W * W.transpose();
        psi = (S - sigma).diagonal().array().max(kTiny);
        sigma.diagonal() += psi;

        const SelfAdjointEigenSolver<MatrixXd> model(sigma);
        const VectorXd evals = model.eigenvalues().array().max(kTiny);
        const MatrixXd sigmaInv = model.eigenvectors() * evals.array().inverse().matrix().asDiagonal() * model.eigenvectors().transpose();
        logLikelihood = -0.5 * n * (static_cast<double>(p) * std::log(2.0 * std::numbers::pi) + evals.array().log().sum() + (sigmaInv * S).trace());
        if (std::abs(logLikelihood - previous) < dTol) {
            break;
        }
        previous = logLikelihood;
    }

    if (W.size() == 0) { // no iteration: the initial model
        sigma = psi.asDiagonal();
    }
    return {sigma, logLikelihood};
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
    return -0.5 * (static_cast<double>(p) * std::log(2.0 * std::numbers::pi) + logDet + trInvS);
}

//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::autoSelect(const MatrixXd& matData,
                                                         int iNFolds)
{
    const int n = static_cast<int>(matData.cols());

    if (iNFolds < 2)
        iNFolds = 2;
    if (iNFolds > n)
        iNFolds = n;

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
        case 1:
            result = ledoitWolf(matData);
            result.second = static_cast<double>(bestMethod);
            break;
        case 2:
            result = oas(matData);
            result.second = static_cast<double>(bestMethod);
            break;
        case 3:
            result = diagonalFixed(matData);
            result.second = static_cast<double>(bestMethod);
            break;
        case 4:
            result = pca(matData);
            result.second = static_cast<double>(bestMethod);
            break;
        case 5:
            result = factorAnalysis(matData);
            result.second = static_cast<double>(bestMethod);
            break;
        default:
            result = ledoitWolf(matData);
            result.second = 1.0;
            break;
    }

    return result;
}
