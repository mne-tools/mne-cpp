//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     connectivity_aec.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    ConnectivityAec class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "connectivity_aec.h"

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

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

VectorXd ConnectivityAec::hilbertEnvelope(const VectorXd& signal, int nFft)
{
    const Index n = signal.size();
    const Index nPad = std::max<Index>(nFft, n);
    if (n == 0) {
        return VectorXd();
    }
    VectorXd padded = VectorXd::Zero(nPad);
    padded.head(n) = signal;

    FFT<double> fft;
    fft.SetFlag(FFT<double>::HalfSpectrum);
    VectorXcd half;
    fft.fwd(half, padded);

    // Analytic spectrum: DC and (even length) Nyquist kept, positive frequencies doubled, negative zeroed
    VectorXcd spec = VectorXcd::Zero(nPad);
    spec.head(half.size()) = half;
    spec.segment(1, (nPad - 1) / 2) *= 2.0;

    fft.ClearFlag(FFT<double>::HalfSpectrum);
    VectorXcd analytic;
    fft.inv(analytic, spec);
    return analytic.head(n).cwiseAbs();
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
