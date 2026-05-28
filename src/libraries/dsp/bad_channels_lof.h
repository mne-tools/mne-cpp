//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bad_channels_lof.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Bad-channel detection via the Local Outlier Factor (LOF) algorithm.
 *
 * LOF (Breunig et al., SIGMOD 2000) scores each channel by the ratio between
 * its own local density (reachability distance to its @c k nearest channels
 * in the chosen feature space) and the average local density of those
 * neighbours. Values close to 1 indicate a sample that belongs to a dense
 * cluster, values noticeably above 1 mark density outliers. For MEG / EEG
 * each channel is summarised by per-segment statistics (RMS, variance,
 * higher-order moments) so that genuinely flat, very noisy or amplifier-
 * saturated sensors stand out in the LOF score and can be flagged at a
 * single threshold (default 2.0).
 *
 * The detector follows MNE-Python's @c mne.preprocessing.find_bad_channels_lof
 * so MEG / EEG pipelines can switch between the two without changing the
 * downstream interpolation / SSS step.
 */

#ifndef BAD_CHANNELS_LOF_H
#define BAD_CHANNELS_LOF_H

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

#include <QStringList>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * Parameters for LOF bad channel detection.
 */
struct DSPSHARED_EXPORT LofBadChannelParams
{
    int iNNeighbors = 20;       /**< Number of neighbours for LOF (k parameter). */
    double dThreshold = 2.0;    /**< LOF score threshold for marking a channel as bad. */
    bool bMegOnly = false;      /**< If true, only check MEG channels. */
    bool bEegOnly = false;      /**< If true, only check EEG channels. */
};

//=============================================================================================================
/**
 * @brief Detect bad channels using Local Outlier Factor (LOF).
 *
 * Computes features for each channel (standard deviation, kurtosis,
 * max absolute amplitude) and applies the LOF algorithm to identify
 * outlier channels. LOF compares the local density of a point's
 * neighbourhood to the densities of its neighbours.
 *
 * @param[in] data      Data matrix (n_channels x n_times).
 * @param[in] info      FiffInfo with channel types.
 * @param[in] params    LOF parameters.
 *
 * @return List of bad channel names.
 */
DSPSHARED_EXPORT QStringList findBadChannelsLof(const Eigen::MatrixXd& data,
                                                 const FIFFLIB::FiffInfo& info,
                                                 const LofBadChannelParams& params = LofBadChannelParams());

//=============================================================================================================
/**
 * @brief Compute LOF scores for a feature matrix.
 *
 * Pure algorithmic LOF implementation operating on feature vectors.
 *
 * @param[in] features   Feature matrix (n_points x n_features).
 * @param[in] k          Number of neighbours.
 *
 * @return LOF score for each point (n_points x 1). Values > 1 indicate outliers.
 */
DSPSHARED_EXPORT Eigen::VectorXd computeLofScores(const Eigen::MatrixXd& features,
                                                    int k);

} // namespace UTILSLIB

#endif // BAD_CHANNELS_LOF_H
