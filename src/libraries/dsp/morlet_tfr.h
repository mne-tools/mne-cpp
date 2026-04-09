//=============================================================================================================
/**
 * @file     morlet_tfr.h
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
 * @brief    MorletTfr class declaration — complex Morlet wavelet time-frequency representation.
 */

#ifndef MORLET_TFR_H
#define MORLET_TFR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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
 * @brief Result of a Morlet TFR computation for one channel.
 */
struct DSPSHARED_EXPORT MorletTfrResult
{
    Eigen::MatrixXd    matPower;   ///< n_freqs × n_times, instantaneous power (amplitude²)
    Eigen::RowVectorXd vecFreqs;   ///< Centre frequencies in Hz, length n_freqs
};

//=============================================================================================================
/**
 * @brief Complex Morlet wavelet time-frequency representation.
 *
 * For each requested centre frequency f the signal is convolved (via FFT) with a
 * complex Morlet wavelet whose time-domain standard deviation is
 *   σ_t = nCycles / (2π·f)
 * The instantaneous power at every time sample is |convolution|².
 *
 * @code
 *   // 30 log-spaced frequencies from 4 to 80 Hz, 7 cycles per wavelet
 *   RowVectorXd freqs = RowVectorXd::LinSpaced(30, 4.0, 80.0);
 *   MorletTfrResult r = MorletTfr::compute(vecSignal, 600.0, freqs);
 *   // r.matPower → 30 × n_samples instantaneous-power map
 * @endcode
 */
class DSPSHARED_EXPORT MorletTfr
{
public:
    //=========================================================================================================
    /**
     * Compute Morlet TFR for a single channel time series.
     *
     * @param[in] vecData   Single-channel data row vector (1 × n_samples).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] vecFreqs  Centre frequencies in Hz (row vector).
     * @param[in] dNCycles  Number of wavelet cycles (controls time–frequency trade-off; default 7).
     * @return              MorletTfrResult with matPower (n_freqs × n_samples).
     */
    static MorletTfrResult compute(const Eigen::RowVectorXd& vecData,
                                    double                    dSFreq,
                                    const Eigen::RowVectorXd& vecFreqs,
                                    double                    dNCycles = 7.0);

    //=========================================================================================================
    /**
     * Compute Morlet TFR for every (selected) channel of a data matrix.
     *
     * @param[in] matData   Data matrix (n_channels × n_samples).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] vecFreqs  Centre frequencies in Hz.
     * @param[in] dNCycles  Number of wavelet cycles (default 7).
     * @param[in] vecPicks  Channel row indices; empty = all channels.
     * @return              One MorletTfrResult per selected channel.
     */
    static QVector<MorletTfrResult> computeMultiChannel(
            const Eigen::MatrixXd&    matData,
            double                    dSFreq,
            const Eigen::RowVectorXd& vecFreqs,
            double                    dNCycles = 7.0,
            const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

private:
    //=========================================================================================================
    /**
     * Build a complex Morlet wavelet for a given frequency.
     *
     * @param[in]  dFreq    Centre frequency in Hz.
     * @param[in]  dSFreq   Sampling frequency in Hz.
     * @param[in]  dNCycles Number of cycles.
     * @param[out] halfLen  Half-length of the returned wavelet in samples.
     * @return              Complex wavelet column vector of length 2·halfLen+1.
     */
    static Eigen::VectorXcd buildWavelet(double dFreq, double dSFreq, double dNCycles, int& halfLen);

    /** Return the smallest power of 2 ≥ n. */
    static int nextPow2(int n);
};

} // namespace UTILSLIB

#endif // MORLET_TFR_H
