//=============================================================================================================
/**
 * @file     dpss.cpp
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
 * @brief    Implementation of Dpss.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dpss.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

namespace
{
constexpr double DPSS_PI = 3.14159265358979323846;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DpssResult Dpss::compute(int N, double halfBandwidth, int nTapers)
{
    if (nTapers < 0)
        nTapers = static_cast<int>(std::floor(2.0 * halfBandwidth - 1.0));

    const double W = halfBandwidth / static_cast<double>(N);

    // Build the symmetric tridiagonal matrix T
    // Diagonal:     d(i) = ((N-1-2*i)/2)^2 * cos(2*pi*W)
    // Off-diagonal: e(i) = (i+1)*(N-i-1)/2
    VectorXd diag(N);
    VectorXd offdiag(N > 1 ? N - 1 : 0);

    const double cosW = std::cos(2.0 * DPSS_PI * W);

    for (int i = 0; i < N; ++i) {
        const double val = (static_cast<double>(N - 1) - 2.0 * static_cast<double>(i)) / 2.0;
        diag[i] = val * val * cosW;
    }

    for (int i = 0; i < N - 1; ++i) {
        offdiag[i] = static_cast<double>(i + 1) * static_cast<double>(N - i - 1) / 2.0;
    }

    // Construct tridiagonal matrix and solve eigenvalue problem
    MatrixXd T = MatrixXd::Zero(N, N);
    for (int i = 0; i < N; ++i)
        T(i, i) = diag[i];
    for (int i = 0; i < N - 1; ++i) {
        T(i, i + 1) = offdiag[i];
        T(i + 1, i) = offdiag[i];
    }

    SelfAdjointEigenSolver<MatrixXd> solver(T);

    // Eigenvalues are sorted ascending; we want the largest nTapers
    const VectorXd& allEigenvalues = solver.eigenvalues();
    const MatrixXd& allEigenvectors = solver.eigenvectors();

    DpssResult result;
    result.matTapers.resize(nTapers, N);
    result.vecEigenvalues.resize(nTapers);

    for (int k = 0; k < nTapers; ++k) {
        const int idx = N - 1 - k;  // largest eigenvalue first
        result.vecEigenvalues[k] = allEigenvalues[idx];

        // Extract eigenvector as a row, normalize to unit L2 norm
        VectorXd taper = allEigenvectors.col(idx);
        const double norm = taper.norm();
        if (norm > 0.0)
            taper /= norm;

        // Sign convention: first element should be positive
        if (taper[0] < 0.0)
            taper = -taper;

        result.matTapers.row(k) = taper.transpose();
    }

    return result;
}
