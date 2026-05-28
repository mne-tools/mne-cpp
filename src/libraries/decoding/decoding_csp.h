//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_csp.h
 * @since May 2026
 * @brief Common Spatial Patterns (CSP) for two-class discriminative spatial filtering of band-passed M/EEG.
 *
 * CSP finds spatial filters @f$w@f$ that maximise the variance of
 * narrow-band M/EEG for one class while simultaneously minimising it
 * for the other; equivalently, it diagonalises the per-class covariance
 * matrices @f$\Sigma_1@f$ and @f$\Sigma_2@f$ jointly via the
 * generalised eigenvalue problem
 * @f$\Sigma_1 w = \lambda (\Sigma_1 + \Sigma_2) w@f$. The components
 * with the largest and smallest eigenvalues carry the strongest
 * class-discriminative band-power and form the standard 2-class motor
 * imagery feature set used since Koles (1990) and popularised for BCI
 * by Blankertz, Tomioka, Lemm, Kawanabe & Müller, *Optimizing Spatial
 * Filters for Robust EEG Single-Trial Analysis*, IEEE Signal Processing
 * Magazine 25(1), 2008.
 *
 * @ref DecodingCsp mirrors the public surface of @c mne.decoding.CSP
 * but implements the GED inline with Eigen so no LAPACK dependency is
 * required, which matters for the WebAssembly target. Beyond the
 * upstream algorithm it provides the @c TransformMode switch
 * (@c AveragePower returns one log- or z-scored band-power feature per
 * component and epoch; @c CspSpace returns the time-resolved
 * projection), the closed-form @c inverseTransform back to sensor space
 * via the patterns matrix, and persistent @c mean / @c stddev vectors
 * for cross-session normalisation. Inputs are always epoched and
 * already band-passed in the discriminative frequency range — CSP itself
 * is purely spatial.
 */

#ifndef DECODING_CSP_H
#define DECODING_CSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE DECODINGLIB
//=============================================================================================================

namespace DECODINGLIB{

//=============================================================================================================
/**
 * @brief Common Spatial Patterns decoder for two-class discriminative spatial filtering.
 *
 * Estimates the joint diagonaliser of two class-conditional covariance
 * matrices estimated from labelled, band-passed epochs and exposes the
 * top and bottom @c n_components eigenvectors as a bank of spatial
 * filters; the corresponding patterns (the columns of the pseudoinverse
 * of the filter matrix) describe the sensor-space activity that each
 * filter is sensitive to and are what should be plotted as topographies
 * for neurophysiological interpretation, as discussed by Haufe et al.
 * 2014. After @ref fit the class works as a deterministic feature
 * extractor: @ref transform reduces an epoch tensor either to one
 * log-power (or z-scored) value per component (@c AveragePower, the
 * standard input to an LDA / logistic-regression classifier) or to the
 * full time-resolved projection (@c CspSpace, useful for downstream
 * Riemannian or deep-learning stages).
 *
 * The implementation expects exactly two unique class labels in @c y
 * and epochs already restricted to the discriminative frequency band
 * (e.g.\ 8–30 Hz for sensorimotor rhythms); no regularisation is
 * applied to the covariance estimate, so callers should ensure enough
 * trials per class to avoid the ill-conditioned regime that motivates
 * the regularised variants of Lotte & Guan 2011. @ref inverseTransform
 * provides the closed-form back-projection of a power-feature vector
 * into sensor space, which is what the application layer uses to render
 * CSP topographies side-by-side with the discriminative scores.
 *
 * @see DECODINGLIB, @c mne.decoding.CSP
 */
class DECODINGSHARED_EXPORT DecodingCsp
{
public:
    //=========================================================================================================
    /**
     * Transform mode for the CSP output.
     */
    enum class TransformMode {
        AveragePower,   /**< Return average band power per component (n_epochs × n_components). */
        CspSpace        /**< Return data projected into CSP space (n_epochs × n_components × n_times). */
    };

    //=========================================================================================================
    /**
     * Constructs a CSP decoder.
     *
     * @param[in] nComponents     Number of CSP components (split between classes). Default: 4.
     * @param[in] transformInto   Feature extraction mode. Default: AveragePower.
     * @param[in] useLog          If true (default) and transformInto == AveragePower, apply log transform;
     *                            otherwise z-score features using mean_ and std_.
     */
    explicit DecodingCsp(int nComponents = 4,
                         TransformMode transformInto = TransformMode::AveragePower,
                         bool useLog = true);

    //=========================================================================================================
    /**
     * Fit CSP from labelled epoch data (binary classification).
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @param[in] y       Class label for each epoch (must contain exactly 2 unique values).
     */
    void fit(const std::vector<Eigen::MatrixXd>& epochs,
             const Eigen::VectorXi& y);

    //=========================================================================================================
    /**
     * Transform epoch data using the fitted CSP filters.
     *
     * When transformInto == AveragePower, returns (n_epochs × n_components)
     * with log-transformed or z-scored mean band power.
     *
     * When transformInto == CspSpace, returns a matrix where each
     * n_components rows correspond to one epoch's CSP-space projection.
     * The shape is (n_epochs * n_components, n_times).
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @return Feature matrix.
     */
    Eigen::MatrixXd transform(const std::vector<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * Fit and transform in one step.
     *
     * @param[in] epochs  Epoch data.
     * @param[in] y       Class labels.
     * @return Feature matrix (same as transform output).
     */
    Eigen::MatrixXd fitTransform(const std::vector<Eigen::MatrixXd>& epochs,
                                 const Eigen::VectorXi& y);

    //=========================================================================================================
    /**
     * Project CSP power features back to sensor space.
     *
     * Only valid when transformInto == AveragePower.
     *
     * @param[in] X  Feature matrix (n_epochs × n_components).
     * @return Sensor-space projection (n_epochs × n_channels × n_components).
     *         Stored as (n_epochs, n_channels * n_components) flattened.
     */
    Eigen::MatrixXd inverseTransform(const Eigen::MatrixXd& X) const;

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
     * @return Mean band power per component (computed during fit).
     */
    const Eigen::VectorXd& mean() const;

    //=========================================================================================================
    /**
     * @return Standard deviation of band power per component (computed during fit).
     */
    const Eigen::VectorXd& stddev() const;

    //=========================================================================================================
    /**
     * @return True if the model has been fitted.
     */
    bool isFitted() const;

private:
    int m_nComponents;
    TransformMode m_transformInto;
    bool m_useLog;

    Eigen::MatrixXd m_filters;      /**< Spatial filters (n_components × n_channels). */
    Eigen::MatrixXd m_patterns;     /**< Spatial patterns (n_channels × n_components). */
    Eigen::VectorXd m_mean;         /**< Mean band power per component. */
    Eigen::VectorXd m_std;          /**< Std dev band power per component. */
    bool m_fitted = false;

    /**
     * Compute average band power features from filtered epochs.
     */
    Eigen::MatrixXd computePowerFeatures(
        const std::vector<Eigen::MatrixXd>& epochs) const;
};

} // namespace DECODINGLIB

#endif // DECODING_CSP_H
