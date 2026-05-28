//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file ica.h
 * @since March 2026
 * @brief FastICA-based independent component analysis for MEG / EEG artifact removal.
 *
 * Independent Component Analysis recovers a set of statistically independent
 * latent sources from their linear, instantaneous mixture at the sensors.
 * This class implements the deflationary FastICA algorithm of Hyvärinen and
 * Oja: after centring and PCA-whitening the data to remove second-order
 * correlations, each component direction is found one at a time as the
 * fixed point of the @c logcosh / @c tanh contrast function, with Gram–
 * Schmidt deflation against previously extracted components to guarantee
 * orthogonality.
 *
 * In a typical MEG / EEG pipeline the extracted components carry topographies
 * and time courses that often map cleanly onto physiological artifacts —
 * cardiac field, ocular blinks and saccades, EMG bursts — which can then
 * be zeroed in component space before back-projection to the sensors.
 *
 * Reference: A. Hyvärinen and E. Oja, "Independent Component Analysis:
 * Algorithms and Applications", Neural Networks 13(4-5):411-430 (2000).
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
