//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file peak_finder.h
 * @since 2026
 * @date  May 2026
 * @brief Local-maxima peak detection in 1-D signals with prominence filtering.
 *
 * A peak is defined as a sample that is strictly larger than its two direct
 * neighbours. The basic detector locates every such sample in linear time;
 * three classical post-filters then prune the candidate list:
 *   * a minimum height (absolute amplitude floor),
 *   * a minimum inter-peak distance in samples (suppressing close-by peaks
 *     by retaining only the tallest within the exclusion window), and
 *   * a minimum topographic prominence — the height by which a peak rises
 *     above the higher of the two adjacent valleys, computed in the same
 *     way as @c scipy.signal.peak_prominences.
 *
 * The API mirrors @c scipy.signal.find_peaks closely so MEG/EEG analysis
 * pipelines that already rely on SciPy semantics (cHPI peak picking, ECG
 * R-wave detection, event onset extraction) port to mne-cpp without
 * behavioural drift.
 */

#ifndef PEAK_FINDER_H
#define PEAK_FINDER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QPair>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * Parameters for peak detection.
 */
struct DSPSHARED_EXPORT PeakFinderParams
{
    double dMinHeight = -std::numeric_limits<double>::infinity();   /**< Minimum peak height. */
    int iMinDistance = 1;                                            /**< Minimum distance between peaks (samples). */
    double dProminence = 0.0;                                       /**< Minimum peak prominence. */
};

//=============================================================================================================
/**
 * @brief Find peaks in a 1D signal.
 *
 * A peak is a sample that is strictly greater than both its neighbours.
 * Additional filtering by height, distance, and prominence can be applied.
 *
 * @param[in] data      1D signal (row vector or VectorXd).
 * @param[in] params    Peak detection parameters.
 *
 * @return List of (peak_index, peak_value) pairs, sorted by index.
 */
DSPSHARED_EXPORT QList<QPair<int,double>> peakFinder(const Eigen::VectorXd& data,
                                                      const PeakFinderParams& params = PeakFinderParams());

//=============================================================================================================
/**
 * @brief Compute prominence of each peak.
 *
 * Prominence is the vertical distance from a peak to the highest valley
 * on either side before reaching a higher peak.
 *
 * @param[in] data          1D signal.
 * @param[in] peakIndices   Indices of detected peaks.
 *
 * @return Prominence value for each peak.
 */
DSPSHARED_EXPORT Eigen::VectorXd peakProminences(const Eigen::VectorXd& data,
                                                   const QList<int>& peakIndices);

} // namespace UTILSLIB

#endif // PEAK_FINDER_H
