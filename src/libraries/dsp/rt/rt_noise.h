//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     rt_noise.h
 * @author   Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August, 2014
 * @brief    Real-time noise power-spectral-density estimation from streaming data blocks.
 *
 * RtNoise accumulates a configurable number of contiguous data blocks, splits them into complete
 * FFT-length segments and averages their Hann-windowed periodograms into a one-sided PSD in dB re
 * unit^2/Hz, the same estimate as scipy.signal.welch with no overlap and no detrending. Longer
 * accumulation windows trade temporal responsiveness for spectral variance reduction. It is
 * synchronous: call it from whichever thread receives the data.
 *
 * This estimate feeds live noise-spectrum displays and complements @ref mne::RtCov whenever a quick
 * frequency-domain view of the noise floor is more informative than the full channel–channel
 * covariance matrix.
 */

#ifndef RT_NOISE_RTPROCESSING_H
#define RT_NOISE_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../dsp_global.h"

#include <Eigen/Core>

#include <optional>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Estimates the noise power spectral density of streaming data.
 *
 * @snippet ex_dsp_rt/main.cpp rt_noise_usage
 */
class DSPSHARED_EXPORT RtNoise
{
public:
    //=========================================================================================================
    /**
     * @param[in] fftLength   Number of samples per FFT segment.
     * @param[in] sfreq       Sampling frequency in Hz.
     * @param[in] blocks      Number of blocks to accumulate before a spectrum is estimated (at least 1).
     */
    RtNoise(int fftLength, double sfreq, int blocks = 10);

    //=========================================================================================================
    /**
     * Adds a block of data and returns the spectrum once enough blocks have arrived.
     *
     * Blocks may have any number of samples; a block with another number of channels than the blocks
     * before it starts the accumulation over.
     *
     * @param[in] data   A block of data, channels by samples.
     *
     * @return The PSD in dB re unit^2/Hz, channels by fftLength / 2 + 1 frequencies, once the configured
     *         number of blocks has arrived, after which the next estimate starts from nothing; no value
     *         before that.
     */
    std::optional<Eigen::MatrixXd> estimate_spectrum(const Eigen::MatrixXd& data);

private:
    int m_fftLength;              /**< Samples per FFT segment. */
    double m_sfreq;               /**< Sampling frequency in Hz. */
    int m_blocks;                 /**< Blocks per estimate. */
    std::vector<double> m_window; /**< Hann window without its zero end points, like hann(fftLength + 2)[1:-1]. */
    Eigen::MatrixXd m_data;       /**< Accumulated samples. */
    int m_received = 0;           /**< Blocks in m_data. */
};

} // namespace mne

#endif // RT_NOISE_RTPROCESSING_H
