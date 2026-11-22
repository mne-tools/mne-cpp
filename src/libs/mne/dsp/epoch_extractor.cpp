//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     epoch_extractor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of EpochExtractor.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epoch_extractor.h"

#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// PRIVATE
//=============================================================================================================

void EpochExtractor::applyBaseline(MatrixXd& matEpoch, int iBase0, int iBase1)
{
    if (iBase0 > iBase1 || iBase0 < 0 || iBase1 >= matEpoch.cols())
        return;
    const int nBaseSamp = iBase1 - iBase0 + 1;
    // Per-channel baseline mean
    VectorXd baseline = matEpoch.block(0, iBase0, matEpoch.rows(), nBaseSamp).rowwise().mean();
    matEpoch.colwise() -= baseline;
}

//=============================================================================================================
// PUBLIC
//=============================================================================================================

std::vector<MNEEpochData> EpochExtractor::extract(const MatrixXd& matData,
                                                  const std::vector<int>& eventSamples,
                                                  double dSFreq,
                                                  const Params& params,
                                                  const std::vector<int>& eventCodes)
{
    std::vector<MNEEpochData> epochs;

    if (matData.size() == 0 || eventSamples.empty())
        return epochs;
    if (dSFreq <= 0.0) {
        Log::warning("EpochExtractor::extract: invalid sampling frequency.");
        return epochs;
    }

    const int nSamp = static_cast<int>(matData.cols());
    const int nCh = static_cast<int>(matData.rows());

    // Convert time to sample offsets
    const int iOffset0 = static_cast<int>(std::round(params.dTmin * dSFreq));
    const int iOffset1 = static_cast<int>(std::round(params.dTmax * dSFreq));
    const int epochLen = iOffset1 - iOffset0 + 1;

    if (epochLen <= 0) {
        Log::warning("EpochExtractor::extract: tmax must be > tmin.");
        return epochs;
    }

    // Baseline window sample indices within the epoch (0-based)
    const int iBase0 = static_cast<int>(std::round((params.dBaseMin - params.dTmin) * dSFreq));
    const int iBase1 = static_cast<int>(std::round((params.dBaseMax - params.dTmin) * dSFreq));

    const bool bHaveCodes = (eventCodes.size() == eventSamples.size());

    for (int ev = 0; ev < std::ssize(eventSamples); ++ev) {
        const int evSamp = eventSamples[ev];
        const int s0 = evSamp + iOffset0;
        const int s1 = evSamp + iOffset1;

        // Skip if epoch extends outside the recording
        if (s0 < 0 || s1 >= nSamp)
            continue;

        MNEEpochData epoch;
        epoch.epoch = matData.block(0, s0, nCh, epochLen);
        epoch.tmin = static_cast<float>(params.dTmin);
        epoch.tmax = static_cast<float>(params.dTmax);
        epoch.event = bHaveCodes ? eventCodes[ev] : 1;
        epoch.bReject = false;

        // Baseline correction
        if (params.bApplyBaseline) {
            applyBaseline(epoch.epoch, iBase0, iBase1);
        }

        // Amplitude rejection: peak-to-peak per channel
        if (params.dThreshold > 0.0) {
            for (int ch = 0; ch < nCh; ++ch) {
                const RowVectorXd row = epoch.epoch.row(ch);
                double ptp = row.maxCoeff() - row.minCoeff();
                if (ptp > params.dThreshold) {
                    epoch.bReject = true;
                    break;
                }
            }
        }

        epochs.push_back(epoch);
    }

    return epochs;
}

//=============================================================================================================

MatrixXd EpochExtractor::average(const std::vector<MNEEpochData>& epochs)
{
    MatrixXd result;
    int nGood = 0;

    for (const MNEEpochData& ep : epochs) {
        if (ep.bReject)
            continue;
        if (result.size() == 0) {
            result = ep.epoch;
        } else {
            if (ep.epoch.rows() != result.rows() || ep.epoch.cols() != result.cols()) {
                Log::warning("EpochExtractor::average: epoch dimension mismatch, skipping.");
                continue;
            }
            result += ep.epoch;
        }
        ++nGood;
    }

    if (nGood > 1)
        result /= static_cast<double>(nGood);
    return result;
}

//=============================================================================================================

std::vector<MNEEpochData> EpochExtractor::rejectMarked(const std::vector<MNEEpochData>& epochs)
{
    std::vector<MNEEpochData> good;
    good.reserve(epochs.size());
    for (const MNEEpochData& ep : epochs) {
        if (!ep.bReject)
            good.push_back(ep);
    }
    return good;
}
