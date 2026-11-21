//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_cov_estimators.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Regularised covariance estimators for M/EEG noise covariances, matching MNE-Python's compute_covariance() API.
 *
 * M/EEG sensor counts routinely exceed the number of clean baseline
 * samples, which makes the unregularised sample covariance rank-deficient
 * and unusable as the @c C operator in MNE/dSPM/sLORETA inverse
 * solutions. This module provides the same family of regularised
 * estimators as @c mne.compute_covariance with @c method='auto', so
 * mne_sts users get a numerically well-conditioned covariance regardless
 * of how undersampled the input is.
 *
 * Six estimators are exposed: the analytic Ledoit-Wolf shrinkage,
 * Oracle Approximating Shrinkage (OAS), fixed diagonal regularisation
 * @f$C+\lambda\,\bar{\sigma}^2 I@f$, rank-reduced PCA, EM Factor
 * Analysis and a cross-validated auto-selector that picks the estimator
 * with the highest held-out Gaussian log-likelihood. All routines take
 * zero-mean data shaped @c (n_channels, n_samples) and return a
 * @c (cov, parameter) pair; the auto-selector additionally returns the
 * index of the winning method.
 *
 * Ledoit-Wolf and OAS follow scikit-learn's @c LedoitWolf and @c OAS
 * (which MNE-Python uses), so the resulting covariances match those
 * produced by MNE-Python.
 *
 * References: Ledoit & Wolf (2004), J. Multivariate Anal. 88(2);
 * Chen, Wiesel, Eldar & Hero (2010), IEEE TSP 58(10);
 * Engemann & Gramfort (2015), NeuroImage 108.
 */

#ifndef STS_COV_ESTIMATORS_H
#define STS_COV_ESTIMATORS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief A covariance shrunk towards its mean eigenvalue, @f$(1-\alpha)\,S + \alpha\,\mathrm{tr}(S)/p\;I@f$.
 *
 * @tparam Scalar   float or double.
 *
 * @snippet ex_sts_statistics/main.cpp sts_cov_templates_usage
 */
template<class Scalar>
struct ShrunkCovariance
{
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> covariance; /**< The shrunk covariance, p x p. */
    Scalar shrinkage = 0;                                             /**< The shrinkage coefficient alpha in [0, 1]. */
};

//=============================================================================================================
/**
 * @brief A Factor-Analysis model of a covariance, @f$W W^\top + \Psi@f$.
 *
 * @tparam Scalar   float or double.
 *
 * @snippet ex_sts_statistics/main.cpp sts_cov_templates_usage
 */
template<class Scalar>
struct FactorAnalysisCovariance
{
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> covariance; /**< W W^T + Psi, p x p. */
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> loadings;   /**< W, p x n_factors. */
    Eigen::Matrix<Scalar, Eigen::Dynamic, 1> noise_variance;          /**< The diagonal of Psi, p. */
    Scalar log_likelihood = 0;                                        /**< Log-likelihood of the data under the model. */
    int iterations = 0;                                               /**< EM iterations run. */
};

namespace detail
{

/**
 * @param[in] S       A sample covariance.
 * @param[in] alpha   The shrinkage coefficient.
 *
 * @return @p S shrunk towards its mean eigenvalue.
 */
template<class Scalar>
ShrunkCovariance<Scalar> shrink(Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> S, Scalar alpha)
{
    const Scalar mu = S.trace() / static_cast<Scalar>(S.rows());
    S *= Scalar(1) - alpha;
    S.diagonal().array() += alpha * mu;
    return {std::move(S), alpha};
}

} // namespace detail

//=============================================================================================================
/**
 * Ledoit-Wolf shrinkage with the analytic coefficient of Ledoit & Wolf (2004), as scikit-learn's
 * @c LedoitWolf(assume_centered=True) computes it. A header template, so float data on an embedded target
 * is estimated in float.
 *
 * @param[in] data   Zero-mean data, n_channels x n_samples.
 *
 * @return The shrunk covariance (n_channels x n_channels) and its coefficient.
 */
