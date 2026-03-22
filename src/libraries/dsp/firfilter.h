//=============================================================================================================
/**
 * @file     firfilter.h
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
 * @brief    Declaration of FirFilter — a discoverable façade over the existing FilterKernel FIR API.
 *
 * FirFilter provides the same design/apply/applyZeroPhase/applyZeroPhaseMatrix pattern as IirFilter
 * so that both FIR and IIR filters are found in one consistent place in the DSP library.
 * All frequencies are specified in Hz; the normalisation to Nyquist is handled internally.
 *
 * The underlying engine is FilterKernel (overlap-add FFT convolution).
 */

#ifndef FIRFILTER_DSP_H
#define FIRFILTER_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
#include "filterkernel.h"

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
 * @brief Discoverable façade over the FilterKernel FIR-filter engine.
 *
 * Quick-start example — mirrors the IirFilter API:
 * @code
 *   // Design a 256-tap band-pass filter
 *   FilterKernel bpf = FirFilter::design(256, FirFilter::BandPass, 1.0, 40.0, sFreq);
 *
 *   // Apply to a single row vector (group delay already removed — symmetric FIR)
 *   Eigen::RowVectorXd out = FirFilter::apply(vecData, bpf);
 *
 *   // Apply to all rows of a matrix
 *   Eigen::MatrixXd clean = FirFilter::applyZeroPhaseMatrix(matData, bpf);
 * @endcode
 */
class DSPSHARED_EXPORT FirFilter
{
public:
    //=========================================================================================================
    /**
     * @brief FIR filter type — mirrors IirFilter::FilterType for a uniform API.
     */
    enum FilterType {
        LowPass  = 0,   /**< Low-pass filter. */
        HighPass = 1,   /**< High-pass filter. */
        BandPass = 2,   /**< Band-pass filter. */
        BandStop = 3    /**< Band-stop (notch) filter. */
    };

    //=========================================================================================================
    /**
     * @brief FIR design method — wraps FilterKernel's iDesignMethod integer codes.
     */
    enum DesignMethod {
        Cosine          = 0,  /**< Cosine window (fast, moderate roll-off). */
        ParksMcClellan  = 1   /**< Equiripple Parks-McClellan (optimal minimax). */
    };

    //=========================================================================================================
    /**
     * Design a linear-phase FIR filter using FilterKernel as the backend.
     *
     * Frequencies are given in Hz; the method converts them to the Nyquist-normalised form
     * expected by FilterKernel internally.
     *
     * @param[in] iOrder        Filter order (number of taps − 1).  Must be even for linear phase.
     * @param[in] type          Filter type (LowPass, HighPass, BandPass, BandStop).
     * @param[in] dCutoffLow    Lower cutoff frequency in Hz.
     *                          For LowPass/HighPass: the single cutoff.
     *                          For BandPass/BandStop: lower edge of the transition band.
     * @param[in] dCutoffHigh   Upper cutoff frequency in Hz (ignored for LowPass/HighPass).
     * @param[in] dSFreq        Sampling frequency in Hz.
     * @param[in] dTransition   Width of the transition band in Hz (Parks-McClellan steepness /
     *                          Cosine roll-off width).  Defaults to 5 Hz.
     * @param[in] method        Design method (Cosine or ParksMcClellan).
     *
     * @return A ready-to-use FilterKernel object.
     */
    static FilterKernel design(int          iOrder,
                                FilterType   type,
                                double       dCutoffLow,
                                double       dCutoffHigh,
                                double       dSFreq,
                                double       dTransition = 5.0,
                                DesignMethod method      = Cosine);

    //=========================================================================================================
    /**
     * Apply the filter to a single row vector.
     *
     * Uses overlap-add FFT convolution.  The group delay of a symmetric FIR is removed by the
     * FilterKernel engine (it takes the output segment starting at order/2), so the result is
     * already approximately zero-phase for symmetric designs.
     *
     * @param[in]     vecData   Input row vector.
     * @param[in,out] kernel    FilterKernel (prepareFilter is called if needed).
     *
     * @return Filtered row vector (same length as input).
     */
    static Eigen::RowVectorXd apply(const Eigen::RowVectorXd& vecData,
                                     FilterKernel&             kernel);

    //=========================================================================================================
    /**
     * Apply the filter in a zero-phase forward-backward pass to a single row vector.
     *
     * Runs the filter twice (forward then reverse) to achieve exactly zero phase shift.
     * Effective order is doubled; transition bandwidth is halved.
     *
     * @param[in]     vecData   Input row vector.
     * @param[in,out] kernel    FilterKernel.
     *
     * @return Zero-phase filtered row vector.
     */
    static Eigen::RowVectorXd applyZeroPhase(const Eigen::RowVectorXd& vecData,
                                              FilterKernel&             kernel);

    //=========================================================================================================
    /**
     * Apply the filter in a zero-phase forward-backward pass to every row of a matrix.
     *
     * @param[in]     matData   Input matrix (n_channels × n_samples).
     * @param[in,out] kernel    FilterKernel.
     * @param[in]     vecPicks  Optional row indices to filter; if empty, all rows are filtered.
     *
     * @return Zero-phase filtered matrix (same dimensions as input).
     */
    static Eigen::MatrixXd applyZeroPhaseMatrix(const Eigen::MatrixXd&       matData,
                                                 FilterKernel&                kernel,
                                                 const Eigen::RowVectorXi&    vecPicks = Eigen::RowVectorXi());
};

} // namespace UTILSLIB

#endif // FIRFILTER_DSP_H
