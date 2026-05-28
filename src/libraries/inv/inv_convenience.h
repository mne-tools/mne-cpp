//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_convenience.h
 * @since 2026
 * @date  May 2026
 * @brief Top-level convenience entry points that mirror MNE-Python's @c apply_inverse_* / @c compute_source_psd helpers.
 *
 * Free functions in this header glue together the lower-level INVLIB
 * classes so users can match the MNE-Python ergonomics:
 * @ref INVLIB::applyInverseEpochs applies a precomputed inverse operator
 * across an epoch list, @ref applyInverseRaw streams raw data through
 * the kernel in blocks, @ref estimateSnr returns the source-space SNR
 * trace from an evoked + inverse pair, @ref computeWhitener produces the
 * diagonal whitener from a noise covariance, and
 * @ref computeSourcePsd / @ref computeSourceBandPower run Welch-based
 * spectral analysis directly on @ref InvSourceEstimate output. All
 * methods are headers-only orchestration on top of @ref InvMinimumNorm
 * and the underlying FIFF / MNE primitives.
 */

#ifndef INV_CONVENIENCE_H
#define INV_CONVENIENCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include "inv_source_estimate.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QString>
#include <QPair>
#include <QMap>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEInverseOperator;
}
namespace FIFFLIB {
    class FiffEvoked;
    class FiffRawData;
    class FiffCov;
}

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB {

//=============================================================================================================
/**
 * @brief Apply inverse operator to each epoch in a list.
 *
 * For each epoch (data matrix), applies the minimum norm inverse and
 * returns a list of source estimates. This is the equivalent of
 * MNE-Python's mne.minimum_norm.apply_inverse_epochs().
 *
 * @param[in] epochs    List of epoch data matrices (n_channels x n_times each).
 * @param[in] inverse   Pre-computed inverse operator.
 * @param[in] lambda2   Regularization parameter (1/SNR^2).
 * @param[in] method    "MNE", "dSPM", or "sLORETA".
 * @param[in] tmin      Start time of each epoch (seconds).
 * @param[in] tstep     Time step (1/sfreq).
 * @param[in] pickNormal  If true, keep only the normal component.
 *
 * @return List of source estimates, one per epoch.
 */
INVSHARED_EXPORT QList<InvSourceEstimate> applyInverseEpochs(
    const QList<Eigen::MatrixXd>& epochs,
    const MNELIB::MNEInverseOperator& inverse,
    float lambda2,
    const QString& method = "dSPM",
    float tmin = 0.0f,
    float tstep = 0.001f,
    bool pickNormal = false);

//=============================================================================================================
/**
 * @brief Apply inverse operator to raw data in blocks.
 *
 * Reads and processes the raw data in chunks, applying the inverse
 * operator to produce a continuous source estimate. Equivalent of
 * MNE-Python's mne.minimum_norm.apply_inverse_raw().
 *
 * @param[in] raw       Raw data object.
 * @param[in] inverse   Pre-computed inverse operator.
 * @param[in] lambda2   Regularization parameter (1/SNR^2).
 * @param[in] method    "MNE", "dSPM", or "sLORETA".
 * @param[in] from      First sample to process (default: first_samp).
 * @param[in] to        Last sample to process (default: last_samp).
 * @param[in] pickNormal  If true, keep only the normal component.
 *
 * @return Source estimate spanning the requested time range.
 */
INVSHARED_EXPORT InvSourceEstimate applyInverseRaw(
    const FIFFLIB::FiffRawData& raw,
    const MNELIB::MNEInverseOperator& inverse,
    float lambda2,
    const QString& method = "dSPM",
    int from = -1,
    int to = -1,
    bool pickNormal = false);

//=============================================================================================================
/**
 * @brief Estimate SNR from evoked data and inverse operator.
 *
 * Computes source-space SNR estimate using the GFP (Global Field Power)
 * of the evoked data and the noise normalization from the inverse operator.
 *
 * @param[in] evoked    Evoked data.
 * @param[in] inverse   Inverse operator.
 * @param[in] method    "MNE", "dSPM", or "sLORETA".
 *
 * @return Pair of (SNR time course, times vector).
 */
INVSHARED_EXPORT QPair<Eigen::VectorXd, Eigen::RowVectorXf> estimateSnr(
    const FIFFLIB::FiffEvoked& evoked,
    const MNELIB::MNEInverseOperator& inverse,
    const QString& method = "dSPM");

//=============================================================================================================
/**
 * @brief Compute whitening matrix from a noise covariance.
 *
 * Returns a whitening matrix W such that W * data has identity covariance.
 * Matches MNE-Python's mne.cov.compute_whitener().
 *
 * @param[in] noiseCov  Noise covariance matrix.
 * @param[in] rank      Desired rank (0 = auto from eigenvalues).
 *
 * @return Pair of (whitener matrix, effective rank).
 */
INVSHARED_EXPORT QPair<Eigen::MatrixXd, int> computeWhitener(
    const FIFFLIB::FiffCov& noiseCov,
    int rank = 0);

//=============================================================================================================
/**
 * @brief Compute PSD for a source estimate using Welch's method.
 *
 * Takes a source estimate (n_sources x n_times) and computes the power
 * spectral density at each source vertex. Equivalent to
 * MNE-Python's mne.minimum_norm.compute_source_psd().
 *
 * @param[in] stc       Source estimate.
 * @param[in] sfreq     Sampling frequency (Hz).
 * @param[in] fmin      Minimum frequency of interest (Hz).
 * @param[in] fmax      Maximum frequency of interest (Hz).
 * @param[in] nFft      FFT length (0 = use data length).
 *
 * @return Pair of (PSD matrix [n_sources x n_freqs], frequency vector).
 */
INVSHARED_EXPORT QPair<Eigen::MatrixXd, Eigen::VectorXd> computeSourcePsd(
    const InvSourceEstimate& stc,
    float sfreq,
    float fmin = 0.0f,
    float fmax = -1.0f,
    int nFft = 0);

//=============================================================================================================
/**
 * @brief Compute band power for source estimate.
 *
 * Integrates source PSD within specified frequency bands.
 *
 * @param[in] stc       Source estimate.
 * @param[in] sfreq     Sampling frequency (Hz).
 * @param[in] bands     Map of band name -> (fmin, fmax) pairs.
 *
 * @return Map of band name -> power vector (n_sources).
 */
INVSHARED_EXPORT QMap<QString, Eigen::VectorXd> computeSourceBandPower(
    const InvSourceEstimate& stc,
    float sfreq,
    const QMap<QString, QPair<float, float>>& bands);

} // namespace INVLIB

#endif // INV_CONVENIENCE_H
