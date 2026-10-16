//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     morlet_tfr.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of MorletTfr.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "morlet_tfr.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif
#include <unsupported/Eigen/FFT>

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

namespace
{
constexpr double MORLET_PI = 3.14159265358979323846;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

int MorletTfr::nextPow2(int n)
{
    int p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

//=============================================================================================================

VectorXcd MorletTfr::buildWavelet(double dFreq, double dSFreq, double dNCycles, int& halfLen, bool bZeroMean)
{
    const double sigma_t = dNCycles / (2.0 * MORLET_PI * dFreq);

    // Same grid as np.arange(0.0, 5.0 * sigma_t, 1.0 / sfreq), mirrored around t = 0.
    const double step = 1.0 / dSFreq;
    halfLen = static_cast<int>(std::ceil(5.0 * sigma_t / step)) - 1;

    const int nWav = 2 * halfLen + 1;
    VectorXcd wavelet(nWav);
    const double realOffset = bZeroMean ? std::exp(-2.0 * std::pow(MORLET_PI * dFreq * sigma_t, 2)) : 0.0;

    for (int i = 0; i < nWav; ++i) {
        const double t = (i - halfLen) * step;
        const double gauss = std::exp(-t * t / (2.0 * sigma_t * sigma_t));
        const double phase = 2.0 * MORLET_PI * dFreq * t;
        wavelet[i] = gauss * std::complex<double>(std::cos(phase) - realOffset, std::sin(phase));
    }
    // Norm sqrt(2) for the complex wavelet, i.e. norm 1 for its real part.
    wavelet /= std::sqrt(0.5) * wavelet.norm();
    return wavelet;
}

//=============================================================================================================

MorletTfrResult MorletTfr::compute(const RowVectorXd& vecData,
                                   double dSFreq,
                                   const RowVectorXd& vecFreqs,
                                   double dNCycles)
{
    const int nTimes = static_cast<int>(vecData.cols());
    const int nFreqs = static_cast<int>(vecFreqs.cols());

    MorletTfrResult result;
    result.matPower.resize(nFreqs, nTimes);
    result.vecFreqs = vecFreqs;

    // Pre-compute forward FFT of the (real) signal at the maximum needed convolution length.
    // For each frequency the wavelet may differ in length; recompute convolution per frequency.
    Eigen::FFT<double> fft;

    for (int fi = 0; fi < nFreqs; ++fi) {
        int halfLen = 0;
        const VectorXcd wavelet = buildWavelet(vecFreqs[fi], dSFreq, dNCycles, halfLen, true);

        const int nWav = static_cast<int>(wavelet.size());
        const int nConv = nextPow2(nTimes + nWav - 1);

        // --- FFT of zero-padded real signal ---
        VectorXd sigPad = VectorXd::Zero(nConv);
        sigPad.head(nTimes) = vecData.transpose();
        VectorXcd sigSpec;
        fft.fwd(sigSpec, sigPad);

        // --- FFT of zero-padded complex wavelet (real & imag parts separately) ---
        VectorXd wavReal = VectorXd::Zero(nConv);
        VectorXd wavImag = VectorXd::Zero(nConv);
        for (int k = 0; k < nWav; ++k) {
            wavReal[k] = wavelet[k].real();
            wavImag[k] = wavelet[k].imag();
        }
        VectorXcd wavSpecR, wavSpecI;
        fft.fwd(wavSpecR, wavReal);
        fft.fwd(wavSpecI, wavImag);
        // Combine: FFT(real + i·imag) = FFT(real) + i·FFT(imag)
        VectorXcd wavSpec = wavSpecR + std::complex<double>(0.0, 1.0) * wavSpecI;

        // --- Multiply spectra and inverse FFT ---
        VectorXcd product = sigSpec.array() * wavSpec.array();
        VectorXcd conv;
        fft.inv(conv, product);

        // Trim to "same" length: skip the first halfLen samples (linear → same convolution)
        for (int t = 0; t < nTimes; ++t)
            result.matPower(fi, t) = std::norm(conv[t + halfLen]);
    }

    return result;
}

//=============================================================================================================

QVector<MorletTfrResult> MorletTfr::computeMultiChannel(const MatrixXd& matData,
                                                        double dSFreq,
                                                        const RowVectorXd& vecFreqs,
                                                        double dNCycles,
                                                        const RowVectorXi& vecPicks)
{
    std::vector<int> picks;
    if (vecPicks.size() > 0) {
        picks.reserve(static_cast<std::size_t>(vecPicks.size()));
        for (int i = 0; i < vecPicks.size(); ++i)
            picks.push_back(vecPicks[i]);
    } else {
        picks.reserve(static_cast<std::size_t>(matData.rows()));
        for (int i = 0; i < static_cast<int>(matData.rows()); ++i)
            picks.push_back(i);
    }

    QVector<MorletTfrResult> results;
    results.reserve(static_cast<int>(picks.size()));
    for (int ch : picks)
        results.append(compute(matData.row(ch), dSFreq, vecFreqs, dNCycles));
    return results;
}
