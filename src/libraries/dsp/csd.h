//=============================================================================================================
/**
 * @file     csd.h
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
 * @brief    Csd class declaration — Cross-Spectral Density computation.
 *
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
