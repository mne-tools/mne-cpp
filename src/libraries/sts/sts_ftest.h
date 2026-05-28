//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_ftest.h
 * @since April 2026
 * @brief One-way ANOVA F-test with exact p-values for comparing two or more independent groups.
 *
 * Provides a one-way ANOVA on a list of @c k matrices, each holding
 * @c n_g observations of the same set of variables. For each variable
 * (column) the routine partitions the total sum-of-squares into the
 * between-group and within-group components and forms
 * @f$F = \mathrm{MS}_\text{between} / \mathrm{MS}_\text{within}@f$ with
 * @c k-1 numerator and @c N-k denominator degrees of freedom, where
 * @c N = sum of all @c n_g.
 *
 * The null hypothesis is equality of the group means; the alternative is
 * that at least one group mean differs. P-values come from the exact F
 * CDF expressed via the regularised incomplete beta function that
 * @ref STSLIB::StatsTtest already exposes, so the two test families
 * remain numerically consistent. The F-statistic is also the natural
 * cluster-forming statistic for @ref STSLIB::StatsCluster::fTestPermutationTest.
 *
 * Reference: Fisher (1925), Statistical Methods for Research Workers,
 * chapter 7.
 */

#ifndef STS_FTEST_H
#define STS_FTEST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

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
 * Result structure for F-tests.
 *
 * @brief Per-call output of a one-way ANOVA F-test: F-statistics, p-values, and between/within degrees of freedom.
 */
struct STSSHARED_EXPORT StatsFtestResult {
    Eigen::MatrixXd matFstat;   /**< F-statistics. */
    Eigen::MatrixXd matPval;    /**< p-values. */
    int dfBetween;              /**< Between-groups degrees of freedom. */
    int dfWithin;               /**< Within-groups degrees of freedom. */
};

//=============================================================================================================
/**
 * Provides F-test / one-way ANOVA implementation.
 *
 * @brief One-way ANOVA F-test with exact p-values via the regularised incomplete beta function.
 */
class STSSHARED_EXPORT StatsFtest
{
public:
    //=========================================================================================================
    /**
     * One-way ANOVA F-test.
     *
     * @param[in] groups  Vector of matrices, each n_observations x n_variables.
     *
     * @return StatsFtestResult with F-statistics, p-values, and degrees of freedom.
     */
    static StatsFtestResult oneWay(const QVector<Eigen::MatrixXd>& groups);

    //=========================================================================================================
    /**
     * Compute the CDF of the F-distribution using the regularized incomplete beta function.
     *
     * @param[in] f    The F-value.
     * @param[in] df1  Numerator degrees of freedom.
     * @param[in] df2  Denominator degrees of freedom.
     *
     * @return The probability P(F <= f).
     */
    static double fCdf(double f, int df1, int df2);
};

} // namespace STSLIB

#endif // STS_FTEST_H
