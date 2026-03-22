//=============================================================================================================
/**
 * @file     bad_channel_detect.cpp
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
 * @brief    Implementation of BadChannelDetect.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bad_channel_detect.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE
//=============================================================================================================

double BadChannelDetect::pearsonCorr(const RowVectorXd& a, const RowVectorXd& b)
{
    if (a.size() != b.size() || a.size() == 0) return 0.0;

    RowVectorXd ac = a.array() - a.mean();
    RowVectorXd bc = b.array() - b.mean();

    double normA = ac.norm();
    double normB = bc.norm();
    if (normA < 1e-30 || normB < 1e-30) return 0.0;

    return ac.dot(bc) / (normA * normB);
}

//=============================================================================================================

double BadChannelDetect::median(QVector<double> values)
{
    if (values.isEmpty()) return 0.0;
    std::sort(values.begin(), values.end());
    const int n = values.size();
    if (n % 2 == 1) return values[n / 2];
    return 0.5 * (values[n / 2 - 1] + values[n / 2]);
}

//=============================================================================================================
// PUBLIC
//=============================================================================================================

QVector<int> BadChannelDetect::detect(const MatrixXd& matData,
                                       const Params&   params)
{
    QVector<int> flat  = detectFlat(matData, params.dFlatThreshold);
    QVector<int> noisy = detectHighVariance(matData, params.dVarZThresh);
    QVector<int> low   = detectLowCorrelation(matData, params.dCorrThresh, params.iNeighbours);

    // Union without duplicates
    QVector<bool> flagged(static_cast<int>(matData.rows()), false);
    for (int i : flat)  if (i >= 0 && i < matData.rows()) flagged[i] = true;
    for (int i : noisy) if (i >= 0 && i < matData.rows()) flagged[i] = true;
    for (int i : low)   if (i >= 0 && i < matData.rows()) flagged[i] = true;

    QVector<int> result;
    for (int i = 0; i < static_cast<int>(matData.rows()); ++i) {
        if (flagged[i]) result.append(i);
    }
    return result;
}

//=============================================================================================================

QVector<int> BadChannelDetect::detectFlat(const MatrixXd& matData,
                                           double          dThreshold)
{
    QVector<int> bad;
    for (int ch = 0; ch < matData.rows(); ++ch) {
        const RowVectorXd row = matData.row(ch);
        double ptp = row.maxCoeff() - row.minCoeff();
        if (ptp < dThreshold) bad.append(ch);
    }
    return bad;
}

//=============================================================================================================

QVector<int> BadChannelDetect::detectHighVariance(const MatrixXd& matData,
                                                   double          dZThresh)
{
    const int nCh = static_cast<int>(matData.rows());
    if (nCh < 3) return {};  // Cannot compute meaningful statistics with < 3 channels

    // Compute per-channel standard deviation
    QVector<double> stds(nCh);
    for (int ch = 0; ch < nCh; ++ch) {
        const RowVectorXd row = matData.row(ch);
        double mean = row.mean();
        double var  = (row.array() - mean).square().mean();
        stds[ch]    = std::sqrt(var);
    }

    // Median and MAD of std across channels
    double med = median(stds);

    QVector<double> absDevs(nCh);
    for (int ch = 0; ch < nCh; ++ch)
        absDevs[ch] = std::abs(stds[ch] - med);
    double mad = median(absDevs);

    // Consistency factor: MAD → σ for Gaussian = 1.4826
    const double sigma = 1.4826 * mad;

    if (sigma < 1e-30) return {};  // All channels identical (e.g. all zeros)

    QVector<int> bad;
    for (int ch = 0; ch < nCh; ++ch) {
        double z = (stds[ch] - med) / sigma;
        if (z > dZThresh) bad.append(ch);
    }
    return bad;
}

//=============================================================================================================

QVector<int> BadChannelDetect::detectLowCorrelation(const MatrixXd& matData,
                                                     double          dCorrThresh,
                                                     int             iNeighbours)
{
    const int nCh = static_cast<int>(matData.rows());
    if (nCh < 2) return {};

    QVector<int> bad;

    for (int ch = 0; ch < nCh; ++ch) {
        int nValid  = 0;
        double sumC = 0.0;

        int lo = std::max(0, ch - iNeighbours);
        int hi = std::min(nCh - 1, ch + iNeighbours);

        for (int nb = lo; nb <= hi; ++nb) {
            if (nb == ch) continue;
            double c = std::abs(pearsonCorr(matData.row(ch), matData.row(nb)));
            sumC += c;
            ++nValid;
        }

        if (nValid == 0) continue;

        double meanCorr = sumC / static_cast<double>(nValid);
        if (meanCorr < dCorrThresh) bad.append(ch);
    }

    return bad;
}
