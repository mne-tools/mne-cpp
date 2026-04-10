//=============================================================================================================
/**
 * @file     sts_correction.cpp
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
 * @brief    StatsMcCorrection class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_correction.h"

#include <algorithm>
#include <numeric>
#include <vector>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

MatrixXd StatsMcCorrection::bonferroni(const MatrixXd& pValues)
{
    const int n = static_cast<int>(pValues.size());
    MatrixXd corrected = pValues * static_cast<double>(n);
    corrected = corrected.cwiseMin(1.0);
    return corrected;
}

//=============================================================================================================

MatrixXd StatsMcCorrection::holmBonferroni(const MatrixXd& pValues)
{
    const int n = static_cast<int>(pValues.size());

    // Flatten to a vector with indices
    std::vector<std::pair<double, int>> indexed(n);
    for (int i = 0; i < n; ++i) {
        indexed[i] = {pValues.data()[i], i};
    }

    // Sort ascending by p-value
    std::sort(indexed.begin(), indexed.end());

    // Compute adjusted p-values: adjusted_p[i] = p * (n - rank_i + 1), rank is 1-based
    std::vector<double> adjusted(n);
    for (int i = 0; i < n; ++i) {
        adjusted[i] = indexed[i].first * static_cast<double>(n - i);
    }

    // Enforce monotonicity: from first to last, corrected[i] = max(corrected[i], corrected[i-1])
    for (int i = 1; i < n; ++i) {
        adjusted[i] = std::max(adjusted[i], adjusted[i - 1]);
    }

    // Cap at 1.0 and place back in original order
    MatrixXd corrected(pValues.rows(), pValues.cols());
    for (int i = 0; i < n; ++i) {
        corrected.data()[indexed[i].second] = std::min(adjusted[i], 1.0);
    }

    return corrected;
}

//=============================================================================================================

MatrixXd StatsMcCorrection::fdr(const MatrixXd& pValues, double alpha)
{
    Q_UNUSED(alpha);

    const int n = static_cast<int>(pValues.size());

    // Flatten to a vector with indices
    std::vector<std::pair<double, int>> indexed(n);
    for (int i = 0; i < n; ++i) {
        indexed[i] = {pValues.data()[i], i};
    }

    // Sort ascending by p-value
    std::sort(indexed.begin(), indexed.end());

    // Compute adjusted p-values: adjusted_p = p * n / rank (rank is 1-based)
    std::vector<double> adjusted(n);
    for (int i = 0; i < n; ++i) {
        int rank = i + 1;
        adjusted[i] = indexed[i].first * static_cast<double>(n) / static_cast<double>(rank);
    }

    // Enforce monotonicity: from last to first, corrected[i] = min(corrected[i], corrected[i+1])
    for (int i = n - 2; i >= 0; --i) {
        adjusted[i] = std::min(adjusted[i], adjusted[i + 1]);
    }

    // Cap at 1.0 and place back in original order
    MatrixXd corrected(pValues.rows(), pValues.cols());
    for (int i = 0; i < n; ++i) {
        corrected.data()[indexed[i].second] = std::min(adjusted[i], 1.0);
    }

    return corrected;
}
