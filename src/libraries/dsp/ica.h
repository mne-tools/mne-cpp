//=============================================================================================================
/**
 * @file     ica.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Declaration of the ICA class providing FastICA-based independent component analysis
 *           for artifact removal in MEG/EEG data.
 *
 * Algorithm: A. Hyvärinen and E. Oja (2000). "Independent Component Analysis: Algorithms and
 *            Applications." Neural Networks 13(4-5):411-430.
 *            Uses the deflationary FastICA algorithm with logcosh (tanh) nonlinearity.
 */

#ifndef ICA_DSP_H
#define ICA_DSP_H

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
 * @brief Result of an ICA decomposition.
 *
 * Holds the mixing and unmixing matrices and the extracted source time series. The relationship is:
 *   sources   = unmixing  * (data - mean)
 *   data_recon = mixing   * sources + mean
 */
struct DSPSHARED_EXPORT IcaResult
{
    Eigen::MatrixXd matMixing;    /**< Mixing matrix A  (n_channels x n_components). Column k is the spatial pattern of component k. */
    Eigen::MatrixXd matUnmixing;  /**< Unmixing matrix W (n_components x n_channels). Row k is the spatial filter of component k. */
    Eigen::MatrixXd matSources;   /**< Source time series S (n_components x n_samples). */
    Eigen::VectorXd vecMean;      /**< Per-channel mean removed during centering (n_channels). */
    bool            bConverged;   /**< True if all components converged within maxIter. */
};

//=============================================================================================================
/**
 * @brief Independent Component Analysis using the FastICA algorithm (deflationary, logcosh nonlinearity).
 *
 * Typical MEG/EEG usage:
 * @code
 *   // Fit ICA on raw sensor data (n_channels x n_samples)
 *   IcaResult result = ICA::run(matRawData, 20);
 *
 *   // Inspect source waveforms and mark artifact components (e.g. {0, 3})
 *   QVector<int> exclude = {0, 3};
 *   Eigen::MatrixXd matClean = ICA::excludeComponents(matRawData, result, exclude);
 * @endcode
 */
class DSPSHARED_EXPORT ICA
{
public:
    //=========================================================================================================
    /**
     * Fit FastICA on the given data matrix.
     *
     * @param[in] matData       Input data (n_channels x n_samples). Each row is one sensor channel.
     * @param[in] nComponents   Number of independent components to extract.
     *                          Pass -1 (default) to use all channels.
     * @param[in] maxIter       Maximum iterations per component (default 200).
     * @param[in] tol           Convergence tolerance on |w_new · w_old| - 1 (default 1e-4).
     * @param[in] randomSeed    Seed for the random weight initialisation (default 42).
     *
     * @return IcaResult containing mixing/unmixing matrices and source time series.
     */
    static IcaResult run(const Eigen::MatrixXd& matData,
                         int    nComponents = -1,
                         int    maxIter     = 200,
                         double tol         = 1e-4,
                         int    randomSeed  = 42);

    //=========================================================================================================
    /**
     * Project new data through a previously fitted unmixing matrix.
     * The same mean centering as during fitting is applied.
     *
     * @param[in] matData   New data (n_channels x n_samples), same channel order as training data.
     * @param[in] result    Result from a previous ICA::run() call.
     *
     * @return Source matrix (n_components x n_samples).
     */
    static Eigen::MatrixXd applyUnmixing(const Eigen::MatrixXd& matData,
                                          const IcaResult&       result);

    //=========================================================================================================
    /**
     * Reconstruct sensor-space data after suppressing selected components.
     * The excluded component(s) are zeroed in source space before back-projection.
     *
     * @param[in] matData            Original sensor data (n_channels x n_samples).
     * @param[in] result             Result from ICA::run() on matData (or a representative segment).
     * @param[in] excludedComponents 0-based indices of components to suppress (e.g. ECG, EOG artifacts).
     *
     * @return Cleaned sensor data (n_channels x n_samples).
     */
    static Eigen::MatrixXd excludeComponents(const Eigen::MatrixXd& matData,
                                              const IcaResult&       result,
                                              const QVector<int>&    excludedComponents);

private:
    //=========================================================================================================
    /**
     * Center and whiten matCentered using eigendecomposition of the sample covariance.
     * Sets matWhitening (n_comp x n_ch) and matDewhitening (n_ch x n_comp).
     *
     * @param[in]  matCentered      Mean-centered data (n_channels x n_samples).
     * @param[in]  nComponents      Number of principal components to retain.
     * @param[out] matWhitening     Whitening matrix.
     * @param[out] matDewhitening   Dewhitening (pseudo-inverse of whitening) matrix.
     *
     * @return Whitened data (n_components x n_samples).
     */
    static Eigen::MatrixXd whiten(const Eigen::MatrixXd& matCentered,
                                   int                    nComponents,
                                   Eigen::MatrixXd&       matWhitening,
                                   Eigen::MatrixXd&       matDewhitening);
};

} // namespace UTILSLIB

#endif // ICA_DSP_H
