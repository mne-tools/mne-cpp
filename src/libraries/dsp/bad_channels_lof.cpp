//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bad_channels_lof.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    LOF-based bad channel detection implementation.
 *
 * The Local Outlier Factor follows scikit-learn's @c LocalOutlierFactor and was adapted from Skigen (MIT,
 * Copyright (c) 2026 The Skigen Contributors).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bad_channels_lof.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

VectorXd mne::computeLofScores(const MatrixXd& features, int k)
{
    const int n = static_cast<int>(features.rows());

    if (n <= k) {
        return VectorXd::Ones(n);
    }

    // Breunig et al. (2000), as in scikit-learn: features is (n_samples × n_features)
    MatrixXd dist = MatrixXd::Zero(n, n);
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            dist(i, j) = dist(j, i) = (features.row(i) - features.row(j)).norm();
        }
    }

    // The k nearest neighbours of each sample, itself excluded, and its k-distance
    std::vector<std::vector<int>> neighbours(static_cast<std::size_t>(n));
    VectorXd kDistance(n);
    std::vector<int> order(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&dist, i](int a, int b) { return dist(i, a) < dist(i, b); });
        neighbours[static_cast<std::size_t>(i)].assign(order.begin() + 1, order.begin() + 1 + k);
        kDistance(i) = dist(i, order[static_cast<std::size_t>(k)]);
    }

    // Local reachability density, then the ratio of the neighbours' densities to the sample's own
    constexpr double kTiny = 1e-30;
    VectorXd lrd(n);
    for (int i = 0; i < n; ++i) {
        double reach = 0.0;
        for (const int nb : neighbours[static_cast<std::size_t>(i)]) {
            reach += std::max(kDistance(nb), dist(i, nb));
        }
        lrd(i) = reach > kTiny ? k / reach : 1.0;
    }
    VectorXd lof(n);
    for (int i = 0; i < n; ++i) {
        double neighbourLrd = 0.0;
        for (const int nb : neighbours[static_cast<std::size_t>(i)]) {
            neighbourLrd += lrd(nb);
        }
        lof(i) = lrd(i) > kTiny ? neighbourLrd / k / lrd(i) : 1.0;
    }
    return lof;
}

//=============================================================================================================

mne::StringList mne::findBadChannelsLof(const MatrixXd& data,
                                        const FiffInfo& info,
                                        const LofBadChannelParams& params)
{
    // Select channel indices based on type filter
    std::vector<int> chIdx;
    for (int i = 0; i < std::ssize(info.chs); ++i) {
        if (params.bMegOnly && info.chs[i].kind != FIFFV_MEG_CH)
            continue;
        if (params.bEegOnly && info.chs[i].kind != FIFFV_EEG_CH)
            continue;
        if (!params.bMegOnly && !params.bEegOnly) {
            if (info.chs[i].kind != FIFFV_MEG_CH && info.chs[i].kind != FIFFV_EEG_CH)
                continue;
        }
        chIdx.push_back(i);
    }

    const int nCh = static_cast<int>(chIdx.size());
    if (nCh < 3) {
        return StringList();
    }

    // Compute features: [std_dev, kurtosis, max_abs]
    const int nFeatures = 3;
    MatrixXd features(nCh, nFeatures);

    for (int i = 0; i < nCh; ++i) {
        const RowVectorXd row = data.row(chIdx[i]);
        const int nTimes = static_cast<int>(row.size());
        const double mean = row.mean();

        // Standard deviation
        double variance = (row.array() - mean).square().sum() / static_cast<double>(nTimes);
        double stdDev = std::sqrt(variance);
        features(i, 0) = stdDev;

        // Kurtosis (excess)
        double m4 = (row.array() - mean).pow(4.0).sum() / static_cast<double>(nTimes);
        double kurt = (variance > 1e-30) ? m4 / (variance * variance) - 3.0 : 0.0;
        features(i, 1) = kurt;

        // Max absolute value
        features(i, 2) = row.array().abs().maxCoeff();
    }

    // Normalise features to zero mean, unit variance
    for (int f = 0; f < nFeatures; ++f) {
        double fMean = features.col(f).mean();
        double fStd = std::sqrt((features.col(f).array() - fMean).square().mean());
        if (fStd > 1e-30) {
            features.col(f) = (features.col(f).array() - fMean) / fStd;
        }
    }

    // Compute LOF scores
    int k = std::min(params.iNNeighbors, nCh - 1);
    VectorXd lof = computeLofScores(features, k);

    // Flag channels above threshold
    StringList badChannels;
    for (int i = 0; i < nCh; ++i) {
        if (lof(i) > params.dThreshold) {
            badChannels.push_back(info.ch_names[chIdx[i]]);
        }
    }

    return badChannels;
}
