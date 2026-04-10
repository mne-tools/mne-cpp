//=============================================================================================================
/**
 * @file     sts_ftest.cpp
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
 * @brief    StatsFtest class definition.
 *
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
