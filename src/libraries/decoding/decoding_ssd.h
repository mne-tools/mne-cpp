//=============================================================================================================
/**
 * @file     decoding_ssd.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    DecodingSsd class declaration.
 *
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
 * @brief Spatio-Spectral Decomposition (SSD) for M/EEG signal decomposition.
 *
 * Mirrors mne.decoding.SSD from MNE-Python. SSD finds spatial filters that
 * maximise signal-band power relative to noise-band power. Delegates core
 * GED to Skigen::SSD.
 *
 * MNE-specific additions over bare Skigen::SSD:
 * - apply() for denoised low-rank factorisation
 * - Separate accessor for signal/noise frequency bands
 * - Sampling frequency stored for spectral analysis
 *
 * Input data: continuous (n_channels × n_times).
 *
 * @see mne.decoding.SSD in MNE-Python
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
};

} // namespace DECODINGLIB

#endif // DECODING_SSD_H
