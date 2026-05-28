//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_cov_estimators.h
 * @since 2026
 * @date  May 2026
 * @brief Regularised covariance estimators for M/EEG noise covariances, matching MNE-Python's compute_covariance() API.
 *
 * M/EEG sensor counts routinely exceed the number of clean baseline
 * samples, which makes the unregularised sample covariance rank-deficient
 * and unusable as the @c C operator in MNE/dSPM/sLORETA inverse
 * solutions. This module provides the same family of regularised
 * estimators as @c mne.compute_covariance with @c method='auto', so
 * STSLIB users get a numerically well-conditioned covariance regardless
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
 * The Ledoit-Wolf and OAS back-ends are thin wrappers over the
 * scikit-learn-compatible @c Skigen::LedoitWolf and @c Skigen::OAS
 * classes so the resulting covariances are numerically identical to
 * those produced by MNE-Python.
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

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <utility>
#include <vector>
#include <string>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB {

//=============================================================================================================
/**
 * Covariance matrix estimators.
 *
 * Provides multiple regularised covariance estimation methods matching
 * MNE-Python's compute_covariance() API. Each method takes zero-mean data
 * (n_channels x n_samples) and returns a pair of (covariance, parameter).
 *
 * @brief Regularised covariance estimators (Ledoit-Wolf, OAS, fixed-diagonal, PCA, Factor Analysis, cross-validated auto-select) matching the MNE-Python compute_covariance() API.
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

} // namespace STSLIB

#endif // STS_COV_ESTIMATORS_H
