//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_tf_mxne.h
 * @since 2026
 * @date  May 2026
 * @brief Time-Frequency Mixed-Norm Estimate (TF-MxNE) sparse inverse solver — joint L21 + L1 sparsity in a Gabor time-frequency dictionary.
 *
 * @ref INVLIB::InvTfMxne implements the TF-MxNE solver of Gramfort
 * et al., @em Time-Frequency Mixed-Norm Estimates: Sparse M/EEG imaging
 * with non-stationary source activations, NeuroImage 70, 410-422
 * (2013). It solves
 * @f$ \min \|M - G\Phi Z\|_F^{2} + \alpha_{s}\|Z\|_{2,1}
 *    + \alpha_{t}\|Z\|_{1} @f$
 * where @f$\Phi@f$ is a tight Gabor frame; the @c L21 penalty enforces
 * spatial sparsity (few active sources) while the @c L1 penalty
 * enforces temporal sparsity (focal activations in time-frequency).
 * Equivalent to mne-python's
 * @c mne.inverse_sparse.tf_mixed_norm. The accompanying
 * @ref InvTfMxneParams struct exposes the Gabor frequency range,
 * spatial / temporal regularisation, debias flag and convergence
 * controls.
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
