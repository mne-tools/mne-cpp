//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_ftest.cpp
 * @since April 2026
 * @brief Implementation of the one-way ANOVA F-test declared in sts_ftest.h.
 *
 * Computes the grand mean across all groups, then per group adds
 * @c n_g*(mean_g - grand_mean)^2 to the between-group sum-of-squares and
 * the row-wise squared deviations from the group mean to the within-group
 * sum-of-squares. Mean-square ratios give the F-statistic with @c k-1
 * and @c N-k degrees of freedom.
 *
 * The right-tail p-value is obtained from the exact F CDF via the
 * regularised incomplete beta function reused from @ref STSLIB::StatsTtest,
 * so the t- and F-test machinery share the same numerically-stable
 * special-function back-end.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_ftest.h"
#include "sts_ttest.h"

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

StatsFtestResult StatsFtest::oneWay(const QVector<MatrixXd>& groups)
{
    const int k = groups.size();
    const int nVars = static_cast<int>(groups[0].cols());

    // Total number of observations
    int N = 0;
    for (int g = 0; g < k; ++g) {
        N += static_cast<int>(groups[g].rows());
    }

    // Grand mean
    RowVectorXd grandMean = RowVectorXd::Zero(nVars);
    for (int g = 0; g < k; ++g) {
        grandMean += groups[g].colwise().sum();
    }
    grandMean /= static_cast<double>(N);

    // Between-group sum of squares and within-group sum of squares
    RowVectorXd ssBetween = RowVectorXd::Zero(nVars);
    RowVectorXd ssWithin = RowVectorXd::Zero(nVars);

    for (int g = 0; g < k; ++g) {
        int ng = static_cast<int>(groups[g].rows());
        RowVectorXd groupMean = groups[g].colwise().mean();

        // Between: n_g * (groupMean - grandMean)^2
        RowVectorXd diff = groupMean - grandMean;
        ssBetween += static_cast<double>(ng) * diff.array().square().matrix();

        // Within: sum of (x_i - groupMean)^2
        MatrixXd centered = groups[g].rowwise() - groupMean;
        ssWithin += centered.colwise().squaredNorm();
    }

    int dfBetween = k - 1;
    int dfWithin = N - k;

    RowVectorXd msBetween = ssBetween / static_cast<double>(dfBetween);
    RowVectorXd msWithin = ssWithin / static_cast<double>(dfWithin);

    // F-statistic
    RowVectorXd fstat = msBetween.array() / msWithin.array();

    // p-values
    MatrixXd matPval(1, nVars);
    for (int j = 0; j < nVars; ++j) {
        matPval(0, j) = 1.0 - fCdf(fstat(j), dfBetween, dfWithin);
    }

    StatsFtestResult result;
    result.matFstat = fstat;
    result.matPval = matPval;
    result.dfBetween = dfBetween;
    result.dfWithin = dfWithin;
    return result;
}

//=============================================================================================================

double StatsFtest::fCdf(double f, int df1, int df2)
{
    // P(F <= f) using the relationship to the regularized incomplete beta function:
    // P(F <= f) = I_x(df1/2, df2/2) where x = df1*f / (df1*f + df2)
    if (f <= 0.0) return 0.0;

    double x = static_cast<double>(df1) * f / (static_cast<double>(df1) * f + static_cast<double>(df2));
    double a = static_cast<double>(df1) / 2.0;
    double b = static_cast<double>(df2) / 2.0;

    // Reuse the regularized beta function from StatsTtest
    return StatsTtest::regularizedBeta(x, a, b);
}
