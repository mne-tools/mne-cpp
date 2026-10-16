//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sts_ttest.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of one-sample, paired and independent two-sample t-tests declared in sts_ttest.h.
 *
 * One-sample and paired variants compute the column-wise sample mean and
 * unbiased standard deviation in two Eigen reductions and form
 * @f$t = (\bar{x}-\mu)/(s/\sqrt{n})@f$ on @c n-1 degrees of freedom; the
 * paired test reduces to the one-sample test on the difference @c A-B.
 * The independent two-sample test assumes equal variances and uses the
 * pooled standard deviation with @c n_A+n_B-2 degrees of freedom.
 *
 * Tail-specific p-values come from the exact Student-t CDF, expressed
 * through the regularised incomplete beta function and evaluated with a
 * Lentz continued-fraction recursion plus a @c logGamma-based normaliser
 * so the tails stay numerically stable for very large @c df.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_ttest.h"

#include <cmath>
#include <algorithm>
#include <QtGlobal>
#include <random>
#include <set>
#include <vector>

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

StatsTtestResult StatsTtest::permutation(const MatrixXd& data, int nPermutations, StatsTailType tail, unsigned int seed)
{
    // Adapted from mne.stats.permutation_t_test / _get_1samp_orders / _max_stat (MNE-Python, BSD-3-Clause).
    const int n = static_cast<int>(data.rows());
    const RowVectorXd x2 = data.array().square().colwise().mean();
    const double dofScaling = std::sqrt(n / (n - 1.0));
    const double sqrtN = std::sqrt(static_cast<double>(n));
    const auto tValues = [&](const RowVectorXd& mu) -> RowVectorXd {
        return mu.array() / ((x2.array() - mu.array().square()).sqrt() * dofScaling / sqrtN);
    };
    const RowVectorXd tObs = tValues(data.colwise().mean());

    // Sign flips as n-bit numbers, sample 0 in the most significant bit; a two-sided test never flips sample 0
    const int bits = n - (tail == StatsTailType::Both ? 1 : 0);
    if (n < 2 || bits > 63) {
        qWarning("StatsTtest::permutation: needs 2 to 63 samples, got %d.", n);
        return StatsTtestResult();
    }
    const quint64 maxPerms = (quint64(1) << bits) - 1;
    std::vector<quint64> orders;
    if (static_cast<quint64>(std::max(nPermutations, 0)) > maxPerms) {
        for (quint64 s = 1; s <= maxPerms; ++s)
            orders.push_back(s);
    } else {
        std::mt19937_64 gen(seed);
        std::uniform_int_distribution<quint64> pick(1, maxPerms);
        std::set<quint64> seen;
        while (static_cast<int>(orders.size()) < nPermutations - 1) {
            const quint64 s = pick(gen);
            if (seen.insert(s).second)
                orders.push_back(s);
        }
    }

    std::vector<double> h0;
    h0.reserve(orders.size() + 1);
    for (quint64 s : orders) {
        VectorXd signs(n);
        for (int i = 0; i < n; ++i)
            signs(i) = ((s >> (n - 1 - i)) & 1) ? -1.0 : 1.0;
        h0.push_back(tValues(signs.transpose() * data / n).cwiseAbs().maxCoeff());
    }
    h0.push_back(tObs.cwiseAbs().maxCoeff());

    MatrixXd pvals(1, tObs.size());
    for (int j = 0; j < tObs.size(); ++j) {
        const double t = tObs(j);
        const auto count = std::count_if(h0.begin(), h0.end(), [&](double h) {
            return tail == StatsTailType::Both ? h >= std::abs(t) : tail == StatsTailType::Right ? h >= t
                                                                                                 : -h <= t;
        });
        pvals(0, j) = static_cast<double>(count) / static_cast<double>(h0.size());
    }

    StatsTtestResult result;
    result.matTstat = tObs;
    result.matPval = pvals;
    result.degreesOfFreedom = n - 1;
    return result;
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
    // Each tail directly from I_x(df/2, 1/2) = P(|T| >= |t|), so p-values below 1e-16 are not lost to 1 - CDF
    const double x = static_cast<double>(df) / (static_cast<double>(df) + t * t);
    const double twoSided = regularizedBeta(x, static_cast<double>(df) / 2.0, 0.5);
    switch (tail) {
        case StatsTailType::Left:
            return t < 0.0 ? 0.5 * twoSided : 1.0 - 0.5 * twoSided;
        case StatsTailType::Right:
            return t >= 0.0 ? 0.5 * twoSided : 1.0 - 0.5 * twoSided;
        case StatsTailType::Both:
        default:
            return twoSided;
    }
}

//=============================================================================================================

double StatsTtest::regularizedBeta(double x, double a, double b)
{
    // I_x(a,b) = x^a * (1-x)^b / (a * B(a,b)) * CF(x,a,b)
    // where CF is the continued fraction expansion.
    // For x > (a+1)/(a+b+2), use the identity I_x(a,b) = 1 - I_{1-x}(b,a).
    if (x <= 0.0)
        return 0.0;
    if (x >= 1.0)
        return 1.0;

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
    if (std::fabs(d) < tiny)
        d = tiny;
    d = 1.0 / d;
    double h = d;

    for (int m = 1; m <= maxIter; ++m) {
        double m2 = 2.0 * m;

        // Even step
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < tiny)
            d = tiny;
        c = 1.0 + aa / c;
        if (std::fabs(c) < tiny)
            c = tiny;
        d = 1.0 / d;
        h *= d * c;

        // Odd step
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < tiny)
            d = tiny;
        c = 1.0 + aa / c;
        if (std::fabs(c) < tiny)
            c = tiny;
        d = 1.0 / d;
        double del = d * c;
        h *= del;

        if (std::fabs(del - 1.0) < eps)
            break;
    }
    return h;
}

//=============================================================================================================

double StatsTtest::logBeta(double a, double b)
{
    return std::lgamma(a) + std::lgamma(b) - std::lgamma(a + b);
}
