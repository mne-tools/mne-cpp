//=============================================================================================================
/**
 * @file     ml_ssd.h
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
 * @brief    MlSsd class — Spatio-Spectral Decomposition.
 *
 * Equivalent to MNE-Python's mne.decoding.SSD.
 */

#ifndef ML_SSD_H
#define ML_SSD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPair>

//=============================================================================================================
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB
{

//=============================================================================================================
/**
 * @brief Spatio-Spectral Decomposition (SSD) for extracting oscillatory components.
 *
 * SSD finds spatial filters that maximize the power in a signal frequency band
 * relative to flanking noise frequency bands. The resulting components isolate
 * narrowband oscillations from broadband noise.
 *
 * Reference: Nikulin et al., NeuroImage 55(4), 1528-1535, 2011.
 *
 * Usage:
 * @code
 *   MlSsd ssd(4);
 *   QPair<double,double> signal(8.0, 12.0);  // alpha band
 *   QPair<double,double> noise(6.0, 14.0);   // flanking band
 *   ssd.fit(data, sFreq, signal, noise);
 *   Eigen::MatrixXd components = ssd.transform(data);
 * @endcode
 */
class MLSHARED_EXPORT MlSsd
{
public:
    //=========================================================================================================
    /**
     * @brief Construct SSD with given number of components.
     *
     * @param[in] nComponents  Number of SSD components to extract (default 4).
     */
    explicit MlSsd(int nComponents = 4);

    //=========================================================================================================
    /**
     * @brief Fit SSD filters from continuous or concatenated data.
     *
     * Computes covariance matrices for signal and noise frequency bands via
     * bandpass filtering, then solves the generalised eigenvalue problem
     * C_signal · w = λ · C_noise · w.
     *
     * @param[in] matData       Continuous data (n_channels × n_times).
     * @param[in] dSFreq        Sampling frequency in Hz.
     * @param[in] signalBand    Signal frequency band (low, high) in Hz.
     * @param[in] noiseBand     Noise frequency band (low, high) in Hz.
     * @param[in] dRegParam     Regularisation parameter added to noise cov diagonal (default 0.05).
     */
    void fit(const Eigen::MatrixXd& matData,
             double dSFreq,
             const QPair<double, double>& signalBand,
             const QPair<double, double>& noiseBand,
             double dRegParam = 0.05);

    //=========================================================================================================
    /**
     * @brief Transform data into SSD component space.
     *
     * @param[in] matData  Data (n_channels × n_times).
     *
     * @return Component activations (n_components × n_times).
     */
    Eigen::MatrixXd transform(const Eigen::MatrixXd& matData) const;

    //=========================================================================================================
    /**
     * @brief Fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const Eigen::MatrixXd& matData,
                                  double dSFreq,
                                  const QPair<double, double>& signalBand,
                                  const QPair<double, double>& noiseBand,
                                  double dRegParam = 0.05);

    //=========================================================================================================
    /**
     * @brief Get the spatial filters matrix.
     *
     * @return SSD spatial filters (n_components × n_channels).
     */
    const Eigen::MatrixXd& filters() const { return m_filters; }

    //=========================================================================================================
    /**
     * @brief Get the spatial patterns matrix.
     *
     * @return SSD spatial patterns (n_components × n_channels).
     */
    const Eigen::MatrixXd& patterns() const { return m_patterns; }

    //=========================================================================================================
    /**
     * @brief Get the eigenvalues (signal-to-noise ratios).
     */
    const Eigen::VectorXd& eigenvalues() const { return m_eigenvalues; }

    //=========================================================================================================
    /**
     * @brief Check if SSD has been fitted.
     */
    bool isFitted() const { return m_bFitted; }

private:
    /**
     * @brief Simple FIR bandpass filter applied to each row of data.
     *
     * @param[in] matData    Data (n_channels × n_times).
     * @param[in] dSFreq     Sampling frequency in Hz.
     * @param[in] dLowFreq   Low cutoff in Hz.
     * @param[in] dHighFreq  High cutoff in Hz.
     *
     * @return Filtered data (n_channels × n_times).
     */
    static Eigen::MatrixXd bandpassFilter(const Eigen::MatrixXd& matData,
                                           double dSFreq,
                                           double dLowFreq,
                                           double dHighFreq);

    int             m_nComponents;
    bool            m_bFitted = false;
    Eigen::MatrixXd m_filters;       /**< Spatial filters (n_components × n_channels). */
    Eigen::MatrixXd m_patterns;      /**< Spatial patterns (n_components × n_channels). */
    Eigen::VectorXd m_eigenvalues;   /**< Eigenvalues / signal-to-noise ratios. */
};

} // namespace MLLIB

#endif // ML_SSD_H
