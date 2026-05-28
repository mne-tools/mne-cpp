//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bad_channels_maxwell.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    SSS-based bad channel detection implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bad_channels_maxwell.h"
#include "sss.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>
#include <numeric>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

BadChannelsMaxwellResult UTILSLIB::findBadChannelsMaxwell(
    const MatrixXd& matData,
    const FiffInfo& info,
    const BadChannelsMaxwellParams& params)
{
    BadChannelsMaxwellResult result;

    // Build SSS basis
    SSSParams sssParams;
    sssParams.iOrderIn  = params.iOrderIn;
    sssParams.iOrderOut = params.iOrderOut;
    sssParams.origin    = params.origin;

    SSS::Basis basis = SSS::computeBasis(info, sssParams);

    if (basis.megChannelIdx.isEmpty()) {
        qWarning() << "[findBadChannelsMaxwell] No MEG channels found.";
        return result;
    }

    const int nMeg = basis.megChannelIdx.size();

    // Extract MEG data
    int nTimes = static_cast<int>(matData.cols());
    MatrixXd megData(nMeg, nTimes);
    for (int i = 0; i < nMeg; ++i) {
        megData.row(i) = matData.row(basis.megChannelIdx[i]);
    }

    // Apply SSS reconstruction
    MatrixXd reconData = SSS::apply(matData, basis);

    // Extract reconstructed MEG channels
    MatrixXd megRecon(nMeg, nTimes);
    for (int i = 0; i < nMeg; ++i) {
        megRecon.row(i) = reconData.row(basis.megChannelIdx[i]);
    }

    // Compute per-channel residual (RMS of difference)
    result.residuals.resize(nMeg);
    for (int i = 0; i < nMeg; ++i) {
        VectorXd diff = megData.row(i) - megRecon.row(i);
        result.residuals(i) = std::sqrt(diff.squaredNorm() / nTimes);
    }

    // Compute z-scores
    // Use median/MAD for robustness
    VectorXd sorted = result.residuals;
    std::sort(sorted.data(), sorted.data() + sorted.size());

    double median = sorted(nMeg / 2);
    if (nMeg % 2 == 0 && nMeg > 1) {
        median = (sorted(nMeg / 2 - 1) + sorted(nMeg / 2)) / 2.0;
    }

    VectorXd absDev(nMeg);
    for (int i = 0; i < nMeg; ++i) {
        absDev(i) = std::abs(result.residuals(i) - median);
    }
    std::sort(absDev.data(), absDev.data() + absDev.size());

    double mad = absDev(nMeg / 2);
    if (nMeg % 2 == 0 && nMeg > 1) {
        mad = (absDev(nMeg / 2 - 1) + absDev(nMeg / 2)) / 2.0;
    }

    // Robust scale: MAD * 1.4826 (consistency constant for normal distribution)
    double scale = mad * 1.4826;
    if (scale < 1e-30) scale = 1e-30;

    result.zScores.resize(nMeg);
    for (int i = 0; i < nMeg; ++i) {
        result.zScores(i) = (result.residuals(i) - median) / scale;
    }

    // Flag channels exceeding threshold
    for (int i = 0; i < nMeg; ++i) {
        if (result.zScores(i) > params.dZThreshold) {
            int origIdx = basis.megChannelIdx[i];
            result.badIndices.append(origIdx);
            if (origIdx < info.ch_names.size()) {
                result.badChannels.append(info.ch_names[origIdx]);
            }
        }
    }

qInfo("[findBadChannelsMaxwell] Detected %lld bad channel(s) out of %d MEG channels.",
           static_cast<long long>(result.badChannels.size()), nMeg);

    return result;
}
