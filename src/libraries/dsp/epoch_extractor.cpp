//=============================================================================================================
/**
 * @file     epoch_extractor.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
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
 * @brief    Implementation of EpochExtractor.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epoch_extractor.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE
//=============================================================================================================

void EpochExtractor::applyBaseline(MatrixXd& matEpoch, int iBase0, int iBase1)
{
    if (iBase0 > iBase1 || iBase0 < 0 || iBase1 >= matEpoch.cols()) return;
    const int nBaseSamp = iBase1 - iBase0 + 1;
    // Per-channel baseline mean
    VectorXd baseline = matEpoch.block(0, iBase0, matEpoch.rows(), nBaseSamp).rowwise().mean();
    matEpoch.colwise() -= baseline;
}

//=============================================================================================================
// PUBLIC
//=============================================================================================================

QVector<MNEEpochData> EpochExtractor::extract(const MatrixXd&     matData,
                                               const QVector<int>& eventSamples,
                                               double              dSFreq,
                                               const Params&       params,
                                               const QVector<int>& eventCodes)
{
    QVector<MNEEpochData> epochs;

    if (matData.size() == 0 || eventSamples.isEmpty()) return epochs;
    if (dSFreq <= 0.0) {
        qWarning() << "EpochExtractor::extract: invalid sampling frequency.";
        return epochs;
    }

    const int nSamp     = static_cast<int>(matData.cols());
    const int nCh       = static_cast<int>(matData.rows());

    // Convert time to sample offsets
    const int iOffset0  = static_cast<int>(std::round(params.dTmin * dSFreq));
    const int iOffset1  = static_cast<int>(std::round(params.dTmax * dSFreq));
    const int epochLen  = iOffset1 - iOffset0 + 1;

    if (epochLen <= 0) {
        qWarning() << "EpochExtractor::extract: tmax must be > tmin.";
        return epochs;
    }

    // Baseline window sample indices within the epoch (0-based)
    const int iBase0 = static_cast<int>(std::round((params.dBaseMin - params.dTmin) * dSFreq));
    const int iBase1 = static_cast<int>(std::round((params.dBaseMax - params.dTmin) * dSFreq));

    const bool bHaveCodes = (eventCodes.size() == eventSamples.size());

    for (int ev = 0; ev < eventSamples.size(); ++ev) {
        const int evSamp = eventSamples[ev];
        const int s0     = evSamp + iOffset0;
        const int s1     = evSamp + iOffset1;

        // Skip if epoch extends outside the recording
        if (s0 < 0 || s1 >= nSamp) continue;

        MNEEpochData epoch;
        epoch.epoch  = matData.block(0, s0, nCh, epochLen);
        epoch.tmin   = static_cast<float>(params.dTmin);
        epoch.tmax   = static_cast<float>(params.dTmax);
        epoch.event  = bHaveCodes ? eventCodes[ev] : 1;
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

        epochs.append(epoch);
    }

    return epochs;
}

//=============================================================================================================

MatrixXd EpochExtractor::average(const QVector<MNEEpochData>& epochs)
{
    MatrixXd result;
    int nGood = 0;

    for (const MNEEpochData& ep : epochs) {
        if (ep.bReject) continue;
        if (result.size() == 0) {
            result = ep.epoch;
        } else {
            if (ep.epoch.rows() != result.rows() || ep.epoch.cols() != result.cols()) {
                qWarning() << "EpochExtractor::average: epoch dimension mismatch, skipping.";
                continue;
            }
            result += ep.epoch;
        }
        ++nGood;
    }

    if (nGood > 1) result /= static_cast<double>(nGood);
    return result;
}

//=============================================================================================================

QVector<MNEEpochData> EpochExtractor::rejectMarked(const QVector<MNEEpochData>& epochs)
{
    QVector<MNEEpochData> good;
    good.reserve(epochs.size());
    for (const MNEEpochData& ep : epochs) {
        if (!ep.bReject) good.append(ep);
    }
    return good;
}
