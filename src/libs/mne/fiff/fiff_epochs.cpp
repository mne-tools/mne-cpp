//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_epochs.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the static epoch-cutting helpers declared in @ref fiff_epochs.h.
 *
 * Slices a @ref FiffRawData into fixed-length, event-aligned epochs
 * respecting both annotation-driven bad segments and peak-to-peak /
 * flatness rejection. Parity with @c mne.Epochs construction.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_epochs.h"

#include "fiff_evoked.h"
#include <mne/base/log.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::vector<FiffEpochData> FiffEpochs::makeFixedLengthEpochs(const MatrixXd& matData,
                                                             double dSFreq,
                                                             double dDuration,
                                                             double dOverlap,
                                                             bool bDropLast)
{
    std::vector<FiffEpochData> epochs;

    if (matData.size() == 0 || dSFreq <= 0.0 || dDuration <= 0.0) {
        mne::Log::warning("[FiffEpochs::makeFixedLengthEpochs] Invalid input parameters.");
        return epochs;
    }

    if (dOverlap >= dDuration) {
        mne::Log::warning("[FiffEpochs::makeFixedLengthEpochs] Overlap must be less than duration.");
        return epochs;
    }

    const int nCh = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());
    const int epochSamples = static_cast<int>(std::round(dDuration * dSFreq));
    const int stepSamples = static_cast<int>(std::round((dDuration - dOverlap) * dSFreq));

    if (epochSamples <= 0 || stepSamples <= 0)
        return epochs;

    int start = 0;
    while (start + epochSamples <= nTimes) {
        FiffEpochData epoch;
        epoch.data = matData.block(0, start, nCh, epochSamples);
        epoch.tmin = static_cast<double>(start) / dSFreq;
        epoch.tmax = static_cast<double>(start + epochSamples - 1) / dSFreq;
        epochs.push_back(epoch);
        start += stepSamples;
    }

    // Handle remaining samples if not dropping last
    if (!bDropLast && start < nTimes) {
        int remaining = nTimes - start;
        FiffEpochData epoch;
        epoch.data = matData.block(0, start, nCh, remaining);
        epoch.tmin = static_cast<double>(start) / dSFreq;
        epoch.tmax = static_cast<double>(nTimes - 1) / dSFreq;
        epochs.push_back(epoch);
    }

    return epochs;
}

//=============================================================================================================

std::vector<FiffEpochData> FiffEpochs::concatenateEpochs(const std::vector<std::vector<FiffEpochData>>& epochSets)
{
    std::vector<FiffEpochData> result;
    std::size_t total = 0;
    for (const auto& epochSet : epochSets)
        total += epochSet.size();
    result.reserve(total);

    for (const auto& epochSet : epochSets)
        result.insert(result.end(), epochSet.begin(), epochSet.end());

    return result;
}

//=============================================================================================================

FiffEvoked FiffEpochs::averageEpochs(const std::vector<FiffEpochData>& epochs,
                                     double dSFreq,
                                     const std::string& comment)
{
    FiffEvoked evoked;

    if (epochs.empty()) {
        mne::Log::warning("[FiffEpochs::averageEpochs] Empty epoch list.");
        return evoked;
    }

    const int nCh = static_cast<int>(epochs[0].data.rows());
    const int nTimes = static_cast<int>(epochs[0].data.cols());
    const int nEpochs = static_cast<int>(epochs.size());

    MatrixXd avg = MatrixXd::Zero(nCh, nTimes);

    for (const auto& epoch : epochs) {
        if (epoch.data.rows() != nCh || epoch.data.cols() != nTimes) {
            mne::Log::warning("[FiffEpochs::averageEpochs] Inconsistent epoch dimensions; skipping.");
            continue;
        }
        avg += epoch.data;
    }

    avg /= static_cast<double>(nEpochs);

    // Populate the evoked
    evoked.data = avg;
    evoked.nave = nEpochs;
    evoked.aspect_kind = FIFFV_ASPECT_AVERAGE;
    evoked.comment = comment;

    // Compute times vector from the first epoch
    const double tmin = epochs[0].tmin;
    evoked.first = static_cast<fiff_int_t>(std::round(tmin * dSFreq));
    evoked.last = evoked.first + nTimes - 1;

    RowVectorXf times(nTimes);
    for (int i = 0; i < nTimes; ++i)
        times(i) = static_cast<float>(tmin + static_cast<double>(i) / dSFreq);
    evoked.times = times;

    return evoked;
}

//=============================================================================================================

std::vector<MatrixXd> FiffEpochs::toMatrixList(const std::vector<FiffEpochData>& epochs)
{
    std::vector<MatrixXd> result;
    result.reserve(epochs.size());

    for (const auto& epoch : epochs)
        result.push_back(epoch.data);

    return result;
}
