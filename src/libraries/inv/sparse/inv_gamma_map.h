//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_gamma_map.h
 * @since 2026
 * @date  April 2026
 * @brief Gamma-MAP sparse Bayesian inverse solver — automatic-relevance-determination prior on per-source variance hyperparameters.
 *
 * @ref INVLIB::InvGammaMap implements the Gamma-MAP / sparse Bayesian
 * learning (SBL) solver of Wipf &amp; Rao, NeuroImage 44(3), 947-966
 * (2009). The algorithm models source amplitudes as Gaussians with
 * per-source variance hyperparameters @f$\gamma_{i}@f$ and uses an
 * EM / fixed-point update to iteratively re-estimate the @f$\gamma@f$
 * vector from the data; sources whose @f$\gamma_{i}@f$ collapses below
 * threshold are pruned, leaving a sparse active set. Output is an
 * @ref InvGammaMapResult carrying the @ref InvSourceEstimate, the
 * surviving active-vertex list, the final @f$\gamma@f$ vector,
 * iteration count and residual norm.
 */

#ifndef INV_GAMMA_MAP_H
#define INV_GAMMA_MAP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Result structure for the Gamma-MAP solver.
 */
struct INVSHARED_EXPORT InvGammaMapResult {
    InvSourceEstimate stc;
    QVector<int> activeVertices;
    Eigen::VectorXd vecGamma;       /**< Source variance hyperparameters. */
    int nIterations;
    double residualNorm;
};

//=============================================================================================================
/**
 * Gamma-MAP sparse inverse solver (Sparse Bayesian Learning).
 *
 * Iteratively estimates source variance hyperparameters (gamma) and prunes
 * sources whose gamma falls below a threshold, yielding a sparse solution.
 *
 * @brief Gamma-MAP sparse inverse solver.
 */
class INVSHARED_EXPORT InvGammaMap
{
public:
    //=========================================================================================================
    /**
     * Compute the Gamma-MAP inverse solution.
     *
     * @param[in] matGain           Forward gain matrix (n_channels x n_sources).
     * @param[in] matData           Measurement data (n_channels x n_times).
     * @param[in] matNoiseCov       Noise covariance matrix (n_channels x n_channels).
     * @param[in] nIterations       Maximum number of EM iterations.
     * @param[in] tolerance         Convergence tolerance on relative gamma change.
     * @param[in] gammaThreshold    Threshold below which sources are pruned.
     *
     * @return The Gamma-MAP result containing the sparse source estimate.
     */
    static InvGammaMapResult compute(
        const Eigen::MatrixXd& matGain,
        const Eigen::MatrixXd& matData,
        const Eigen::MatrixXd& matNoiseCov,
        int nIterations = 100,
        double tolerance = 1e-6,
        double gammaThreshold = 1e-10);
};

} // namespace INVLIB

#endif // INV_GAMMA_MAP_H
