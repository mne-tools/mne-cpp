//=============================================================================================================
/**
 * @file     inv_convenience.h
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
 * @brief    Inverse convenience functions matching MNE-Python's apply_inverse_* API.
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
