//=============================================================================================================
/**
 * @file     connectivity_aec.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    ConnectivityAec class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_aec.h"

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <complex>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace {

// Simple DFT-based Hilbert transform (no external FFT library needed)
// For production use, this should be replaced with an FFT-based implementation
void dftHilbert(const VectorXd& input, VectorXd& envelope)
{
    const int n = static_cast<int>(input.size());
    envelope.resize(n);

    if (n == 0)
        return;

    // Compute DFT
    std::vector<std::complex<double>> X(static_cast<size_t>(n));
    for (int k = 0; k < n; ++k) {
        std::complex<double> sum(0.0, 0.0);
        for (int t = 0; t < n; ++t) {
            double angle = -2.0 * M_PI * k * t / n;
            sum += input[t] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        X[static_cast<size_t>(k)] = sum;
    }

    // Apply Hilbert transform in frequency domain:
    // H[0] = X[0], H[N/2] = X[N/2]
    // H[k] = 2*X[k] for 0 < k < N/2
    // H[k] = 0       for N/2 < k < N
    std::vector<std::complex<double>> H(static_cast<size_t>(n));
    H[0] = X[0];
    for (int k = 1; k < (n + 1) / 2; ++k)
        H[static_cast<size_t>(k)] = 2.0 * X[static_cast<size_t>(k)];
    if (n % 2 == 0)
        H[static_cast<size_t>(n / 2)] = X[static_cast<size_t>(n / 2)];
    for (int k = (n + 1) / 2; k < n; ++k)
        H[static_cast<size_t>(k)] = std::complex<double>(0.0, 0.0);

    // Inverse DFT to get analytic signal
    for (int t = 0; t < n; ++t) {
        std::complex<double> sum(0.0, 0.0);
        for (int k = 0; k < n; ++k) {
            double angle = 2.0 * M_PI * k * t / n;
            sum += H[static_cast<size_t>(k)] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        sum /= static_cast<double>(n);
        envelope[t] = std::abs(sum);
    }
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd ConnectivityAec::compute(const MatrixXd& matData)
{
    const int nSig = static_cast<int>(matData.rows());
    const int nSamples = static_cast<int>(matData.cols());

    MatrixXd aec = MatrixXd::Identity(nSig, nSig);

    if (nSig < 2 || nSamples < 2)
        return aec;

    // Compute envelopes
    MatrixXd envs(nSig, nSamples);
    for (int i = 0; i < nSig; ++i) {
        envs.row(i) = hilbertEnvelope(matData.row(i).transpose()).transpose();
    }

    // Pairwise correlation of envelopes
    for (int i = 0; i < nSig; ++i) {
        for (int j = i + 1; j < nSig; ++j) {
            double r = pearsonCorrelation(envs.row(i).transpose(), envs.row(j).transpose());
            aec(i, j) = r;
            aec(j, i) = r;
        }
    }

    return aec;
}

//=============================================================================================================

MatrixXd ConnectivityAec::computeOrthogonalized(const MatrixXd& matData)
{
    const int nSig = static_cast<int>(matData.rows());
    const int nSamples = static_cast<int>(matData.cols());

    MatrixXd aec = MatrixXd::Identity(nSig, nSig);

    if (nSig < 2 || nSamples < 2)
        return aec;

    // For orthogonalized AEC, we orthogonalize signal j w.r.t. i, then correlate envelopes
    for (int i = 0; i < nSig; ++i) {
        VectorXd si = matData.row(i).transpose();
        VectorXd envI = hilbertEnvelope(si);

        for (int j = i + 1; j < nSig; ++j) {
            VectorXd sj = matData.row(j).transpose();

            // Orthogonalize j w.r.t. i: sj_orth = sj - (sj·si / si·si) * si
            double siNorm2 = si.squaredNorm();
            VectorXd sjOrth_ij;
            if (siNorm2 > 1e-30)
                sjOrth_ij = sj - (sj.dot(si) / siNorm2) * si;
            else
                sjOrth_ij = sj;

            VectorXd envJOrth = hilbertEnvelope(sjOrth_ij);
            double r_ij = std::abs(pearsonCorrelation(envI, envJOrth));

            // Orthogonalize i w.r.t. j
            double sjNorm2 = sj.squaredNorm();
            VectorXd siOrth_ji;
            if (sjNorm2 > 1e-30)
                siOrth_ji = si - (si.dot(sj) / sjNorm2) * sj;
            else
                siOrth_ji = si;

            VectorXd envJ = hilbertEnvelope(sj);
            VectorXd envIOrth = hilbertEnvelope(siOrth_ji);
            double r_ji = std::abs(pearsonCorrelation(envIOrth, envJ));

            // Symmetrise
            double r = (r_ij + r_ji) / 2.0;
            aec(i, j) = r;
            aec(j, i) = r;
        }
    }

    return aec;
}

//=============================================================================================================

VectorXd ConnectivityAec::hilbertEnvelope(const VectorXd& signal)
{
    VectorXd env;
    dftHilbert(signal, env);
    return env;
}

//=============================================================================================================

double ConnectivityAec::pearsonCorrelation(const VectorXd& a, const VectorXd& b)
{
    const int n = static_cast<int>(a.size());
    if (n < 2 || b.size() != n)
        return 0.0;

    double meanA = a.mean();
    double meanB = b.mean();

    VectorXd ac = a.array() - meanA;
    VectorXd bc = b.array() - meanB;

    double stdA = std::sqrt(ac.squaredNorm() / static_cast<double>(n - 1));
    double stdB = std::sqrt(bc.squaredNorm() / static_cast<double>(n - 1));

    if (stdA < 1e-15 || stdB < 1e-15)
        return 0.0;

    return ac.dot(bc) / (static_cast<double>(n - 1) * stdA * stdB);
}