template<class Derived>
ShrunkCovariance<typename Derived::Scalar> ledoit_wolf_covariance(const Eigen::MatrixBase<Derived>& data)
{
    using Scalar = typename Derived::Scalar;
    const auto p = static_cast<Scalar>(data.rows());
    const auto n = static_cast<Scalar>(data.cols());

    // Sample covariance (1/n, as in the paper and scikit-learn)
    const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> S = (data * data.transpose()) / n;
    const Scalar mu = S.trace() / p;

    // delta = ||S - mu I||_F^2 / p
    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> diff = S;
    diff.diagonal().array() -= mu;
    const Scalar delta = diff.squaredNorm() / p;
    if (delta < Scalar(1e-30)) {
        return {S, Scalar(0)};
    }

    // beta = sum_k ||x_k x_k^T - S||_F^2 / (n^2 p) = (sum_k |x_k|^4 - n ||S||_F^2) / (n^2 p)
    const Scalar sumSquaredNorms = data.colwise().squaredNorm().array().square().sum();
    const Scalar beta = (sumSquaredNorms - n * S.squaredNorm()) / (n * n * p);
    return detail::shrink<Scalar>(S, std::clamp(beta / delta, Scalar(0), Scalar(1)));
}

//=============================================================================================================
/**
 * Oracle Approximating Shrinkage of Chen, Wiesel, Eldar & Hero (2010), with scikit-learn's correction of
 * the coefficient, as scikit-learn's @c OAS(assume_centered=True) computes it.
 *
 * @param[in] data   Zero-mean data, n_channels x n_samples.
 *
 * @return The shrunk covariance (n_channels x n_channels) and its coefficient.
 */
template<class Derived>
ShrunkCovariance<typename Derived::Scalar> oas_covariance(const Eigen::MatrixBase<Derived>& data)
{
    using Scalar = typename Derived::Scalar;
    const auto p = static_cast<Scalar>(data.rows());
    const auto n = static_cast<Scalar>(data.cols());

    const Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> S = (data * data.transpose()) / n;
    const Scalar mu = S.trace() / p;

    // With alpha = ||S||_F^2 / p^2: rho = (alpha + mu^2) / ((n + 1) (alpha - mu^2 / p))
    const Scalar alpha = S.squaredNorm() / (p * p);
    const Scalar den = (n + Scalar(1)) * (alpha - mu * mu / p);
    const Scalar rho = std::abs(den) > Scalar(1e-30) ? std::clamp((alpha + mu * mu) / den, Scalar(0), Scalar(1)) : Scalar(1);
    return detail::shrink<Scalar>(S, rho);
}

//=============================================================================================================
/**
 * Factor Analysis by the EM of Rubin & Thayer (1982) in scikit-learn's whitened form,
 * @f$W = \Psi^{1/2} V_k (\Lambda_k - I)^{1/2}@f$, until the log-likelihood changes by less than @p tol.
 * The data is centred first, like scikit-learn's @c FactorAnalysis.
 *
 * @param[in] data        Data, n_channels x n_samples.
 * @param[in] n_factors   Number of latent factors; 0 for min(n_channels, n_samples) / 2.
 * @param[in] max_iter    Maximum EM iterations.
 * @param[in] tol         Convergence tolerance on the log-likelihood.
 *
 * @return The model, its covariance and its log-likelihood.
 */
