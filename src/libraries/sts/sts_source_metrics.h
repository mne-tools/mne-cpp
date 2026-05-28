//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_source_metrics.h
 * @since April 2026
 * @brief Peak localisation error and spatial dispersion metrics for evaluating distributed M/EEG inverse solutions.
 *
 * Distributed source estimators (MNE, dSPM, sLORETA, eLORETA, CMNE)
 * smear point sources across a neighbourhood of the cortex, so judging
 * an inverse solver requires more than just looking at the maximum.
 * This module provides the two metrics standardly reported in the
 * source-localisation literature: peak localisation error (PE), the
 * Euclidean distance between the true source and the estimated peak,
 * and spatial dispersion (SD), the amplitude-weighted mean distance
 * from the estimated peak
 * @f$\mathrm{SD} = \sum_k |a_k|\,\|r_k - r_\text{peak}\| / \sum_k |a_k|@f$.
 *
 * PE quantifies localisation bias; SD quantifies how focally the
 * solver concentrates activity around that peak. A good solver achieves
 * a small PE and a small SD; trade-offs between the two characterise
 * the different inverse families. The helper @c findPeakIndex returns
 * the index of the source with maximum absolute amplitude so callers
 * never have to roll their own argmax.
 *
 * References: Molins et al. (2008), NeuroImage 42(3); Lin et al. (2006),
 * NeuroImage 31(1); Dinh et al. (2021), Front. Neurosci. 15.
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
 * @brief Peak localisation error and spatial dispersion metrics for evaluating distributed M/EEG inverse solutions.
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
