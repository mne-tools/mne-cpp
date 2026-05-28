//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file peak_finder.cpp
 * @since 2026
 * @date  May 2026
 * @brief Peak detection implementation.
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
