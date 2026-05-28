//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file spectrogram.h
 * @since 2026
 * @date  March 2026
 * @brief Gaussian-windowed short-time Fourier transform (STFT) spectrogram.
 *
 * Spectrogram slides a Gaussian window of user-defined size along a 1-D
 * input signal, applies an FFT to each windowed segment and stacks the
 * resulting magnitude spectra column-wise into a time–frequency matrix.
 * The window's standard deviation controls the classical time–frequency
 * resolution trade-off (Heisenberg–Gabor uncertainty): wider windows give
 * sharper frequency localisation at the expense of temporal blur, and
 * narrower windows do the opposite.
 *
 * The implementation parallelises across windowed segments via Qt Concurrent's
 * map–reduce pattern; @ref SpectogramInputData carries the per-window
 * parameters and @ref Spectrogram::reduce sums the partial spectrograms back
 * into the final matrix. Use this when a single global STFT view is
 * sufficient — for multitaper or wavelet representations see
 * @ref MultitaperTfr and @ref MorletTfr.
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
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

/**
 * @brief Input parameters for short-time Fourier transform spectrogram computation.
 */
struct SpectogramInputData {
    Eigen::VectorXd vecInputData;
    quint32 iRangeLow;
    quint32 iRangeHigh;
    qint32 window_size;
};

/**
 * @brief Computes time-frequency spectrograms via short-time Fourier transform with configurable window and overlap.
 */
class DSPSHARED_EXPORT Spectrogram
{

public:
    //=========================================================================================================
    /**
     * Calculates the spectrogram (tf-representation) of a given signal
     *
     * @param[in] signal         input-signal to calculate spectrogram of.
     * @param[in] windowSize     size of the window which is used (resolution in time an frequency is depending on it).
     *
     * @return spectrogram-matrix (tf-representation of the input signal).
     */
    static Eigen::MatrixXd makeSpectrogram(Eigen::VectorXd signal,
                                           qint32 windowSize);

private:
    //=========================================================================================================
    /**
     * Calculates a gaussean window function
     *
     * @param[in] sample_count   number of samples.
     * @param[in] scale          window width.
     * @param[in] translation    translation of the window among a signal.
     *
     * @return samples of window-vector.
     */
    static Eigen::VectorXd gaussWindow (qint32 sample_count,
                                        qreal scale,
                                        quint32 translation);

    //=========================================================================================================
    /**
     * Calculates the spectogram matrix for a given input data matrix.
     *
     * @param[in] data       The input data.
     *
     * @return               The spectogram matrix.
     */
    static Eigen::MatrixXd compute(const SpectogramInputData& data);

    //=========================================================================================================
    /**
     * Sums up (reduces) the in parallel processed spectogram matrix.
     *
     * @param[out] resultData    The result data.
     * @param[in] data          The incoming, temporary result data.
     */
    static void reduce(Eigen::MatrixXd &resultData,
                       const Eigen::MatrixXd &data);
};
}//namespace

#endif // SPECTROGRAM_H

