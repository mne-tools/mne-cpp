//=============================================================================================================
/**
 * @file     resample.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
 * @brief    Declaration of Resample — polyphase anti-aliased rational resampling for MEG/EEG data.
 *
 * Algorithm:
 *   Given input rate oldSFreq and target rate newSFreq, the rational ratio p/q is computed after
 *   GCD reduction.  An anti-aliasing low-pass FIR is built using a Hamming-windowed sinc with
 *   cutoff = min(p,q) / (2·max(p,q)) (as a fraction of the upsampled Nyquist).  The filter is
 *   evaluated lazily via the polyphase identity:
 *
 *       y[m] = p · Σ_j  h[m·q − j·p] · x[j]
 *
 *   where the summation is restricted to j values for which the tap index m·q − j·p lies in [0, L−1].
 *   No explicit upsampling (inserting p−1 zeros) is performed.
 *
 *   The filter delay (halfLen = nZeros · max(p,q) samples at the upsampled rate) is absorbed into
 *   the output indexing so that the first output sample corresponds to the first input sample.
 *
 * Reference: similar approach to scipy.signal.resample_poly.
 */

#ifndef RESAMPLE_DSP_H
#define RESAMPLE_DSP_H

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
 * @brief Polyphase anti-aliased rational resampling for MEG/EEG data.
 *
 * @code
 *   // Resample from 1000 Hz to 250 Hz (4:1 decimation)
 *   Eigen::RowVectorXd downsampled = Resample::resample(vecData, 250.0, 1000.0);
 *
 *   // Resample all channels of a raw data matrix
 *   Eigen::MatrixXd ds = Resample::resampleMatrix(matData, 250.0, 1000.0);
 * @endcode
 */
class DSPSHARED_EXPORT Resample
{
public:
    //=========================================================================================================
    /**
     * Resample a single-channel row vector from @p dOldSFreq to @p dNewSFreq.
     *
     * The ratio is reduced to its lowest terms p/q via GCD, then a polyphase Hamming-windowed sinc
     * filter is applied.  The output length is ceil(nSamples · p / q).
     *
     * @param[in] vecData    Input row vector (calibrated, SI units).
     * @param[in] dNewSFreq  Target sampling frequency in Hz.
     * @param[in] dOldSFreq  Original sampling frequency in Hz.
     * @param[in] iNZeros    Number of sinc zero-crossings on each side of the kernel (default 10).
     *                        Higher values give better stopband attenuation at the cost of speed.
     *
     * @return Resampled row vector.
     */
    static Eigen::RowVectorXd resample(const Eigen::RowVectorXd& vecData,
                                        double                    dNewSFreq,
                                        double                    dOldSFreq,
                                        int                       iNZeros = 10);

    //=========================================================================================================
    /**
     * Resample every row of a data matrix.
     *
     * @param[in] matData    Input matrix (n_channels × n_samples).
     * @param[in] dNewSFreq  Target sampling frequency in Hz.
     * @param[in] dOldSFreq  Original sampling frequency in Hz.
     * @param[in] vecPicks   Optional channel indices to resample; if empty all rows are processed.
     *                        Non-selected rows are returned at their original length — in practice
     *                        always pass picks or leave empty for a uniform matrix result.
     * @param[in] iNZeros    Sinc zero-crossings per side (default 10).
     *
     * @return Resampled matrix (n_channels × n_new_samples).
     */
    static Eigen::MatrixXd resampleMatrix(const Eigen::MatrixXd&    matData,
                                           double                    dNewSFreq,
                                           double                    dOldSFreq,
                                           const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi(),
                                           int                       iNZeros  = 10);

private:
    //=========================================================================================================
    /**
     * Greatest common divisor via Euclidean algorithm.
     */
    static int gcd(int a, int b);

    //=========================================================================================================
    /**
     * Build a Hamming-windowed sinc low-pass FIR kernel scaled by @p p for the polyphase resampler.
     *
     * Length = 2·iNZeros·max(p,q) + 1.  Cutoff = min(p,q) / (2·max(p,q)) as a fraction of the
     * sampling frequency at the upsampled rate (i.e., half that as a fraction of Nyquist).
     * The coefficients are multiplied by @p p so that the unity-gain convention holds after the
     * implicit factor-of-p upsampling.
     *
     * @param[in] p       Upsampling factor (numerator of ratio).
     * @param[in] q       Downsampling factor (denominator of ratio).
     * @param[in] iNZeros Number of zero-crossings per side.
     *
     * @return FIR kernel row vector of length 2·iNZeros·max(p,q) + 1.
     */
    static Eigen::RowVectorXd buildKernel(int p, int q, int iNZeros);

    //=========================================================================================================
    /**
     * Evaluate the polyphase convolution for a single channel.
     *
     * @param[in] vecX    Input samples.
     * @param[in] vecH    FIR kernel from buildKernel() (length L = 2·halfLen+1).
     * @param[in] p       Upsampling factor.
     * @param[in] q       Downsampling factor.
     * @param[in] halfLen L/2 = iNZeros·max(p,q).
     *
     * @return Resampled row vector of length ceil(nIn·p/q).
     */
    static Eigen::RowVectorXd polyphaseConv(const Eigen::RowVectorXd& vecX,
                                             const Eigen::RowVectorXd& vecH,
                                             int                       p,
                                             int                       q,
                                             int                       halfLen);
};

} // namespace UTILSLIB

#endif // RESAMPLE_DSP_H
