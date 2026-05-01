//=============================================================================================================
/**
 * @file     sts_cov_estimators.h
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
 * @brief    StsCovEstimators class declaration.
 *
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
 * @brief Covariance matrix estimators including shrinkage methods.
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