template<class Derived>
FactorAnalysisCovariance<typename Derived::Scalar> factor_analysis_covariance(const Eigen::MatrixBase<Derived>& data,
                                                                              int n_factors = 0,
                                                                              int max_iter = 200,
                                                                              typename Derived::Scalar tol = typename Derived::Scalar(1e-6))
{
    using Scalar = typename Derived::Scalar;
    using Matrix = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>;
    using Vector = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
    constexpr Scalar tiny = Scalar(1e-30);
    const Eigen::Index p = data.rows();
    const auto n = static_cast<Scalar>(data.cols());

    const Matrix centred = data.colwise() - data.rowwise().mean();
    const Matrix S = (centred * centred.transpose()) / n;

    int k = n_factors > 0 ? n_factors : std::max(1, static_cast<int>(std::min<Eigen::Index>(data.cols(), p)) / 2);
    k = std::min(k, static_cast<int>(p) - 1);

    FactorAnalysisCovariance<Scalar> model;
    Vector psi = S.diagonal();
    Matrix sigma = psi.asDiagonal();
    Scalar previous = -std::numeric_limits<Scalar>::infinity();
    for (int iter = 0; iter < max_iter; ++iter) {
        const Vector psiInvSqrt = psi.array().max(tiny).inverse().sqrt();
        const Eigen::SelfAdjointEigenSolver<Matrix> whitened(psiInvSqrt.asDiagonal() * S * psiInvSqrt.asDiagonal());
        const Vector scale = (whitened.eigenvalues().tail(k).array() - Scalar(1)).max(Scalar(0)).sqrt();
        model.loadings = psi.array().sqrt().matrix().asDiagonal() * whitened.eigenvectors().rightCols(k) * scale.asDiagonal();

        sigma = model.loadings * model.loadings.transpose();
        psi = (S - sigma).diagonal().array().max(tiny);
        sigma.diagonal() += psi;

        const Eigen::SelfAdjointEigenSolver<Matrix> fitted(sigma);
        const Vector evals = fitted.eigenvalues().array().max(tiny);
        const Matrix sigmaInv = fitted.eigenvectors() * evals.array().inverse().matrix().asDiagonal() * fitted.eigenvectors().transpose();
        model.log_likelihood = Scalar(-0.5) * n * (static_cast<Scalar>(p) * std::log(Scalar(2) * std::numbers::pi_v<Scalar>) + evals.array().log().sum() + (sigmaInv * S).trace());
        model.iterations = iter + 1;
        if (std::abs(model.log_likelihood - previous) < tol) {
            break;
        }
        previous = model.log_likelihood;
    }
    model.covariance = std::move(sigma);
    model.noise_variance = std::move(psi);
    return model;
}

//=============================================================================================================
/**
 * Covariance matrix estimators.
 *
 * Provides multiple regularised covariance estimation methods matching
 * MNE-Python's compute_covariance() API. Each method takes zero-mean data
 * (n_channels x n_samples) and returns a pair of (covariance, parameter).
 *
 * @brief Regularised covariance estimators (Ledoit-Wolf, OAS, fixed-diagonal, PCA, Factor Analysis, cross-validated auto-select) matching the MNE-Python compute_covariance() API.
 *
 * @snippet ex_sts_statistics/main.cpp sts_cov_estimators_usage
 */
