//=============================================================================================================
/**
 * @file     bad_channels_maxwell.cpp
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
