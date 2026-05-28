//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_spoc.h
 * @since May 2026
 * @brief Source Power Comodulation (SPoC) for regressing continuous targets onto narrowband M/EEG power.
 *
 * SPoC is the regression analogue of CSP: instead of two class
 * covariances it solves the generalised eigenproblem
 * @f$\Sigma_z w = \lambda \Sigma w@f$ where @f$\Sigma@f$ is the
 * trial-averaged covariance of band-passed epochs and @f$\Sigma_z@f$ is
 * the same covariance weighted (and centred) by the per-trial target
 * variable @f$z@f$. The leading eigenvector therefore extracts a
 * spatial filter whose epoch-wise band-power envelope maximally
 * covaries with the external regressor, which can be a behavioural
 * score, a stimulus parameter, a haemodynamic signal recorded in
 * parallel, or any other continuous label. The method was introduced
 * by Dähne, Meinecke, Haufe, Höhne, Tangermann, Müller & Nikulin,
 * *SPoC: a novel framework for relating the amplitude of neuronal
 * oscillations to behaviorally relevant parameters*, NeuroImage 86,
 * 2014.
 *
 * @ref DecodingSpoc mirrors @c mne.decoding.SPoC and reuses the same
 * @c fit / @c transform / @c fitTransform scikit-learn pattern as
 * @ref DecodingCsp, including the @c AveragePower vs @c CspSpace
 * @c TransformMode switch and the optional log / z-score normalisation
 * of the band-power features. Inputs are expected to be already
 * band-passed (SPoC has no spectral component of its own) and the
 * target vector must be aligned one-to-one with the epoch list.
 */

#ifndef DECODING_SPOC_H
#define DECODING_SPOC_H

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
 * @brief Source Power Comodulation decoder for continuous-target regression on band-power.
 *
 * Estimates a bank of spatial filters whose log band-power envelope is
 * maximally correlated with a continuous trial-level target. The fit
 * builds the unweighted trial-mean covariance and a target-weighted
 * covariance from centred labels, solves the resulting generalised
 * eigenvalue problem, and keeps the top @c n_components eigenvectors as
 * filters and the columns of their pseudoinverse as the corresponding
 * activation patterns. As with CSP, the patterns — not the filters —
 * are the quantity that should be plotted as topographies when
 * interpreting which cortical sources drive the regression, in line
 * with Haufe et al. 2014.
 *
 * After @ref fit the decoder behaves as a deterministic feature
 * extractor. @c AveragePower returns one (optionally log- or
 * z-scored) band-power value per component and epoch, which plugs
 * directly into a linear regressor; @c CspSpace returns the time-
 * resolved filtered signal, useful when the downstream model consumes
 * envelopes or instantaneous phase. Inputs must be epoched and already
 * filtered in the relevant frequency band; the target vector @f$z@f$
 * is a real-valued vector of length @c n_epochs.
 *
 * @see DECODINGLIB, @c mne.decoding.SPoC
 */
class DECODINGSHARED_EXPORT DecodingSpoc
{
public:
    //=========================================================================================================
    /**
     * Transform mode for the SPoC output.
     */
    enum class TransformMode {
        AveragePower,   /**< Return average band power per component. */
        CspSpace        /**< Return data projected into SPoC space. */
    };

    //=========================================================================================================
    /**
     * Constructs a SPoC decoder.
     *
     * @param[in] nComponents     Number of components. Default: 4.
     * @param[in] transformInto   Feature extraction mode. Default: AveragePower.
     * @param[in] useLog          If true and transformInto == AveragePower, apply log transform.
     */
    explicit DecodingSpoc(int nComponents = 4,
                          TransformMode transformInto = TransformMode::AveragePower,
                          bool useLog = true);

    //=========================================================================================================
    /**
     * Fit SPoC from epoch data and a continuous target variable.
     *
     * @param[in] epochs  Vector of epoch matrices, each (n_channels × n_times).
     * @param[in] y       Continuous target variable (one value per epoch).
     */
    void fit(const std::vector<Eigen::MatrixXd>& epochs,
             const Eigen::VectorXd& y);

    //=========================================================================================================
    /**
     * Transform epoch data using the fitted SPoC filters.
     *
     * @param[in] epochs  Vector of epoch matrices.
     * @return Feature matrix.
     */
    Eigen::MatrixXd transform(const std::vector<Eigen::MatrixXd>& epochs) const;

    //=========================================================================================================
    /**
     * Fit and transform in one step.
     */
    Eigen::MatrixXd fitTransform(const std::vector<Eigen::MatrixXd>& epochs,
                                 const Eigen::VectorXd& y);

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
     * @return Standard deviation of band power per component.
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

    Eigen::MatrixXd m_filters;
    Eigen::MatrixXd m_patterns;
    Eigen::VectorXd m_mean;
    Eigen::VectorXd m_std;
    bool m_fitted = false;

    Eigen::MatrixXd computePowerFeatures(
        const std::vector<Eigen::MatrixXd>& epochs) const;
};

} // namespace DECODINGLIB

#endif // DECODING_SPOC_H
