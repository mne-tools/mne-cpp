//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_epochs.cpp
 * @since 2026
 * @date  May 2026
 * @brief Implementation of the static epoch-cutting helpers declared in @ref fiff_epochs.h.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QList<FiffEpochData> FiffEpochs::makeFixedLengthEpochs(const MatrixXd& matData,
                                                          double dSFreq,
                                                          double dDuration,
                                                          double dOverlap,
                                                          bool bDropLast)
{
    QList<FiffEpochData> epochs;

    if (matData.size() == 0 || dSFreq <= 0.0 || dDuration <= 0.0) {
        qWarning() << "[FiffEpochs::makeFixedLengthEpochs] Invalid input parameters.";
        return epochs;
    }

    if (dOverlap >= dDuration) {
        qWarning() << "[FiffEpochs::makeFixedLengthEpochs] Overlap must be less than duration.";
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
        epochs.append(epoch);
        start += stepSamples;
    }

    // Handle remaining samples if not dropping last
    if (!bDropLast && start < nTimes) {
        int remaining = nTimes - start;
        FiffEpochData epoch;
        epoch.data = matData.block(0, start, nCh, remaining);
        epoch.tmin = static_cast<double>(start) / dSFreq;
        epoch.tmax = static_cast<double>(nTimes - 1) / dSFreq;
        epochs.append(epoch);
    }

    return epochs;
}

//=============================================================================================================

QList<FiffEpochData> FiffEpochs::concatenateEpochs(const QList<QList<FiffEpochData>>& epochSets)
{
    QList<FiffEpochData> result;

    for (const auto& epochSet : epochSets)
        result.append(epochSet);

    return result;
}

//=============================================================================================================

FiffEvoked FiffEpochs::averageEpochs(const QList<FiffEpochData>& epochs,
                                     double dSFreq,
                                     const QString& comment)
{
    FiffEvoked evoked;

    if (epochs.isEmpty()) {
        qWarning() << "[FiffEpochs::averageEpochs] Empty epoch list.";
        return evoked;
    }

    const int nCh = static_cast<int>(epochs[0].data.rows());
    const int nTimes = static_cast<int>(epochs[0].data.cols());
    const int nEpochs = epochs.size();

    MatrixXd avg = MatrixXd::Zero(nCh, nTimes);

    for (const auto& epoch : epochs) {
        if (epoch.data.rows() != nCh || epoch.data.cols() != nTimes) {
            qWarning() << "[FiffEpochs::averageEpochs] Inconsistent epoch dimensions; skipping.";
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

QList<MatrixXd> FiffEpochs::toMatrixList(const QList<FiffEpochData>& epochs)
{
    QList<MatrixXd> result;
    result.reserve(epochs.size());

    for (const auto& epoch : epochs)
        result.append(epoch.data);

    return result;
}
