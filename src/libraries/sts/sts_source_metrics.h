//=============================================================================================================
/**
 * @file     sts_source_metrics.h
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
 * @brief    StatsSourceMetrics class declaration.
 *
 */

#ifndef STS_SOURCE_METRICS_H
#define STS_SOURCE_METRICS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE STSLIB
//=============================================================================================================

namespace STSLIB
{

//=============================================================================================================
/**
 * Source-space evaluation metrics for inverse solutions.
 *
 * Provides peak localization error (PE) and spatial dispersion (SD)
 * as defined in Lin et al. (2006) and used for CMNE evaluation
 * in Dinh et al. (2021).
 *
 * @brief Source-space evaluation metrics.
 */
class STSSHARED_EXPORT StatsSourceMetrics
{
public:
    //=========================================================================================================
    /**
     * Peak localization error (Euclidean distance between true and estimated peak).
     *
     * @param[in] truePos        True source position (3D).
     * @param[in] estimatedPos   Estimated source position (3D).
     *
     * @return Distance (same unit as input, typically mm).
     */
    static double peakLocalizationError(
        const Eigen::Vector3d& truePos,
        const Eigen::Vector3d& estimatedPos);

    //=========================================================================================================
    /**
     * Spatial dispersion (amplitude-weighted mean distance from peak).
     *
     * SD = sum_k |a_k| * d(k, peak) / sum_k |a_k|
     *
     * @param[in] sourceAmplitudes   Source amplitudes (n_sources).
     * @param[in] sourcePositions    Source positions (n_sources x 3).
     * @param[in] peakIndex          Index of the peak source.
     *
     * @return Spatial dispersion value (same unit as positions).
     */
    static double spatialDispersion(
        const Eigen::VectorXd& sourceAmplitudes,
        const Eigen::MatrixXd& sourcePositions,
        int peakIndex);

    //=========================================================================================================
    /**
     * Find the index of the source with maximum absolute amplitude.
     *
     * @param[in] sourceAmplitudes   Source amplitudes (n_sources).
     *
     * @return Index of the peak source.
     */
    static int findPeakIndex(const Eigen::VectorXd& sourceAmplitudes);
};

} // namespace STSLIB

#endif // STS_SOURCE_METRICS_H
