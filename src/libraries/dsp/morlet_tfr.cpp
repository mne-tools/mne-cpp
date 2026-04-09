//=============================================================================================================
/**
 * @file     morlet_tfr.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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
    while (p < n) p <<= 1;
    return p;
}

//=============================================================================================================

VectorXcd MorletTfr::buildWavelet(double dFreq, double dSFreq, double dNCycles, int& halfLen)
{
    // Time-domain standard deviation: σ_t = nCycles / (2π·f)
    const double sigma_t = dNCycles / (2.0 * MORLET_PI * dFreq);

    // Truncate at ±4σ — captures > 99.99 % of energy
    halfLen = static_cast<int>(std::round(4.0 * sigma_t * dSFreq));

    const int    nWav = 2 * halfLen + 1;
    VectorXcd    wavelet(nWav);

    // L2-energy normalisation: A = (σ_t · √(2π))^(-0.5)
    const double A = std::pow(sigma_t * std::sqrt(2.0 * MORLET_PI), -0.5);

    for (int i = 0; i < nWav; ++i) {
        const double t     = static_cast<double>(i - halfLen) / dSFreq;
        const double gauss = std::exp(-t * t / (2.0 * sigma_t * sigma_t));
        const double phase = 2.0 * MORLET_PI * dFreq * t;
        wavelet[i] = std::complex<double>(A * gauss * std::cos(phase),
                                          A * gauss * std::sin(phase));
    }
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
        const VectorXcd wavelet = buildWavelet(vecFreqs[fi], dSFreq, dNCycles, halfLen);

        const int nWav  = static_cast<int>(wavelet.size());
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

QVector<MorletTfrResult> MorletTfr::computeMultiChannel(const MatrixXd&    matData,
                                                          double             dSFreq,
                                                          const RowVectorXd& vecFreqs,
                                                          double             dNCycles,
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
