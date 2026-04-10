//=============================================================================================================
/**
 * @file     multitaper_psd.h
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
 * @brief    MultitaperPsd class declaration — multitaper power spectral density estimator.
 *
 */

#ifndef MULTITAPER_PSD_H
#define MULTITAPER_PSD_H

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
 * @brief Result of a multitaper PSD computation.
 */
struct DSPSHARED_EXPORT MultitaperPsdResult
{
    Eigen::MatrixXd    matPsd;    ///< n_channels × n_freqs; one-sided PSD in unit²/Hz
    Eigen::RowVectorXd vecFreqs;  ///< Frequency axis in Hz, length nFft/2+1
};

//=============================================================================================================
/**
 * @brief Multitaper power spectral density estimator using DPSS (Slepian) tapers.
 *
 * Applies multiple orthogonal DPSS tapers to the data, computes the FFT of each
 * tapered segment, and averages the resulting periodograms weighted by the taper
 * eigenvalues. This provides a PSD estimate with reduced variance compared to
 * a single-taper (periodogram) approach.
 *
 * @code
 *   // 600 Hz data, half-bandwidth 4, default tapers
 *   MultitaperPsdResult r = MultitaperPsd::compute(matData, 600.0);
 *   // r.matPsd   → n_channels × (n_times/2+1)
 *   // r.vecFreqs → frequency axis in Hz
 * @endcode
 */
class DSPSHARED_EXPORT MultitaperPsd
{
public:
    //=========================================================================================================
    /**
     * Compute multitaper PSD for every channel of a data matrix.
     *
     * @param[in] matData         Data matrix (n_channels × n_times).
     * @param[in] sfreq           Sampling frequency in Hz.
     * @param[in] halfBandwidth   Half-bandwidth parameter (NW); default 4.0.
     * @param[in] nTapers         Number of DPSS tapers; -1 → floor(2*halfBandwidth - 1).
     * @param[in] nFft            FFT length; -1 → n_times.
     * @return                    MultitaperPsdResult with matPsd and vecFreqs.
     */
    static MultitaperPsdResult compute(const Eigen::MatrixXd& matData,
                                        double                 sfreq,
                                        double                 halfBandwidth = 4.0,
                                        int                    nTapers = -1,
                                        int                    nFft = -1);
};

} // namespace UTILSLIB

#endif // MULTITAPER_PSD_H
