//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file morlet_tfr.h
 * @since 2026
 * @date  April 2026
 * @brief Complex Morlet wavelet time-frequency representation (TFR).
 *
 * The complex Morlet wavelet ψ(t) = π^{-1/4} · e^{i2πf₀t} · e^{-t²/2σ²}
 * is a Gaussian-modulated complex exponential and is by far the most
 * popular wavelet for electrophysiological time-frequency analysis. At each
 * analysis frequency @c f the wavelet is dilated so the number of cycles
 * inside the Gaussian envelope (parameter @c n_cycles, typically 5–7)
 * stays approximately constant; this gives a logarithmically scaled
 * resolution Δt ∝ 1/f, Δf ∝ f matching the wavelet uncertainty principle.
 *
 * The TFR is evaluated as a convolution of the signal with each wavelet
 * — implemented in the frequency domain via FFT × FFT multiplication —
 * yielding a complex time-frequency tensor whose squared magnitude is the
 * instantaneous power and whose argument is the instantaneous phase.
 * Trial-wise averaging of the complex tensor gives the inter-trial phase
 * coherence (ITPC / phase-locking factor); averaging the power gives the
 * event-related spectral perturbation (ERSP).
 */

#ifndef MORLET_TFR_H
#define MORLET_TFR_H

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
 * @brief Result of a Morlet TFR computation for one channel.
 */
struct DSPSHARED_EXPORT MorletTfrResult
{
    Eigen::MatrixXd    matPower;   ///< n_freqs × n_times, instantaneous power (amplitude²)
    Eigen::RowVectorXd vecFreqs;   ///< Centre frequencies in Hz, length n_freqs
};

//=============================================================================================================
/**
 * @brief Complex Morlet wavelet time-frequency representation.
 *
 * For each requested centre frequency f the signal is convolved (via FFT) with a
 * complex Morlet wavelet whose time-domain standard deviation is
 *   σ_t = nCycles / (2π·f)
 * The instantaneous power at every time sample is |convolution|².
 *
 * @code
 *   // 30 log-spaced frequencies from 4 to 80 Hz, 7 cycles per wavelet
 *   RowVectorXd freqs = RowVectorXd::LinSpaced(30, 4.0, 80.0);
 *   MorletTfrResult r = MorletTfr::compute(vecSignal, 600.0, freqs);
 *   // r.matPower → 30 × n_samples instantaneous-power map
 * @endcode
 */
class DSPSHARED_EXPORT MorletTfr
{
public:
    //=========================================================================================================
    /**
     * Compute Morlet TFR for a single channel time series.
     *
     * @param[in] vecData   Single-channel data row vector (1 × n_samples).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] vecFreqs  Centre frequencies in Hz (row vector).
     * @param[in] dNCycles  Number of wavelet cycles (controls time–frequency trade-off; default 7).
     * @return              MorletTfrResult with matPower (n_freqs × n_samples).
     */
    static MorletTfrResult compute(const Eigen::RowVectorXd& vecData,
                                    double                    dSFreq,
                                    const Eigen::RowVectorXd& vecFreqs,
                                    double                    dNCycles = 7.0);

    //=========================================================================================================
    /**
     * Compute Morlet TFR for every (selected) channel of a data matrix.
     *
     * @param[in] matData   Data matrix (n_channels × n_samples).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] vecFreqs  Centre frequencies in Hz.
     * @param[in] dNCycles  Number of wavelet cycles (default 7).
     * @param[in] vecPicks  Channel row indices; empty = all channels.
     * @return              One MorletTfrResult per selected channel.
     */
    static QVector<MorletTfrResult> computeMultiChannel(
            const Eigen::MatrixXd&    matData,
            double                    dSFreq,
            const Eigen::RowVectorXd& vecFreqs,
            double                    dNCycles = 7.0,
            const Eigen::RowVectorXi& vecPicks = Eigen::RowVectorXi());

private:
    //=========================================================================================================
    /**
     * Build a complex Morlet wavelet for a given frequency.
     *
     * @param[in]  dFreq    Centre frequency in Hz.
     * @param[in]  dSFreq   Sampling frequency in Hz.
     * @param[in]  dNCycles Number of cycles.
     * @param[out] halfLen  Half-length of the returned wavelet in samples.
     * @return              Complex wavelet column vector of length 2·halfLen+1.
     */
    static Eigen::VectorXcd buildWavelet(double dFreq, double dSFreq, double dNCycles, int& halfLen);

    /** Return the smallest power of 2 ≥ n. */
    static int nextPow2(int n);
};

} // namespace UTILSLIB

#endif // MORLET_TFR_H
