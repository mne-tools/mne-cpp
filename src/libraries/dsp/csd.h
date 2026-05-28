//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     csd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Cross-spectral density (CSD) estimation via averaged windowed FFTs.
 *
 * The cross-spectral density matrix is the frequency-domain counterpart of
 * the time-domain covariance and the central quantity behind coherence,
 * phase-locking value and weighted phase-lag index connectivity measures.
 * For each requested frequency the estimator computes the average outer
 * product @c X(f) · X(f)ᴴ of the complex Fourier coefficients across
 * windowed segments (and, optionally, across multitaper eigenspectra),
 * yielding the @c (n_channels × n_channels) Hermitian CSD matrix per
 * frequency bin.
 *
 * Both Welch-style segmenting (overlapping Hann-windowed segments) and
 * multitaper averaging (Slepian / DPSS tapers via @ref Dpss) are supported
 * to trade between spectral leakage, bias and variance. The output is
 * compatible with the connectivity routines in CONNLIB.
 */

#ifndef CSD_H
#define CSD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

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
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Result of a Cross-Spectral Density computation.
 */
struct DSPSHARED_EXPORT CsdResult
{
    Eigen::MatrixXcd matCsd;                ///< n_channels × n_channels mean CSD across selected frequencies
    QVector<Eigen::MatrixXcd> csdByFreq;    ///< One n_ch × n_ch CSD matrix per selected frequency bin
    Eigen::RowVectorXd vecFreqs;            ///< Frequency axis (Hz) for csdByFreq entries
};

//=============================================================================================================
/**
 * @brief Cross-Spectral Density (CSD) estimator.
 *
 * Provides three methods for computing the CSD matrix between channels:
 * multitaper (DPSS tapers), Fourier (Welch-style segmented), and Morlet wavelet.
 *
 * @code
 *   // Multitaper CSD: 600 Hz data, 0–60 Hz band, half-bandwidth 4
 *   CsdResult r = Csd::computeMultitaper(matData, 600.0, 0.0, 60.0, 4.0);
 *   // r.matCsd       → n_ch × n_ch mean CSD
 *   // r.csdByFreq[k] → n_ch × n_ch at frequency r.vecFreqs[k]
 * @endcode
 */
class DSPSHARED_EXPORT Csd
{
public:
    //=========================================================================================================
    /**
     * Compute CSD using multitaper (DPSS) spectral estimation.
     *
     * @param[in] matData        Data matrix (n_channels × n_samples).
     * @param[in] sfreq          Sampling frequency in Hz.
     * @param[in] fmin           Minimum frequency of interest in Hz (default 0).
     * @param[in] fmax           Maximum frequency in Hz; -1 → Nyquist (default -1).
     * @param[in] halfBandwidth  Half-bandwidth parameter (NW) for DPSS tapers (default 4).
     * @param[in] nTapers        Number of tapers; -1 → floor(2*halfBandwidth - 1) (default -1).
     * @return                   CsdResult with per-frequency and mean CSD matrices.
     */
    static CsdResult computeMultitaper(const Eigen::MatrixXd& matData,
                                       double sfreq,
                                       double fmin = 0.0,
                                       double fmax = -1.0,
                                       double halfBandwidth = 4.0,
                                       int nTapers = -1);

    //=========================================================================================================
    /**
     * Compute CSD using Welch-style Fourier segmented estimation.
     *
     * @param[in] matData   Data matrix (n_channels × n_samples).
     * @param[in] sfreq     Sampling frequency in Hz.
     * @param[in] fmin      Minimum frequency of interest in Hz (default 0).
     * @param[in] fmax      Maximum frequency in Hz; -1 → Nyquist (default -1).
     * @param[in] nFft      FFT / segment length in samples (default 256).
     * @param[in] overlap   Fractional overlap between adjacent segments, in [0, 1) (default 0.5).
     * @return              CsdResult with per-frequency and mean CSD matrices.
     */
    static CsdResult computeFourier(const Eigen::MatrixXd& matData,
                                    double sfreq,
                                    double fmin = 0.0,
                                    double fmax = -1.0,
                                    int nFft = 256,
                                    double overlap = 0.5);

    //=========================================================================================================
    /**
     * Compute CSD using Morlet wavelet convolution.
     *
     * @param[in] matData       Data matrix (n_channels × n_samples).
     * @param[in] sfreq         Sampling frequency in Hz.
     * @param[in] frequencies   Target frequencies in Hz.
     * @param[in] nCycles       Number of cycles in the Morlet wavelet (default 7).
     * @return                  CsdResult with one CSD matrix per target frequency.
     */
    static CsdResult computeMorlet(const Eigen::MatrixXd& matData,
                                   double sfreq,
                                   const Eigen::RowVectorXd& frequencies,
                                   int nCycles = 7);
};

} // namespace UTILSLIB

#endif // CSD_H
