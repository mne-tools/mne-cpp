//=============================================================================================================
/**
 * @file     peak_finder.h
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
 * @brief    General-purpose peak detection functions.
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
