//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bridged_electrodes.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
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
