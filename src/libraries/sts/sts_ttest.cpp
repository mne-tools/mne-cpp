//=============================================================================================================
/**
 * @file     sts_ttest.cpp
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
 * @brief    StatsTtest class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_ttest.h"

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

StatsTtestResult StatsTtest::oneSample(const MatrixXd& data, double mu, StatsTailType tail)
{
    const int n = static_cast<int>(data.rows());
    const int nVars = static_cast<int>(data.cols());
    const int df = n - 1;

    // Column means
    RowVectorXd means = data.colwise().mean();

    // Column standard deviations
    MatrixXd centered = data.rowwise() - means;
    RowVectorXd stddev = (centered.colwise().squaredNorm() / static_cast<double>(df)).array().sqrt();

    // t-statistics: (mean - mu) / (std / sqrt(n))
    double sqrtN = std::sqrt(static_cast<double>(n));
    RowVectorXd tstat = (means.array() - mu) / (stddev.array() / sqrtN);

    // p-values
    MatrixXd matPval(1, nVars);
    for (int j = 0; j < nVars; ++j) {
        matPval(0, j) = tToPval(tstat(j), df, tail);
    }

    StatsTtestResult result;
    result.matTstat = tstat;
    result.matPval = matPval;
    result.degreesOfFreedom = df;
    return result;
}

//=============================================================================================================

StatsTtestResult StatsTtest::paired(const MatrixXd& dataA, const MatrixXd& dataB, StatsTailType tail)
{
    return oneSample(dataA - dataB, 0.0, tail);
}

//=============================================================================================================

StatsTtestResult StatsTtest::independent(const MatrixXd& dataA, const MatrixXd& dataB, StatsTailType tail)
{
    const int nA = static_cast<int>(dataA.rows());
    const int nB = static_cast<int>(dataB.rows());
    const int nVars = static_cast<int>(dataA.cols());
    const int df = nA + nB - 2;

    RowVectorXd meanA = dataA.colwise().mean();
    RowVectorXd meanB = dataB.colwise().mean();

    // Pooled variance
    MatrixXd centA = dataA.rowwise() - meanA;
    MatrixXd centB = dataB.rowwise() - meanB;
    RowVectorXd ssA = centA.colwise().squaredNorm();
    RowVectorXd ssB = centB.colwise().squaredNorm();
    RowVectorXd pooledVar = (ssA + ssB) / static_cast<double>(df);

    // t-statistic
    double invN = 1.0 / static_cast<double>(nA) + 1.0 / static_cast<double>(nB);
    RowVectorXd tstat = (meanA - meanB).array() / (pooledVar.array() * invN).sqrt();

    // p-values
    MatrixXd matPval(1, nVars);
    for (int j = 0; j < nVars; ++j) {
        matPval(0, j) = tToPval(tstat(j), df, tail);
    }

    StatsTtestResult result;
    result.matTstat = tstat;
    result.matPval = matPval;
    result.degreesOfFreedom = df;
    return result;
}

//=============================================================================================================

double StatsTtest::tCdf(double t, int df)
{
    // P(T <= t) for t-distribution with df degrees of freedom.
    // Using: P(T <= t) = 1 - 0.5 * I_x(df/2, 1/2) where x = df/(df + t^2)  [for t >= 0]
    // For t < 0: P(T <= t) = 0.5 * I_x(df/2, 1/2) where x = df/(df + t^2)
    double x = static_cast<double>(df) / (static_cast<double>(df) + t * t);
    double iBeta = regularizedBeta(x, static_cast<double>(df) / 2.0, 0.5);
    if (t >= 0.0) {
        return 1.0 - 0.5 * iBeta;
    } else {
        return 0.5 * iBeta;
    }
}

//=============================================================================================================

double StatsTtest::tToPval(double t, int df, StatsTailType tail)
{
    double cdf = tCdf(t, df);
    switch (tail) {
    case StatsTailType::Left:
        return cdf;
    case StatsTailType::Right:
        return 1.0 - cdf;
    case StatsTailType::Both:
    default:
        return 2.0 * std::min(cdf, 1.0 - cdf);
    }
}

//=============================================================================================================

double StatsTtest::regularizedBeta(double x, double a, double b)
{
    // I_x(a,b) = x^a * (1-x)^b / (a * B(a,b)) * CF(x,a,b)
    // where CF is the continued fraction expansion.
    // For x > (a+1)/(a+b+2), use the identity I_x(a,b) = 1 - I_{1-x}(b,a).
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;

    if (x > (a + 1.0) / (a + b + 2.0)) {
        return 1.0 - regularizedBeta(1.0 - x, b, a);
    }

    double lnPre = a * std::log(x) + b * std::log(1.0 - x) - logBeta(a, b) - std::log(a);
    return std::exp(lnPre) * betaCf(x, a, b);
}

//=============================================================================================================

double StatsTtest::betaCf(double x, double a, double b)
{
    // Continued fraction for I_x(a,b) using Lentz's algorithm.
    const int maxIter = 200;
    const double eps = 1.0e-12;
    const double tiny = 1.0e-30;

    double qab = a + b;
    double qap = a + 1.0;
    double qam = a - 1.0;

    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    if (std::fabs(d) < tiny) d = tiny;
    d = 1.0 / d;
    double h = d;

    for (int m = 1; m <= maxIter; ++m) {
        double m2 = 2.0 * m;

        // Even step
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < tiny) d = tiny;
        c = 1.0 + aa / c;
        if (std::fabs(c) < tiny) c = tiny;
        d = 1.0 / d;
        h *= d * c;

        // Odd step
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < tiny) d = tiny;
        c = 1.0 + aa / c;
        if (std::fabs(c) < tiny) c = tiny;
        d = 1.0 / d;
        double del = d * c;
        h *= del;

        if (std::fabs(del - 1.0) < eps) break;
    }
    return h;
}

//=============================================================================================================

double StatsTtest::logBeta(double a, double b)
{
    return std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b);
}
