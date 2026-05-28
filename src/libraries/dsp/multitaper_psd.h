//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multitaper_psd.h
 * @since April 2026
 * @brief Thomson multitaper power-spectral-density estimator.
 *
 * The multitaper estimator (Thomson, 1982) computes @c K independent
 * eigenspectra by tapering the input with the first @c K Discrete Prolate
 * Spheroidal Sequences (@ref Dpss) of time-half-bandwidth product @c NW,
 * then averages them. Because each Slepian taper concentrates almost all
 * of its energy inside the design bandwidth, the resulting estimate has
 * dramatically lower spectral leakage than a single-tapered periodogram
 * while keeping the bias–variance trade-off explicit: variance shrinks
 * roughly as @c 1/K, and frequency resolution degrades to @c ±W cycles per
 * sample.
 *
 * Adaptive eigenvalue weighting (Thomson's adaptive scheme) is supported
 * so tapers whose concentration λ_k is degraded by signal energy outside
 * the design band contribute less to the final spectrum. The estimator
 * mirrors MNE-Python's @c mne.time_frequency.psd_array_multitaper so MEG /
 * EEG analysis results stay reproducible across the two stacks.
 */

#ifndef MULTITAPER_PSD_H
#define MULTITAPER_PSD_H

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
 * @brief Result of a multitaper PSD computation.
 */
struct DSPSHARED_EXPORT MultitaperPsdResult
{
    Eigen::MatrixXd    matPsd;    ///< n_channels × n_freqs; one-sided PSD in unit²/Hz
    Eigen::RowVectorXd vecFreqs;  ///< Frequency axis in Hz, length nFft/2+1
};

//=============================================================================================================
/**
 * @brief Multitaper power spectral density estimator using DPSS (Slepian) tapers.
 *
 * Applies multiple orthogonal DPSS tapers to the data, computes the FFT of each
 * tapered segment, and averages the resulting periodograms weighted by the taper
 * eigenvalues. This provides a PSD estimate with reduced variance compared to
 * a single-taper (periodogram) approach.
 *
 * @code
 *   // 600 Hz data, half-bandwidth 4, default tapers
 *   MultitaperPsdResult r = MultitaperPsd::compute(matData, 600.0);
 *   // r.matPsd   → n_channels × (n_times/2+1)
 *   // r.vecFreqs → frequency axis in Hz
 * @endcode
 */
class DSPSHARED_EXPORT MultitaperPsd
{
public:
    //=========================================================================================================
    /**
     * Compute multitaper PSD for every channel of a data matrix.
     *
     * @param[in] matData         Data matrix (n_channels × n_times).
     * @param[in] sfreq           Sampling frequency in Hz.
     * @param[in] halfBandwidth   Half-bandwidth parameter (NW); default 4.0.
     * @param[in] nTapers         Number of DPSS tapers; -1 → floor(2*halfBandwidth - 1).
     * @param[in] nFft            FFT length; -1 → n_times.
     * @return                    MultitaperPsdResult with matPsd and vecFreqs.
     */
    static MultitaperPsdResult compute(const Eigen::MatrixXd& matData,
                                        double                 sfreq,
                                        double                 halfBandwidth = 4.0,
                                        int                    nTapers = -1,
                                        int                    nFft = -1);
};

} // namespace UTILSLIB

#endif // MULTITAPER_PSD_H
