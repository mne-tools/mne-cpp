//=============================================================================================================
/**
 * @file     cosinefilter.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Declaration of the CosineFilter class
 *
 */

#ifndef COSINEFILTER_H
#define COSINEFILTER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../rtprocessing_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
/**
 * Creates a cosine filter response in the frequency domain.
 *
 * @brief Creates a cosine filter response in the frequency domain.
 */
class RTPROCESINGSHARED_EXPORT CosineFilter
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
} // NAMESPACE RTPROCESSINGLIB

#endif // COSINEFILTER_H
