//=============================================================================================================
/**
 * @file     multitaper_tfr.h
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
 * @brief    MultitaperTfr class declaration — sliding-window multitaper time-frequency representation.
 *
 */

#ifndef MULTITAPER_TFR_H
#define MULTITAPER_TFR_H

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
 * @brief Result of a multitaper TFR computation.
 */
struct DSPSHARED_EXPORT MultitaperTfrResult
{
    QVector<Eigen::MatrixXd> tfrData;   ///< One matrix per channel, each: n_freqs × n_times
    Eigen::RowVectorXd       vecFreqs;  ///< Frequency axis in Hz
    Eigen::RowVectorXd       vecTimes;  ///< Time axis in seconds (window centres)
};

//=============================================================================================================
/**
 * @brief Sliding-window multitaper time-frequency representation.
 *
 * Slides a fixed-length analysis window across the data and computes a multitaper
 * PSD at each position, yielding a time-frequency power map per channel.
 *
 * @code
 *   // 600 Hz data, 256-sample windows, 128-sample step, half-bandwidth 4
 *   MultitaperTfrResult r = MultitaperTfr::compute(matData, 600.0);
 *   // r.tfrData[ch] → n_freqs × n_time_steps
 * @endcode
 */
class DSPSHARED_EXPORT MultitaperTfr
{
public:
    //=========================================================================================================
    /**
     * Compute sliding-window multitaper TFR for every channel of a data matrix.
     *
     * @param[in] matData         Data matrix (n_channels × n_times).
     * @param[in] sfreq           Sampling frequency in Hz.
     * @param[in] windowSize      Analysis window length in samples (default 256).
     * @param[in] stepSize        Step size in samples; -1 → windowSize / 2.
     * @param[in] halfBandwidth   Half-bandwidth parameter (NW); default 4.0.
     * @param[in] nTapers         Number of DPSS tapers; -1 → floor(2*halfBandwidth - 1).
     * @return                    MultitaperTfrResult with tfrData, vecFreqs, vecTimes.
     */
    static MultitaperTfrResult compute(const Eigen::MatrixXd& matData,
                                        double                 sfreq,
                                        int                    windowSize = 256,
                                        int                    stepSize = -1,
                                        double                 halfBandwidth = 4.0,
                                        int                    nTapers = -1);
};

} // namespace UTILSLIB

#endif // MULTITAPER_TFR_H
