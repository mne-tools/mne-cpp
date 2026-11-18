//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     spectrogram.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Martin Henfling <martin.henfling@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     September, 2015
 * @brief    Gaussian-windowed short-time Fourier transform (STFT) spectrogram.
 *
 * Spectrogram slides a Gaussian window of user-defined size along a 1-D
 * input signal, applies an FFT to each windowed segment and stacks the
 * resulting magnitude spectra column-wise into a time–frequency matrix.
 * The window's standard deviation controls the classical time–frequency
 * resolution trade-off (Heisenberg–Gabor uncertainty): wider windows give
 * sharper frequency localisation at the expense of temporal blur, and
 * narrower windows do the opposite.
 *
 * The windows are computed in parallel on the global thread pool, each straight into its column of
 * the result. Use this when a single global STFT view is sufficient — for multitaper or wavelet
 * representations see @ref mne::MultitaperTfr and @ref mne::MorletTfr.
 */

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * @brief Computes time-frequency spectrograms via short-time Fourier transform with configurable window and overlap.
 *
 * @snippet ex_dsp_spectral/main.cpp spectrogram_make
 */
class DSPSHARED_EXPORT Spectrogram
{
public:
    //=========================================================================================================
    /**
     * Calculates the spectrogram (tf-representation) of a given signal
     *
     * @param[in] signal         input-signal to calculate spectrogram of.
     * @param[in] windowSize     width of the Gaussian window in samples (trades time against frequency resolution);
     *                           0 for a fifteenth of the signal.
     *
     * @return Power |STFT|^2, N/2 x N for a signal of N samples: row k is the FFT bin at k * sfreq / N Hz,
     *         column j the window centred on sample j.
     */
    static Eigen::MatrixXd make_spectrogram(Eigen::VectorXd signal,
                                            int windowSize = 0);
};
} //namespace

#endif // SPECTROGRAM_H
