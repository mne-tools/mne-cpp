//=============================================================================================================
/**
 * @file     connectivity_aec.h
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
 * @brief    ConnectivityAec — Amplitude Envelope Correlation connectivity metric.
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
