//=============================================================================================================
/**
 * @file     bad_channels_lof.cpp
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
 * @brief    LOF-based bad channel detection implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bad_channels_lof.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// SKIGEN INCLUDES
//=============================================================================================================

#include <Skigen/Neighbors>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
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

VectorXd UTILSLIB::computeLofScores(const MatrixXd& features, int k)
{
    const int n = static_cast<int>(features.rows());

    if (n <= k) {
        return VectorXd::Ones(n);
    }

    // features is (n_samples × n_features), matches skigen convention
    Skigen::LocalOutlierFactor<double> lof(k);
    lof.fit(features);
    return lof.lof_scores();
}

//=============================================================================================================

QStringList UTILSLIB::findBadChannelsLof(const MatrixXd& data,
                                          const FiffInfo& info,
                                          const LofBadChannelParams& params)
{
    // Select channel indices based on type filter
    QList<int> chIdx;
    for (int i = 0; i < info.chs.size(); ++i) {
        if (params.bMegOnly && info.chs[i].kind != FIFFV_MEG_CH)
            continue;
        if (params.bEegOnly && info.chs[i].kind != FIFFV_EEG_CH)
            continue;
        if (!params.bMegOnly && !params.bEegOnly) {
            if (info.chs[i].kind != FIFFV_MEG_CH && info.chs[i].kind != FIFFV_EEG_CH)
                continue;
        }
        chIdx.append(i);
    }

    const int nCh = chIdx.size();
    if (nCh < 3) {
        return QStringList();
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
    QStringList badChannels;
    for (int i = 0; i < nCh; ++i) {
        if (lof(i) > params.dThreshold) {
            badChannels.append(info.ch_names[chIdx[i]]);
        }
    }

    return badChannels;
}
