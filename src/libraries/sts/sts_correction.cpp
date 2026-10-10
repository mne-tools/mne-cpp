//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_correction.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of Bonferroni, Holm-Bonferroni and Benjamini-Hochberg FDR corrections declared in sts_correction.h.
 *
 * Bonferroni is a constant rescale by @c n followed by a @c cwiseMin(1).
 * Holm-Bonferroni flattens the p-value matrix, sorts ascending and walks
 * the ranks computing @f$p^*_{(i)} = (n-i+1)\, p_{(i)}@f$ with the
 * cumulative-maximum enforcement that keeps the sequence monotone;
 * results are then scattered back to the original (channel, time)
 * layout. Benjamini-Hochberg FDR uses the conjugate step-up recursion
 * @f$p^*_{(i)} = \min_{k\ge i} \tfrac{n}{k}\, p_{(k)}@f$, capped at 1.
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

MatrixXd StatsMcCorrection::bonferroni(const MatrixXd& pValues, double alpha, RejectMask* reject)
{
    const MatrixXd corrected = (pValues * static_cast<double>(pValues.size())).cwiseMin(1.0);
    if (reject)
        *reject = corrected.array() < alpha;
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

MatrixXd StatsMcCorrection::fdr(const MatrixXd& pValues, double alpha, FdrMethod method, RejectMask* reject)
{
    const int n = static_cast<int>(pValues.size());

    std::vector<std::pair<double, int>> indexed(n);
    for (int i = 0; i < n; ++i)
        indexed[i] = {pValues.data()[i], i};
    std::sort(indexed.begin(), indexed.end());

    // Benjamini-Yekutieli divides the rank factor by the harmonic number of n
    double harmonic = 1.0;
    if (method == FdrMethod::NegativelyCorrelated) {
        harmonic = 0.0;
        for (int k = 1; k <= n; ++k)
            harmonic += 1.0 / k;
    }

    std::vector<double> adjusted(n);
    int lastRejected = -1;
    for (int i = 0; i < n; ++i) {
        const double factor = (i + 1) / (static_cast<double>(n) * harmonic);
        adjusted[i] = indexed[i].first / factor;
        if (indexed[i].first < factor * alpha)
            lastRejected = i;
    }
    for (int i = n - 2; i >= 0; --i)
        adjusted[i] = std::min(adjusted[i], adjusted[i + 1]);

    MatrixXd corrected(pValues.rows(), pValues.cols());
    if (reject)
        *reject = RejectMask::Constant(pValues.rows(), pValues.cols(), false);
    for (int i = 0; i < n; ++i) {
        corrected.data()[indexed[i].second] = std::min(adjusted[i], 1.0);
        if (reject)
            reject->data()[indexed[i].second] = i <= lastRejected;
    }
    return corrected;
}
