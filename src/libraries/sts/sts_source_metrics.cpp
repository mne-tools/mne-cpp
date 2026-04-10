//=============================================================================================================
/**
 * @file     sts_source_metrics.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    StatsSourceMetrics class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_source_metrics.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

double StatsSourceMetrics::peakLocalizationError(
    const Vector3d& truePos,
    const Vector3d& estimatedPos)
{
    return (truePos - estimatedPos).norm();
}

//=============================================================================================================

double StatsSourceMetrics::spatialDispersion(
    const VectorXd& sourceAmplitudes,
    const MatrixXd& sourcePositions,
    int peakIndex)
{
    int nSources = sourceAmplitudes.size();
    Vector3d peakPos = sourcePositions.row(peakIndex);

    double weightedSum = 0.0;
    double totalWeight = 0.0;

    for (int k = 0; k < nSources; ++k) {
        double absAmp = std::abs(sourceAmplitudes(k));
        double dist = (sourcePositions.row(k).transpose() - peakPos).norm();
        weightedSum += dist * absAmp;
        totalWeight += absAmp;
    }

    if (totalWeight < 1e-10) {
        return 0.0;
    }

    return weightedSum / totalWeight;
}

//=============================================================================================================

int StatsSourceMetrics::findPeakIndex(const VectorXd& sourceAmplitudes)
{
    int peakIdx = 0;
    sourceAmplitudes.cwiseAbs().maxCoeff(&peakIdx);
    return peakIdx;
}
