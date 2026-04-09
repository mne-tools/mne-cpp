//=============================================================================================================
/**
 * @file     welch_psd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
 * @brief    WelchPsd class declaration — Welch's averaged periodogram PSD estimator.
 */

#ifndef WELCH_PSD_H
#define WELCH_PSD_H

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
 * @brief Result of a Welch PSD computation.
 *
 * matPsd rows correspond to channels (or the single channel passed to computeVector).
 * Each column corresponds to a frequency bin from vecFreqs.
 */
struct DSPSHARED_EXPORT WelchPsdResult
{
    Eigen::MatrixXd    matPsd;    ///< n_channels × (iNfft/2+1); one-sided PSD in unit²/Hz
    Eigen::RowVectorXd vecFreqs;  ///< Frequency axis in Hz, length iNfft/2+1
};

//=============================================================================================================
/**
 * @brief Welch's averaged-periodogram power spectral density estimator.
 *
 * Divides each channel into overlapping windowed segments, computes the FFT of each
 * segment, and averages the squared magnitudes.  The result is a one-sided PSD
 * normalised so that integrating over frequency recovers the mean signal power.
 *
 * @code
 *   // 600 Hz data, 512-sample FFT, 50 % overlap, Hann window
 *   WelchPsdResult r = WelchPsd::compute(matData, 600.0, 512);
 *   // r.matPsd   → n_channels × 257
 *   // r.vecFreqs → [0, 1.17, 2.34, …, 300] Hz
 * @endcode
 */
class DSPSHARED_EXPORT WelchPsd
{
public:
    /** Window function applied to each segment before the FFT. */
    enum WindowType { Hann=0, Hamming=1, Blackman=2, FlatTop=3 };

    //=========================================================================================================
    /**
     * Compute Welch PSD for every (selected) channel of a data matrix.
     *
     * @param[in] matData   Data matrix (n_channels × n_samples).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] iNfft     FFT length / segment length in samples (default 256).
     * @param[in] dOverlap  Fractional overlap between adjacent segments, in [0, 1) (default 0.5).
     * @param[in] window    Window function (default Hann).
     * @param[in] vecPicks  Channel row indices to process; empty = all channels.
     * @return              WelchPsdResult with matPsd (n_picks × iNfft/2+1) and vecFreqs.
     */
    static WelchPsdResult compute(const Eigen::MatrixXd&    matData,
                                   double                    dSFreq,
                                   int                       iNfft    = 256,
                                   double                    dOverlap = 0.5,
                                   WindowType                window   = Hann,
                                   const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

    //=========================================================================================================
    /**
     * Compute Welch PSD for a single channel row vector.
     *
     * @param[in] vecData   Single-channel data (1 × n_samples row vector).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] iNfft     FFT / segment length in samples.
     * @param[in] dOverlap  Fractional segment overlap in [0, 1).
     * @param[in] window    Window function.
     * @return              One-sided PSD row vector of length iNfft/2+1.
     */
    static Eigen::RowVectorXd computeVector(const Eigen::RowVectorXd& vecData,
                                             double     dSFreq,
                                             int        iNfft    = 256,
                                             double     dOverlap = 0.5,
                                             WindowType window   = Hann);

    //=========================================================================================================
    /**
     * Build the frequency axis for a given FFT length and sampling frequency.
     *
     * @param[in] iNfft   FFT length.
     * @param[in] dSFreq  Sampling frequency in Hz.
     * @return            Row vector of length iNfft/2+1 with frequencies in Hz.
     */
    static Eigen::RowVectorXd freqAxis(int iNfft, double dSFreq);

private:
    static Eigen::VectorXd buildWindow(int iN, WindowType window);
};

} // namespace UTILSLIB

#endif // WELCH_PSD_H
