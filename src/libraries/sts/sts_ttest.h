//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sts_ttest.h
 * @since 2026
 * @date  April 2026
 * @brief Frequentist Student's t-tests with exact p-values via the regularised incomplete beta function.
 *
 * Provides the three single-sample / paired / two-sample independent
 * variants of Student's t-test that the rest of STSLIB needs. All three
 * test the null hypothesis that the mean (or mean difference) equals
 * @c mu, with the alternative selected via @ref STSLIB::StatsTailType
 * (left, right, both). The statistic is the textbook
 * @f$t = (\bar{x}-\mu) / (s/\sqrt{n})@f$ with @c n-1 / @c n-1 / @c n_A+n_B-2
 * degrees of freedom; columns of the input matrix are tested in parallel
 * so a (n_obs x n_features) array produces a single row of t-values and
 * p-values in one call.
 *
 * The p-value is computed from the exact Student-t CDF, evaluated via
 * the regularised incomplete beta function @f$I_x(a,b)@f$ with the
 * Lentz continued-fraction recursion of Numerical Recipes 6.4; no
 * normal-approximation fallback is used. The CDF and incomplete-beta
 * routines are exposed as @c public so @ref STSLIB::StatsFtest can reuse
 * them for its F-distribution implementation.
 *
 * References: Student (1908), Biometrika 6(1); Press et al., Numerical
 * Recipes 3rd ed., section 6.4.
 */

#ifndef STS_TTEST_H
#define STS_TTEST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_global.h"
#include "sts_types.h"

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
 * Result structure for t-tests.
 *
 * @brief Per-call output of a Student t-test: t-statistics, p-values and degrees of freedom.
 */
struct STSSHARED_EXPORT StatsTtestResult {
    Eigen::MatrixXd matTstat;       /**< t-statistics (same shape as input columns). */
    Eigen::MatrixXd matPval;        /**< p-values. */
    int degreesOfFreedom;           /**< Degrees of freedom. */
};

//=============================================================================================================
/**
 * Provides t-test implementations: one-sample, paired, and independent two-sample.
 *
 * @brief One-sample, paired and independent two-sample Student t-tests with exact p-values via the regularised incomplete beta function.
 */
class STSSHARED_EXPORT StatsTtest
{
public:
    //=========================================================================================================
    /**
     * One-sample t-test. Each column of data is tested against mu.
     *
     * @param[in] data   Matrix of observations (rows = observations, cols = variables).
     * @param[in] mu     Hypothesized mean (default 0.0).
     * @param[in] tail   Tail type for the test.
     *
     * @return StatsTtestResult with t-statistics, p-values, and degrees of freedom.
     */
    static StatsTtestResult oneSample(const Eigen::MatrixXd& data,
                                      double mu = 0.0,
                                      StatsTailType tail = StatsTailType::Both);

    //=========================================================================================================
    /**
     * Paired t-test. Tests dataA - dataB against 0.
     *
     * @param[in] dataA  Matrix of observations for condition A.
     * @param[in] dataB  Matrix of observations for condition B (same dimensions as dataA).
     * @param[in] tail   Tail type for the test.
     *
     * @return StatsTtestResult with t-statistics, p-values, and degrees of freedom.
     */
    static StatsTtestResult paired(const Eigen::MatrixXd& dataA,
                                   const Eigen::MatrixXd& dataB,
                                   StatsTailType tail = StatsTailType::Both);

    //=========================================================================================================
    /**
     * Independent two-sample t-test.
     *
     * @param[in] dataA  Matrix of observations for group A (rows = observations, cols = variables).
     * @param[in] dataB  Matrix of observations for group B (same number of columns as dataA).
     * @param[in] tail   Tail type for the test.
     *
     * @return StatsTtestResult with t-statistics, p-values, and degrees of freedom.
     */
    static StatsTtestResult independent(const Eigen::MatrixXd& dataA,
                                        const Eigen::MatrixXd& dataB,
                                        StatsTailType tail = StatsTailType::Both);

    //=========================================================================================================
    /**
     * Compute the CDF of the t-distribution using the regularized incomplete beta function.
     *
     * @param[in] t   The t-value.
     * @param[in] df  Degrees of freedom.
     *
     * @return The probability P(T <= t).
     */
    static double tCdf(double t, int df);

    //=========================================================================================================
    /**
     * Regularized incomplete beta function I_x(a, b).
     * Public because it is also used by StatsFtest.
     */
    static double regularizedBeta(double x, double a, double b);

private:

    //=========================================================================================================
    /**
     * Continued fraction expansion for regularized incomplete beta function.
     */
    static double betaCf(double x, double a, double b);

    //=========================================================================================================
    /**
     * Log of the Beta function: ln(B(a,b)) = lnGamma(a) + lnGamma(b) - lnGamma(a+b).
     */
    static double logBeta(double a, double b);

    //=========================================================================================================
    /**
     * Convert t-statistic and df to a p-value based on tail type.
     */
    static double tToPval(double t, int df, StatsTailType tail);
};

} // namespace STSLIB

#endif // STS_TTEST_H
