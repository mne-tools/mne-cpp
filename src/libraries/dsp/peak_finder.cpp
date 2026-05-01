//=============================================================================================================
/**
 * @file     peak_finder.cpp
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
 * @brief    Peak detection implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "peak_finder.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE FUNCTIONS
//=============================================================================================================

VectorXd UTILSLIB::peakProminences(const VectorXd& data, const QList<int>& peakIndices)
{
    const int n = static_cast<int>(data.size());
    VectorXd proms(peakIndices.size());

    for (int p = 0; p < peakIndices.size(); ++p) {
        int idx = peakIndices[p];
        double peakVal = data(idx);

        // Search left for highest valley before a higher peak
        double leftMin = peakVal;
        for (int i = idx - 1; i >= 0; --i) {
            if (data(i) > peakVal) break;
            leftMin = std::min(leftMin, data(i));
        }

        // Search right for highest valley before a higher peak
        double rightMin = peakVal;
        for (int i = idx + 1; i < n; ++i) {
            if (data(i) > peakVal) break;
            rightMin = std::min(rightMin, data(i));
        }

        // Prominence = peak height minus highest of the two valleys
        double highestValley = std::max(leftMin, rightMin);
        proms(p) = peakVal - highestValley;
    }

    return proms;
}

//=============================================================================================================

QList<QPair<int,double>> UTILSLIB::peakFinder(const VectorXd& data,
                                               const PeakFinderParams& params)
{
    const int n = static_cast<int>(data.size());
    QList<QPair<int,double>> peaks;

    if (n < 3) {
        return peaks;
    }

    // Step 1: Find all local maxima (strictly greater than both neighbours)
    QList<int> peakIdx;
    for (int i = 1; i < n - 1; ++i) {
        if (data(i) > data(i - 1) && data(i) > data(i + 1)) {
            if (data(i) >= params.dMinHeight) {
                peakIdx.append(i);
            }
        }
    }

    // Step 2: Filter by prominence
    if (params.dProminence > 0.0 && !peakIdx.isEmpty()) {
        VectorXd proms = peakProminences(data, peakIdx);
        QList<int> filtered;
        for (int i = 0; i < peakIdx.size(); ++i) {
            if (proms(i) >= params.dProminence) {
                filtered.append(peakIdx[i]);
            }
        }
        peakIdx = filtered;
    }

    // Step 3: Filter by minimum distance (keep highest peaks)
    if (params.iMinDistance > 1 && !peakIdx.isEmpty()) {
        // Sort peaks by height descending
        std::vector<int> sortedIdx(peakIdx.begin(), peakIdx.end());
        std::sort(sortedIdx.begin(), sortedIdx.end(),
                  [&data](int a, int b) { return data(a) > data(b); });

        QList<int> kept;
        std::vector<bool> suppressed(static_cast<size_t>(n), false);

        for (int idx : sortedIdx) {
            if (suppressed[static_cast<size_t>(idx)]) continue;
            kept.append(idx);

            // Suppress nearby peaks
            int lo = std::max(0, idx - params.iMinDistance);
            int hi = std::min(n - 1, idx + params.iMinDistance);
            for (int s = lo; s <= hi; ++s) {
                if (s != idx) {
                    suppressed[static_cast<size_t>(s)] = true;
                }
            }
        }

        // Re-sort by index
        std::sort(kept.begin(), kept.end());
        peakIdx = kept;
    }

    // Build output
    for (int idx : peakIdx) {
        peaks.append(qMakePair(idx, data(idx)));
    }

    return peaks;
}
