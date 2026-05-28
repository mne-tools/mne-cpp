//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_source_metrics.cpp
 * @since April 2026
 * @brief Implementation of the peak localisation error and spatial dispersion metrics declared in sts_source_metrics.h.
 *
 * Peak localisation error is the Euclidean norm of the difference
 * between true and estimated peak positions. Spatial dispersion walks
 * the source list once, accumulating @c |a_k| * dist(k, peak) into the
 * numerator and @c |a_k| into the denominator so the result is the
 * amplitude-weighted mean distance from the estimated peak in the same
 * units as the input positions (typically millimetres). @c findPeakIndex
 * is a single absolute-value argmax over the amplitude vector.
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
