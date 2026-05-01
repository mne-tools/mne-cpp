//=============================================================================================================
/**
 * @file     inv_tf_mxne.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Time-Frequency Mixed-Norm Estimate (TF-MxNE) sparse inverse solver.
 *
 * Equivalent to MNE-Python's mne.inverse_sparse.tf_mixed_norm().
 * Solves the L21+L1 problem in the time-frequency domain using
 * Gabor dictionary decomposition.
 */

#ifndef INV_TF_MXNE_H
#define INV_TF_MXNE_H

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
 * @brief Result structure for the TF-MxNE solver.
 */
struct INVSHARED_EXPORT InvTfMxneResult
{
    InvSourceEstimate stc;                      /**< Sparse source estimate (time domain). */
    QVector<int> activeVertices;                /**< Indices of active sources. */
    int nIterations = 0;                        /**< Number of iterations performed. */
    double residualNorm = 0.0;                  /**< Final residual norm. */
    Eigen::MatrixXd tfCoefficients;            /**< Time-frequency coefficients of active sources (n_active*n_freqs × n_times). */
};

//=============================================================================================================
/**
 * @brief Parameters for the TF-MxNE solver.
 */
struct INVSHARED_EXPORT InvTfMxneParams
{
    double dAlphaSpace = 0.5;           /**< Spatial regularization (L21 penalty on source groups). */
    double dAlphaTime = 0.1;            /**< Temporal regularization (L1 penalty on TF coefficients). */
    int iMaxIterations = 100;           /**< Maximum number of iterations. */
    double dTolerance = 1e-6;           /**< Convergence tolerance. */
    int iNFreqs = 8;                    /**< Number of frequency bins for Gabor dictionary. */
    double dFMin = 1.0;                 /**< Minimum frequency (Hz) for Gabor atoms. */
    double dFMax = 40.0;                /**< Maximum frequency (Hz) for Gabor atoms. */
    double dSFreq = 1000.0;            /**< Sampling frequency (Hz). */
    bool bDebias = true;                /**< Whether to debias the final solution. */
};

//=============================================================================================================
/**
 * @brief Time-Frequency Mixed-Norm Estimate (TF-MxNE) sparse inverse solver.
 *
 * Solves the inverse problem in the time-frequency domain:
 *   min ||M - G*Phi*Z||^2_F + alpha_space * ||Z||_21 + alpha_time * ||Z||_1
 *
 * where Phi is a Gabor dictionary (tight frame) and Z are the TF coefficients.
 * The L21 penalty enforces spatial sparsity (few active sources) while L1
 * enforces temporal sparsity (focal activations in time-frequency).
 *
 * Usage:
 * @code
 *   InvTfMxneParams params;
 *   params.dAlphaSpace = 0.5;
 *   params.dAlphaTime  = 0.1;
 *   params.dSFreq      = 1000.0;
 *   InvTfMxneResult result = InvTfMxne::compute(matGain, matData, params);
 * @endcode
 */
class INVSHARED_EXPORT InvTfMxne
{
public:
    //=========================================================================================================
    /**
     * @brief Compute the TF-MxNE inverse solution.
     *
     * @param[in] matGain   Forward gain matrix (n_channels × n_sources).
     * @param[in] matData   Measurement data (n_channels × n_times).
     * @param[in] params    TF-MxNE parameters.
     *
     * @return TF-MxNE result with sparse source estimate.
     */
    static InvTfMxneResult compute(const Eigen::MatrixXd& matGain,
                                    const Eigen::MatrixXd& matData,
                                    const InvTfMxneParams& params = InvTfMxneParams());

    //=========================================================================================================
    /**
     * @brief Build a Gabor dictionary (tight frame) for time-frequency decomposition.
     *
     * @param[in] iNSamples Number of time samples.
     * @param[in] iNFreqs   Number of frequency bins.
     * @param[in] dFMin     Minimum frequency (Hz).
     * @param[in] dFMax     Maximum frequency (Hz).
     * @param[in] dSFreq    Sampling frequency (Hz).
     *
     * @return Gabor dictionary matrix (n_atoms × n_samples), where n_atoms = n_freqs * n_samples.
     */
    static Eigen::MatrixXd buildGaborDictionary(int iNSamples, int iNFreqs,
                                                 double dFMin, double dFMax,
                                                 double dSFreq);
};

} // namespace INVLIB

#endif // INV_TF_MXNE_H
