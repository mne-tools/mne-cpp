//=============================================================================================================
/**
 * @file     sts_ttest.h
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
 * @brief    StatsTtest class declaration.
 *
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
 * @brief T-test statistical testing.
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
