//=============================================================================================================
/**
 * @file     bridged_electrodes.cpp
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
 * @brief    Bridged electrode detection implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bridged_electrodes.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

MatrixXd UTILSLIB::computeElectricalDistance(const MatrixXd& data,
                                              const FiffInfo& info)
{
    // Find EEG channel indices
    QList<int> eegIdx;
    for (int i = 0; i < info.chs.size(); ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH) {
            eegIdx.append(i);
        }
    }

    const int nEeg = eegIdx.size();
    if (nEeg < 2) {
        qWarning() << "[computeElectricalDistance] Need at least 2 EEG channels, found" << nEeg;
        return MatrixXd::Zero(nEeg, nEeg);
    }

    // Compute per-channel standard deviations
    VectorXd stdDevs(nEeg);
    for (int i = 0; i < nEeg; ++i) {
        const RowVectorXd row = data.row(eegIdx[i]);
        double mean = row.mean();
        double variance = (row.array() - mean).square().mean();
        stdDevs(i) = std::sqrt(variance);
    }

    // Compute electrical distance matrix
    MatrixXd edist = MatrixXd::Zero(nEeg, nEeg);
    const int nTimes = static_cast<int>(data.cols());

    for (int i = 0; i < nEeg; ++i) {
        for (int j = i + 1; j < nEeg; ++j) {
            // Standard deviation of the difference signal
            RowVectorXd diff = data.row(eegIdx[i]) - data.row(eegIdx[j]);
            double meanDiff = diff.mean();
            double varDiff = (diff.array() - meanDiff).square().mean();
            double stdDiff = std::sqrt(varDiff);

            // Normalise by geometric mean of individual std devs
            double geoMean = std::sqrt(stdDevs(i) * stdDevs(j));
            double ed = (geoMean > 1e-30) ? stdDiff / geoMean : 0.0;

            edist(i, j) = ed;
            edist(j, i) = ed;
        }
    }

    return edist;
}

//=============================================================================================================

QList<QPair<int,int>> UTILSLIB::computeBridgedElectrodes(
    const MatrixXd& data,
    const FiffInfo& info,
    const BridgedElectrodeParams& params)
{
    // Find EEG channel indices
    QList<int> eegIdx;
    for (int i = 0; i < info.chs.size(); ++i) {
        if (info.chs[i].kind == FIFFV_EEG_CH) {
            eegIdx.append(i);
        }
    }

    const int nEeg = eegIdx.size();
    QList<QPair<int,int>> bridged;

    if (nEeg < 2) {
        return bridged;
    }

    MatrixXd edist = computeElectricalDistance(data, info);

    // Find pairs below threshold
    for (int i = 0; i < nEeg; ++i) {
        for (int j = i + 1; j < nEeg; ++j) {
            if (edist(i, j) < params.dElectricalDistanceThreshold) {
                bridged.append(qMakePair(eegIdx[i], eegIdx[j]));
            }
        }
    }

    return bridged;
}
