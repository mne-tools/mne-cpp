//=============================================================================================================
/**
 * @file     iirfilter.h
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
 * @brief    Declaration of IirFilter — Butterworth IIR filter design and application using
 *           numerically stable second-order sections (SOS / biquad cascade).
 *
 * The design follows the classical analogue-prototype → frequency-transform → bilinear-transform
 * pipeline described in:
 *   A. V. Oppenheim & R. W. Schafer, "Discrete-Time Signal Processing", 3rd ed., Ch. 7.
 */

#ifndef IIRFILTER_DSP_H
#define IIRFILTER_DSP_H

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
 * @brief Coefficients of one second-order IIR section (biquad).
 *
 * Transfer function (Direct-Form II):
 * @code
 *   H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
 * @endcode
 * Note: a0 is normalised to 1.  For first-order sections set b2 = a2 = 0.
 */
struct DSPSHARED_EXPORT IirBiquad
{
    double b0 = 1.0; /**< Numerator coefficient z^0. */
    double b1 = 0.0; /**< Numerator coefficient z^-1. */
    double b2 = 0.0; /**< Numerator coefficient z^-2. */
    double a1 = 0.0; /**< Denominator coefficient z^-1 (a0 normalised to 1). */
    double a2 = 0.0; /**< Denominator coefficient z^-2. */
};

//=============================================================================================================
/**
 * @brief Butterworth IIR filter design and application using second-order sections.
 *
 * All filter types (LP, HP, BP, BS) are supported via the classical analogue-prototype →
 * bilinear-transform design route with pre-warped cutoff frequencies.  The resulting
 * biquad cascade is numerically stable for high filter orders.
 *
 * Typical usage:
 * @code
 *   // Design a 4th-order zero-phase Butterworth bandpass 1–40 Hz at 1000 Hz sampling rate
 *   auto sos = IirFilter::designButterworth(4, IirFilter::BandPass, 1.0, 40.0, 1000.0);
 *
 *   // Apply to one channel
 *   Eigen::RowVectorXd filtered = IirFilter::applyZeroPhase(rawChannel, sos);
 *
 *   // Apply to all channels of a matrix
 *   Eigen::MatrixXd matFiltered = IirFilter::applyZeroPhaseMatrix(rawMatrix, sos);
 * @endcode
 */
class DSPSHARED_EXPORT IirFilter
{
public:
    //=========================================================================================================
    /**
     * @brief Filter type selector.
     */
    enum FilterType {
        LowPass  = 0, /**< Low-pass: attenuate above cutoffLow. */
        HighPass = 1, /**< High-pass: attenuate below cutoffLow. */
        BandPass = 2, /**< Band-pass: pass cutoffLow–cutoffHigh. */
        BandStop = 3  /**< Band-stop (notch): attenuate cutoffLow–cutoffHigh. */
    };

    //=========================================================================================================
    /**
     * Design a Butterworth filter as a cascade of second-order sections.
     *
     * @param[in] iOrder       Filter order (≥ 1).  For BP/BS the effective order is 2*iOrder.
     * @param[in] type         Filter type: LowPass, HighPass, BandPass, or BandStop.
     * @param[in] dCutoffLow   Lower cutoff frequency in Hz. Used for LP, HP, BP, BS.
     * @param[in] dCutoffHigh  Upper cutoff frequency in Hz. Used for BP and BS only.
     * @param[in] dSFreq       Sampling frequency in Hz.
     *
     * @return QVector of IirBiquad sections (multiply their transfer functions to get H(z)).
     */
    static QVector<IirBiquad> designButterworth(int        iOrder,
                                                FilterType type,
                                                double     dCutoffLow,
                                                double     dCutoffHigh,
                                                double     dSFreq);

    //=========================================================================================================
    /**
     * Apply a biquad cascade to one data row (causal, single-pass).
     *
     * @param[in] vecData   Input row vector.
     * @param[in] sos       Second-order sections from designButterworth().
     *
     * @return Filtered row vector (same length as vecData).
     */
    static Eigen::RowVectorXd applySos(const Eigen::RowVectorXd& vecData,
                                        const QVector<IirBiquad>& sos);

    //=========================================================================================================
    /**
     * Apply a biquad cascade with zero-phase (forward + backward pass) to one row.
     * The effective order is doubled and there is no phase distortion.
     *
     * @param[in] vecData   Input row vector.
     * @param[in] sos       Second-order sections from designButterworth().
     *
     * @return Zero-phase filtered row vector (same length as vecData).
     */
    static Eigen::RowVectorXd applyZeroPhase(const Eigen::RowVectorXd& vecData,
                                              const QVector<IirBiquad>& sos);

    //=========================================================================================================
    /**
     * Apply zero-phase filtering to every row of a matrix (each row = one channel).
     *
     * @param[in] matData   Input matrix (n_channels x n_samples).
     * @param[in] sos       Second-order sections from designButterworth().
     *
     * @return Filtered matrix (n_channels x n_samples).
     */
    static Eigen::MatrixXd applyZeroPhaseMatrix(const Eigen::MatrixXd&    matData,
                                                 const QVector<IirBiquad>& sos);

private:
    //=========================================================================================================
    /**
     * Compute Butterworth analogue prototype poles for a normalised LP prototype (cutoff = 1 rad/s).
     * Returns the n poles in the left half-plane (negative real part).
     *
     * @param[in] n  Filter order.
     * @return       Vector of n complex poles.
     */
    static QVector<std::complex<double>> butterworthPrototypePoles(int n);

    //=========================================================================================================
    /**
     * Convert one complex-conjugate analogue pole pair to a digital biquad via the bilinear transform.
     *
     * @param[in] pole   One of the complex conjugate pair.
     * @param[in] dC     Pre-warp constant 2*fs.
     * @param[in] dGain  Gain to apply to the section numerator (for passband normalisation).
     *
     * @return  IirBiquad with normalised coefficients.
     */
    static IirBiquad poleToDigitalBiquad(std::complex<double> pole,
                                          double               dC,
                                          double               dGain);

    //=========================================================================================================
    /**
     * Convert one real analogue pole to a digital first-order section via the bilinear transform
     * (b2 = a2 = 0 in the returned IirBiquad).
     *
     * @param[in] dPoleReal  Real pole value (must be negative for stability).
     * @param[in] dC         Pre-warp constant 2*fs.
     * @param[in] dGain      Gain to apply to the section numerator.
     *
     * @return  IirBiquad representing a first-order section (b2 = a2 = 0).
     */
    static IirBiquad realPoleToDigitalSection(double dPoleReal,
                                               double dC,
                                               double dGain);
};

} // namespace UTILSLIB

#endif // IIRFILTER_DSP_H
