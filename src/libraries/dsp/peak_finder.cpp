//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     peak_finder.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
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
#include <limits>
#include <vector>

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
            if (data(i) > peakVal)
                break;
            leftMin = std::min(leftMin, data(i));
        }

        // Search right for highest valley before a higher peak
        double rightMin = peakVal;
        for (int i = idx + 1; i < n; ++i) {
            if (data(i) > peakVal)
                break;
            rightMin = std::min(rightMin, data(i));
        }

        // Prominence = peak height minus highest of the two valleys
        double highestValley = std::max(leftMin, rightMin);
        proms(p) = peakVal - highestValley;
    }

    return proms;
}

//=============================================================================================================

QList<QPair<int, double>> UTILSLIB::peakFinder(const VectorXd& data,
                                               const PeakFinderParams& params)
{
    const int n = static_cast<int>(data.size());
    QList<QPair<int, double>> peaks;

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
            if (suppressed[static_cast<size_t>(idx)])
                continue;
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

//=============================================================================================================

QList<QPair<int, double>> UTILSLIB::peakFinderMne(const VectorXd& data, double thresh, int extrema)
{
    // Adapted from mne.preprocessing.peak_finder (MNE-Python, BSD-3-Clause).
    QList<QPair<int, double>> result;
    const Index s = data.size();
    if (s == 0)
        return result;
    if (std::isnan(thresh))
        thresh = (data.maxCoeff() - data.minCoeff()) / 4.0;
    const VectorXd x0 = extrema < 0 ? VectorXd(-data) : data;

    // Turning points (the first of repeated values) plus both end points
    std::vector<Index> ind{0};
    for (Index i = 1; i + 1 < s; ++i) {
        double left = x0(i) - x0(i - 1);
        double right = x0(i + 1) - x0(i);
        if (left == 0.0)
            left = -std::numeric_limits<double>::epsilon();
        if (right == 0.0)
            right = -std::numeric_limits<double>::epsilon();
        if (left * right < 0.0)
            ind.push_back(i);
    }
    ind.push_back(s - 1);
    std::vector<double> x(ind.size());
    for (std::size_t i = 0; i < ind.size(); ++i)
        x[i] = x0(ind[i]);

    const double minMag = *std::min_element(x.begin(), x.end());
    std::vector<Index> peakInds;
    std::vector<double> peakMags;
    if (x.size() > 2) {
        double tempMag = minMag;
        double leftMin = minMag;
        bool foundPeak = false;
        Index tempLoc = 0;
        const auto sign = [](double v) {
            return (v > 0.0) - (v < 0.0);
        };
        const int sign0 = sign(x[1] - x[0]);
        const int sign1 = sign(x[2] - x[1]);
        long ii = 0;
        if (sign0 <= 0) {
            ii = -1;
            if (sign0 == sign1) {
                x.erase(x.begin() + 1);
                ind.erase(ind.begin() + 1);
            }
        } else if (sign0 == sign1) {
            x.erase(x.begin());
            ind.erase(ind.begin());
        }
        const long length = static_cast<long>(x.size());
        // mne compares with the last slot of its preallocated peak array, which stays 0 until it is filled
        const std::size_t maxPeaks = static_cast<std::size_t>(std::ceil(length / 2.0));
        const auto lastSlot = [&]() {
            return peakMags.size() == maxPeaks ? peakMags.back() : 0.0;
        };
        while (ii < length - 1) {
            ++ii;
            if (foundPeak && (x[ii] > lastSlot() || leftMin < lastSlot() - thresh)) {
                tempMag = minMag;
                foundPeak = false;
            }
            if (ii == length - 1)
                break;
            if (x[ii] > tempMag && x[ii] > leftMin + thresh) {
                tempLoc = ii;
                tempMag = x[ii];
            }
            ++ii;
            if (!foundPeak && tempMag > thresh + x[ii]) {
                foundPeak = true;
                leftMin = x[ii];
                peakInds.push_back(tempLoc);
                peakMags.push_back(tempMag);
            } else if (x[ii] < leftMin) {
                leftMin = x[ii];
            }
        }
        if (x.back() > tempMag && x.back() > leftMin + thresh) {
            peakInds.push_back(length - 1);
            peakMags.push_back(x.back());
        } else if (!foundPeak && tempMag > minMag) {
            peakInds.push_back(tempLoc);
            peakMags.push_back(tempMag);
        }
        for (std::size_t k = 0; k < peakInds.size(); ++k)
            result.append(qMakePair(static_cast<int>(ind[peakInds[k]]), extrema < 0 ? -peakMags[k] : peakMags[k]));
    } else {
        const auto best = std::max_element(x.begin(), x.end());
        if (*best > minMag + thresh)
            result.append(qMakePair(static_cast<int>(ind[best - x.begin()]), extrema < 0 ? -*best : *best));
    }
    return result;
}