class STSSHARED_EXPORT StsCovEstimators
{
public:
    //=========================================================================================================
    /**
     * @brief Ledoit-Wolf optimal shrinkage covariance estimator.
     *
     * Computes the shrinkage coefficient analytically using the formula from
     * Ledoit & Wolf (2004) "A well-conditioned estimator for large-dimensional
     * covariance matrices" (Journal of Multivariate Analysis, 88(2), 365-411).
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     *
     * @return std::pair containing the shrunk covariance matrix (n_channels x n_channels)
     *         and the shrinkage coefficient alpha in [0,1].
     */
    static std::pair<Eigen::MatrixXd, double> ledoitWolf(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * @brief Oracle Approximating Shrinkage (OAS) covariance estimator.
     *
     * Implements the OAS formula from Chen, Wiesel, Eldar & Hero (2010)
     * "Shrinkage Algorithms for MMSE Covariance Estimation"
     * (IEEE Transactions on Signal Processing, 58(10), 5016-5029).
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     *
     * @return std::pair containing the shrunk covariance matrix and
     *         the shrinkage coefficient rho in [0,1].
     */
    static std::pair<Eigen::MatrixXd, double> oas(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * @brief Fixed diagonal regularisation.
     *
     * Computes the sample covariance and adds a fixed fraction of the
     * mean eigenvalue to the diagonal: C_reg = C + reg * trace(C)/p * I.
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     * @param[in] dReg      Regularisation fraction (default 0.1).
     *
     * @return std::pair containing the regularised covariance and dReg.
     */
    static std::pair<Eigen::MatrixXd, double> diagonalFixed(const Eigen::MatrixXd& matData,
                                                            double dReg = 0.1);

    //=========================================================================================================
    /**
     * @brief PCA-based rank-reduced covariance estimator.
     *
     * Computes covariance in the subspace of the top-k principal components.
     * Components beyond rank are zeroed. If iRank <= 0, the rank is estimated
     * from the eigenvalue spectrum.
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     * @param[in] iRank     Number of principal components to retain (0 = auto).
     *
     * @return std::pair containing the rank-reduced covariance and
     *         the effective rank used.
     */
    static std::pair<Eigen::MatrixXd, double> pca(const Eigen::MatrixXd& matData,
                                                  int iRank = 0);

    //=========================================================================================================
    /**
     * @brief Factor Analysis covariance estimator via EM algorithm.
     *
     * Decomposes covariance as C = W*W^T + Psi, where W is a low-rank
     * loading matrix and Psi is a diagonal noise matrix. Uses the EM
     * algorithm from Rubin & Thayer (1982).
     *
     * @param[in] matData       Zero-mean data, n_channels x n_samples.
     * @param[in] iNFactors     Number of latent factors (default: min(p,n)/2).
     * @param[in] iMaxIter      Maximum EM iterations (default 200).
     * @param[in] dTol          Convergence tolerance on log-likelihood (default 1e-6).
     *
     * @return std::pair containing the Factor Analysis covariance and
     *         the final log-likelihood.
     */
    static std::pair<Eigen::MatrixXd, double> factorAnalysis(const Eigen::MatrixXd& matData,
                                                             int iNFactors = 0,
                                                             int iMaxIter = 200,
                                                             double dTol = 1e-6);

    //=========================================================================================================
    /**
     * @brief Auto-select the best covariance estimator via cross-validation.
     *
     * Runs all available estimators (empirical, shrunk/LW, OAS, diagonal_fixed,
     * PCA, factor_analysis) and selects the one with the highest average
     * Gaussian log-likelihood on held-out folds.
     *
     * @param[in] matData   Zero-mean data, n_channels x n_samples.
     * @param[in] iNFolds   Number of cross-validation folds (default 3).
     *
     * @return std::pair containing the best covariance matrix and the index
     *         of the winning method (0=empirical, 1=shrunk, 2=oas,
     *         3=diagonal_fixed, 4=pca, 5=factor_analysis).
     */
    static std::pair<Eigen::MatrixXd, double> autoSelect(const Eigen::MatrixXd& matData,
                                                         int iNFolds = 3);

    //=========================================================================================================
    /**
     * @brief Gaussian log-likelihood of held-out data given a covariance model.
     *
     * Computes: -0.5 * (p * log(2π) + log|Σ| + trace(Σ^{-1} * S_test))
     * where S_test is the sample covariance of the test data.
     *
     * @param[in] matTestData   Zero-mean test data, n_channels x n_test_samples.
     * @param[in] matCov        Covariance model, n_channels x n_channels.
     *
     * @return Average log-likelihood per sample.
     */
    static double gaussianLogLikelihood(const Eigen::MatrixXd& matTestData,
                                        const Eigen::MatrixXd& matCov);
};

} // namespace mne

#endif // STS_COV_ESTIMATORS_H
