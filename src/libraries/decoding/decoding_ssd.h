//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_ssd.h
 * @since 2026
 * @date  May 2026
 * @brief Spatio-Spectral Decomposition (SSD) for noise-aware narrowband enhancement of continuous M/EEG.
 *
 * SSD extracts spatial filters that maximise variance in a narrow
 * signal band of interest while simultaneously minimising variance in
 * the flanking noise band. It does so by solving the generalised
 * eigenvalue problem
 * @f$\Sigma_s w = \lambda \Sigma_n w@f$, where @f$\Sigma_s@f$ is the
 * covariance of the data band-passed to the signal band and
 * @f$\Sigma_n@f$ is the covariance of the data band-passed to the
 * adjacent noise band. The result is a small set of components with
 * the highest achievable signal-to-noise ratio for the band of
 * interest, which is what makes SSD the standard preprocessor for
 * narrowband oscillation studies (alpha, mu, beta peaks). The method
 * was introduced by Nikulin, Nolte & Curio, *A novel method for
 * reliable and fast extraction of neuronal EEG/MEG oscillations on the
 * basis of spatio-spectral decomposition*, NeuroImage 55, 2011.
 *
 * @ref DecodingSsd mirrors @c mne.decoding.SSD and adds an explicit
 * @ref DecodingSsd::apply method that reconstructs the sensor-space
 * signal from a chosen subset of components, providing a low-rank
 * denoised version of the input that the connectivity and inverse
 * pipelines can consume without re-filtering. The covariance of the
 * noise band is shrunk towards a scaled identity by @c regParam to keep
 * the GED well-conditioned in the typical high-channel-count / short-
 * recording regime. Bandpassing is performed internally with a zero-
 * phase windowed-sinc FIR (forward + reverse pass) so the same call
 * site works regardless of the upstream filtering history.
 */

#ifndef DECODING_SSD_H
#define DECODING_SSD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE DECODINGLIB
//=============================================================================================================

namespace DECODINGLIB{

//=============================================================================================================
/**
 * @brief Spatio-spectral decomposition for narrowband signal enhancement on continuous M/EEG.
 *
 * Computes the spatial filters that jointly maximise variance in the
 * caller-specified signal band and minimise it in the surrounding
 * noise band, returning the @c n_components leading components ranked
 * by signal-to-noise eigenvalue. The fit takes raw continuous data
 * (channels × samples), bandpasses it twice internally with a zero-
 * phase windowed-sinc FIR, builds the two covariance matrices, shrinks
 * the noise covariance towards a scaled identity by @c regParam to keep
 * the generalised eigenproblem well-conditioned, and stores the
 * resulting filters, patterns and eigenvalues. The eigenvalues are
 * directly interpretable as the per-component signal-band power gain
 * over the noise band and are the standard diagnostic for choosing the
 * effective rank of the decomposition.
 *
 * @ref transform returns the components themselves — a low-dimensional
 * signal that already lives in the band of interest — while
 * @ref apply performs the low-rank back-projection
 * @f$X_{\text{clean}} = A_{:,1:k} W_{1:k,:} X@f$ that yields a
 * sensor-space, denoised reconstruction with the oscillation of
 * interest preserved and the broadband background suppressed; this is
 * the call most downstream pipelines (connectivity, inverse modelling)
 * want when SSD is used purely as a preprocessor.
 *
 * @see DECODINGLIB, @c mne.decoding.SSD
 */
class DECODINGSHARED_EXPORT DecodingSsd
{
public:
    //=========================================================================================================
    /**
     * Constructs an SSD decoder.
     *
     * @param[in] nComponents   Number of SSD components. Default: 6.
     * @param[in] regParam      Regularisation parameter for noise covariance. Default: 0.05.
     */
    explicit DecodingSsd(int nComponents = 6,
                         double regParam = 0.05);

    //=========================================================================================================
    /**
     * Fit SSD from continuous data.
     *
     * @param[in] data         Continuous data (n_channels × n_times).
     * @param[in] sfreq        Sampling frequency in Hz.
     * @param[in] signalLow    Lower edge of signal band (Hz).
     * @param[in] signalHigh   Upper edge of signal band (Hz).
     * @param[in] noiseLow     Lower edge of noise band (Hz).
     * @param[in] noiseHigh    Upper edge of noise band (Hz).
     */
    void fit(const Eigen::Ref<const Eigen::MatrixXd>& data,
             double sfreq,
             double signalLow, double signalHigh,
             double noiseLow, double noiseHigh);

    //=========================================================================================================
    /**
     * Apply spatial filters to data.
     *
     * @param[in] data  Continuous data (n_channels × n_times).
     * @return Filtered data (n_components × n_times).
     */
    Eigen::MatrixXd transform(const Eigen::Ref<const Eigen::MatrixXd>& data) const;

    //=========================================================================================================
    /**
     * Fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const Eigen::Ref<const Eigen::MatrixXd>& data,
                                 double sfreq,
                                 double signalLow, double signalHigh,
                                 double noiseLow, double noiseHigh);

    //=========================================================================================================
    /**
     * Denoise data by low-rank factorisation.
     *
     * Reconstructs M/EEG signals from which the dynamics described by the
     * excluded components is subtracted.
     *
     * @param[in] data  Continuous data (n_channels × n_times).
     * @return Denoised data (n_channels × n_times).
     */
    Eigen::MatrixXd apply(const Eigen::Ref<const Eigen::MatrixXd>& data) const;

    //=========================================================================================================
    /**
     * @return Spatial filters (n_components × n_channels).
     */
    const Eigen::MatrixXd& filters() const;

    //=========================================================================================================
    /**
     * @return Spatial patterns (n_channels × n_components).
     */
    const Eigen::MatrixXd& patterns() const;

    //=========================================================================================================
    /**
     * @return Eigenvalues (signal/noise power ratio).
     */
    const Eigen::VectorXd& eigenvalues() const;

    //=========================================================================================================
    /**
     * @return True if the model has been fitted.
     */
    bool isFitted() const;

private:
    int m_nComponents;
    double m_regParam;

    Eigen::MatrixXd m_filters;
    Eigen::MatrixXd m_patterns;
    Eigen::VectorXd m_eigenvalues;
    bool m_fitted = false;

    /**
     * Windowed-sinc FIR bandpass filter (zero-phase, forward+reverse).
     */
    static Eigen::MatrixXd bandpassFilter(const Eigen::MatrixXd& data,
                                          double sfreq,
                                          double lowFreq,
                                          double highFreq);
};

} // namespace DECODINGLIB

#endif // DECODING_SSD_H
