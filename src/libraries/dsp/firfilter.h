//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file firfilter.h
 * @since March 2026
 * @brief Discoverable design / apply façade over the @ref FilterKernel FIR engine.
 *
 * FirFilter mirrors the @ref IirFilter API surface (design, apply,
 * applyZeroPhase, applyZeroPhaseMatrix) so callers see one consistent
 * way of building and running FIR or IIR filters in mne-cpp. Internally
 * every call delegates to @ref FilterKernel, which executes the actual
 * frequency-domain overlap-add convolution against the precomputed FFT
 * of the impulse response.
 *
 * All cutoff / bandwidth parameters are specified in Hz; normalisation
 * against the sampling frequency happens inside the design step. Because
 * the underlying kernels are linear-phase Type-I FIRs, the zero-phase
 * helpers simply remove the deterministic @c (NumTaps-1)/2 group delay
 * after a single forward pass — no forward / backward filtering is
 * required.
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
