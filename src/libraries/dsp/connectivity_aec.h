//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file connectivity_aec.h
 * @since May 2026
 * @brief ConnectivityAec — Amplitude Envelope Correlation connectivity metric.
 *
 * Implements both standard AEC and orthogonalized AEC (AEC-c / corrected) as described in:
 *   Brookes et al. (2012) NeuroImage 62(3):2271-2280.
 *   Hipp et al. (2012) Nature Neuroscience 15:884–890.
 */

#ifndef CONNECTIVITY_AEC_DSP_H
#define CONNECTIVITY_AEC_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Amplitude Envelope Correlation connectivity.
 *
 * @code
 *   // Standard AEC
 *   MatrixXd aec = ConnectivityAec::compute(bandpassedData);
 *
 *   // Orthogonalized AEC (corrected for source leakage)
 *   MatrixXd aecCorr = ConnectivityAec::computeOrthogonalized(bandpassedData);
 * @endcode
 */
class DSPSHARED_EXPORT ConnectivityAec
{
public:
    //=========================================================================================================
    /**
     * Compute AEC between all pairs of signals.
     *
     * Steps: Hilbert envelope → Pearson correlation of envelopes.
     *
     * @param[in] matData  Bandpassed data (n_signals x n_samples).
     *
     * @return Symmetric connectivity matrix (n_signals x n_signals) with values in [-1, 1].
     */
    static Eigen::MatrixXd compute(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Compute orthogonalized AEC (corrected for volume conduction / source leakage).
     *
     * For each pair (i, j): orthogonalize j w.r.t. i, compute envelope correlation,
     * then symmetrise by averaging AEC(i→j) and AEC(j→i).
     *
     * @param[in] matData  Bandpassed data (n_signals x n_samples).
     *
     * @return Symmetric connectivity matrix (n_signals x n_signals) with values in [0, 1].
     */
    static Eigen::MatrixXd computeOrthogonalized(const Eigen::MatrixXd& matData);

    //=========================================================================================================
    /**
     * Compute the analytic signal envelope via Hilbert transform (using FFT).
     *
     * @param[in] signal  Real-valued signal (n_samples).
     *
     * @return Envelope (n_samples) — absolute value of analytic signal.
     */
    static Eigen::VectorXd hilbertEnvelope(const Eigen::VectorXd& signal);

    //=========================================================================================================
    /**
     * Pearson correlation between two vectors.
     *
     * @param[in] a  First vector.
     * @param[in] b  Second vector (same length).
     *
     * @return Correlation coefficient in [-1, 1].
     */
    static double pearsonCorrelation(const Eigen::VectorXd& a, const Eigen::VectorXd& b);

private:
    ConnectivityAec() = delete;
};

} // namespace UTILSLIB

#endif // CONNECTIVITY_AEC_DSP_H
