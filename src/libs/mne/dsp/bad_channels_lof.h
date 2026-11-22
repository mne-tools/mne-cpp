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
 *
 * @snippet ex_dsp_artifacts/main.cpp find_bad_channels_lof_usage
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
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Parameters for LOF bad channel detection.
 *
 * @snippet ex_dsp_artifacts/main.cpp find_bad_channels_lof_usage
 */
struct DSPSHARED_EXPORT LofBadChannelParams
{
    int iNNeighbors = 20;    /**< Number of neighbours for LOF (k parameter). */
    double dThreshold = 2.0; /**< LOF score threshold for marking a channel as bad. */
    bool bMegOnly = false;   /**< If true, only check MEG channels. */
    bool bEegOnly = false;   /**< If true, only check EEG channels. */
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
DSPSHARED_EXPORT mne::StringList findBadChannelsLof(const Eigen::MatrixXd& data,
                                                    const mne::FiffInfo& info,
                                                    const LofBadChannelParams& params = LofBadChannelParams());

//=============================================================================================================
/**
 * @brief Local Outlier Factor of every sample, as scikit-learn's @c LocalOutlierFactor computes it.
 *
 * Breunig et al. (2000): the ratio of the neighbours' mean local reachability density to the sample's own,
 * from the @p n_neighbors nearest samples by Euclidean distance. A header template, so float features on an
 * embedded target are scored in float. Adapted from Skigen (MIT, Copyright (c) 2026 The Skigen
 * Contributors).
 *
 * @param[in] features      Feature matrix, n_samples x n_features.
 * @param[in] n_neighbors   Number of neighbours; all ones are returned unless it is below n_samples.
 *
 * @return The factor of each sample, n_samples; values well above 1 mark outliers.
 */
template<class Derived>
Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, 1> local_outlier_factor(const Eigen::MatrixBase<Derived>& features, int n_neighbors)
{
    using Scalar = typename Derived::Scalar;
    using Vector = Eigen::Matrix<Scalar, Eigen::Dynamic, 1>;
    const Eigen::Index n = features.rows();
    const Eigen::Index k = n_neighbors;
    if (k < 1 || n <= k) {
        return Vector::Ones(n);
    }

    Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> dist = Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic>::Zero(n, n);
    for (Eigen::Index i = 0; i < n; ++i) {
        for (Eigen::Index j = i + 1; j < n; ++j) {
            dist(i, j) = dist(j, i) = (features.row(i) - features.row(j)).norm();
        }
    }

    // The k nearest neighbours of each sample, itself excluded, and its k-distance
    std::vector<Eigen::Index> neighbours(static_cast<std::size_t>(n * k));
    Vector kDistance(n);
    std::vector<Eigen::Index> order(static_cast<std::size_t>(n));
    for (Eigen::Index i = 0; i < n; ++i) {
        std::iota(order.begin(), order.end(), Eigen::Index(0));
        std::sort(order.begin(), order.end(), [&dist, i](Eigen::Index a, Eigen::Index b) { return dist(i, a) < dist(i, b); });
        std::copy_n(order.begin() + 1, k, neighbours.begin() + i * k);
        kDistance(i) = dist(i, order[static_cast<std::size_t>(k)]);
    }
    const auto neighbour = [&neighbours, k](Eigen::Index i, Eigen::Index j) {
        return neighbours[static_cast<std::size_t>(i * k + j)];
    };

    // Local reachability density, then the ratio of the neighbours' densities to the sample's own
    constexpr Scalar tiny = Scalar(1e-30);
    Vector lrd(n);
    for (Eigen::Index i = 0; i < n; ++i) {
        Scalar reach = 0;
        for (Eigen::Index j = 0; j < k; ++j) {
            reach += std::max(kDistance(neighbour(i, j)), dist(i, neighbour(i, j)));
        }
        lrd(i) = reach > tiny ? static_cast<Scalar>(k) / reach : Scalar(1);
    }
    Vector lof(n);
    for (Eigen::Index i = 0; i < n; ++i) {
        Scalar neighbourLrd = 0;
        for (Eigen::Index j = 0; j < k; ++j) {
            neighbourLrd += lrd(neighbour(i, j));
        }
        lof(i) = lrd(i) > tiny ? neighbourLrd / static_cast<Scalar>(k) / lrd(i) : Scalar(1);
    }
    return lof;
}

//=============================================================================================================
/**
 * @brief Compute LOF scores for a feature matrix.
 *
 * local_outlier_factor() in double precision.
 *
 * @param[in] features   Feature matrix (n_points x n_features).
 * @param[in] k          Number of neighbours.
 *
 * @return LOF score for each point (n_points x 1). Values > 1 indicate outliers.
 */
DSPSHARED_EXPORT Eigen::VectorXd computeLofScores(const Eigen::MatrixXd& features,
                                                  int k);

} // namespace mne

#endif // BAD_CHANNELS_LOF_H
