//=============================================================================================================
/**
 * @file     fiff_epochs.cpp
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
 * @brief    FiffEpochs class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_epochs.h"

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

MatrixXd FiffEpochs::averageEpochs(const QList<FiffEpochData>& epochs)
{
    if (epochs.isEmpty()) {
        qWarning() << "[FiffEpochs::averageEpochs] Empty epoch list.";
        return MatrixXd();
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
    return avg;
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
