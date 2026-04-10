//=============================================================================================================
/**
 * @file     sts_cov_estimators.cpp
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
 * @brief    StsCovEstimators class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sts_cov_estimators.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::pair<MatrixXd, double> StsCovEstimators::ledoitWolf(const MatrixXd& matData)
{
    const int p = static_cast<int>(matData.rows());
    const int n = static_cast<int>(matData.cols());

    // Sample covariance (using 1/n, matching Ledoit-Wolf and scikit-learn)
    const MatrixXd S = (matData * matData.transpose()) / static_cast<double>(n);

    // Shrinkage target: mu * I_p
    const double mu = S.trace() / static_cast<double>(p);

    // delta = ||S - mu * I||^2_F / p
    MatrixXd centered = S - mu * MatrixXd::Identity(p, p);
    const double delta = centered.squaredNorm() / static_cast<double>(p);

    // If delta is essentially zero, S is already a scaled identity
    if (delta < 1e-30) {
        return {S, 0.0};
    }

    // Efficient computation of beta:
    // sum_k ||x_k x_k^T - S||^2_F = sum_k (x_k^T x_k)^2 - n * ||S||^2_F
    double sumSqNorms = 0.0;
    for (int k = 0; k < n; ++k) {
        const double nrm = matData.col(k).squaredNorm();
        sumSqNorms += nrm * nrm;
    }
    const double betaSum = sumSqNorms - static_cast<double>(n) * S.squaredNorm();
    const double beta = betaSum / (static_cast<double>(n) * static_cast<double>(n) * static_cast<double>(p));

    // Optimal shrinkage coefficient, clipped to [0, 1]
    const double alpha = std::clamp(beta / delta, 0.0, 1.0);

    // Shrunk covariance: alpha * mu * I + (1 - alpha) * S
    MatrixXd covShrunk = (1.0 - alpha) * S;
    covShrunk.diagonal().array() += alpha * mu;

    return {covShrunk, alpha};
}
