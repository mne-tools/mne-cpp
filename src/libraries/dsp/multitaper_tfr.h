//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multitaper_tfr.h
 * @since April 2026
 * @brief Sliding-window multitaper time-frequency representation.
 *
 * MultitaperTfr applies the multitaper PSD estimator (@ref MultitaperPsd)
 * to consecutive overlapping windows of a signal, producing a
 * time-frequency matrix whose columns share the bias–variance properties
 * of Thomson's classical multitaper spectrum. Compared with the
 * complex-Morlet TFR (@ref MorletTfr) the frequency resolution is fixed
 * to the time-half-bandwidth product @c NW of the window rather than
 * scaling with frequency, which makes the multitaper TFR the preferred
 * choice when broadband line-noise leakage or non-stationary high-
 * frequency activity (e.g. high-gamma) has to be resolved without
 * over-smoothing in time.
 *
 * The number and concentration of tapers, window length, step size and
 * frequency grid are all user-controlled; defaults mirror those of
 * @c mne.time_frequency.tfr_array_multitaper.
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
