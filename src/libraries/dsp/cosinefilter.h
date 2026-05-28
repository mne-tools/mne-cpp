//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file cosinefilter.h
 * @since 2026
 * @date  March 2026
 * @brief Frequency-domain cosine-tapered (raised-cosine) FIR filter design.
 *
 * CosineFilter assembles an FIR magnitude response directly in the frequency
 * domain by raising a rectangular pass-band to a cosine-tapered transition
 * region and then mapping the result back to the time domain via an inverse
 * FFT. The taper width controls the trade-off between transition steepness
 * and stop-band ripple: wide transitions give a smoother roll-off and very
 * low side-lobes, narrow transitions sharpen the cutoff at the cost of
 * Gibbs-style ringing.
 *
 * Because the design is symmetric in frequency, the resulting impulse
 * response is linear-phase (Type I), so any DC group delay can be removed
 * exactly by a forward / time-reverse pass — see
 * @ref FilterKernel::applyFFTFilter and @ref FirFilter::applyZeroPhase.
 * The class produces both the time-domain coefficients (@c m_vecCoeff) and
 * their zero-padded FFT (@c m_vecFftCoeff) ready for overlap-add use.
 */

#ifndef COSINEFILTER_H
#define COSINEFILTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
 * Creates a cosine filter response in the frequency domain.
 *
 * @brief Creates a cosine filter response in the frequency domain.
 */
class DSPSHARED_EXPORT CosineFilter
{
public:
    enum TPassType {LPF, HPF, BPF, NOTCH };

    //=========================================================================================================
    /**
     * Constructs a CosineFilter object.
     *
     */
    CosineFilter();

    //=========================================================================================================
    /**
     * Constructs a CosineFilter object.
     *
     * @param[in] fftLength length of the fft (multiple integer of 2^x).
     * @param[in] lowpass low cutoff frequency in Hz (not normed to sampling freq).
     * @param[in] lowpass_width determines the width of the filter slopes (steepness) in Hz (not normed to sampling freq).
     * @param[in] highpass highpass high cutoff frequency in Hz (not normed to sampling freq).
     * @param[in] highpass_width determines the width of the filter slopes (steepness) in Hz (not normed to sampling freq).
     * @param[in] sFreq sampling frequency.
     * @param[in] type filter type (lowpass, highpass, etc.).
     */
    CosineFilter(int fftLength,
                 float lowpass,
                 float lowpass_width,
                 float highpass,
                 float highpass_width,
                 double sFreq,
                 TPassType type);

    Eigen::RowVectorXcd    m_vecFftCoeff;   /**< the FFT-transformed forward filter coefficient set, required for frequency-domain filtering, zero-padded to m_iFftLength. */
    Eigen::RowVectorXd     m_vecCoeff;      /**< the time filter coefficient set*/

    int             m_iFilterOrder;
};
} // NAMESPACE UTILSLIB

#endif // COSINEFILTER_H
