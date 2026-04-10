//=============================================================================================================
/**
 * @file     inv_gamma_map.h
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
 * @brief    InvGammaMap class declaration.
 *
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
