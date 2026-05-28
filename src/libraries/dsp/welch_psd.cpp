//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file welch_psd.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of WelchPsd.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "welch_psd.h"

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
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

namespace
{
constexpr double WELCH_PI = 3.14159265358979323846;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

VectorXd WelchPsd::buildWindow(int iN, WindowType window)
{
    VectorXd w(iN);
    const double pi2 = 2.0 * WELCH_PI;

    for (int n = 0; n < iN; ++n) {
        const double t = static_cast<double>(n) / static_cast<double>(iN - 1);
        switch (window) {
        case Hann:
            w[n] = 0.5 * (1.0 - std::cos(pi2 * t));
            break;
        case Hamming:
            w[n] = 0.54 - 0.46 * std::cos(pi2 * t);
            break;
        case Blackman:
            w[n] = 0.42
                 - 0.5  * std::cos(      pi2 * t)
                 + 0.08 * std::cos(2.0 * pi2 * t);
            break;
        case FlatTop:
            w[n] = 1.0
                 - 1.93293488969 * std::cos(      pi2 * t)
                 + 1.28349769674 * std::cos(2.0 * pi2 * t)
                 - 0.38763473916 * std::cos(3.0 * pi2 * t)
                 + 0.03279543650 * std::cos(4.0 * pi2 * t);
            break;
        }
    }
    return w;
}

//=============================================================================================================

RowVectorXd WelchPsd::freqAxis(int iNfft, double dSFreq)
{
    const int iNFreqs = iNfft / 2 + 1;
    RowVectorXd freqs(iNFreqs);
    for (int k = 0; k < iNFreqs; ++k)
        freqs[k] = static_cast<double>(k) * dSFreq / static_cast<double>(iNfft);
    return freqs;
}

//=============================================================================================================

RowVectorXd WelchPsd::computeVector(const RowVectorXd& vecData,
                                     double     dSFreq,
                                     int        iNfft,
                                     double     dOverlap,
                                     WindowType window)
{
    const int nIn = static_cast<int>(vecData.cols());
    if (iNfft > nIn) iNfft = nIn;

    const int iNFreqs = iNfft / 2 + 1;
    const int iStep   = std::max(1, static_cast<int>(std::round(
                            static_cast<double>(iNfft) * (1.0 - dOverlap))));

    const VectorXd w       = buildWindow(iNfft, window);
    const double   dWinPow = w.squaredNorm();  // Σ w²

    Eigen::FFT<double> fft;
    RowVectorXd psd = RowVectorXd::Zero(iNFreqs);
    int nSeg = 0;

    for (int start = 0; start + iNfft <= nIn; start += iStep) {
        // Apply window and forward FFT
        VectorXd seg = vecData.segment(start, iNfft).transpose().array() * w.array();
        VectorXcd spec;
        fft.fwd(spec, seg);

        for (int k = 0; k < iNFreqs; ++k)
            psd[k] += std::norm(spec[k]);  // accumulate |X[k]|²
        ++nSeg;
    }

    if (nSeg == 0)
        return RowVectorXd::Zero(iNFreqs);

    // Normalise: divide by (nSeg × window_power × sfreq)
    const double dNorm = 1.0 / (static_cast<double>(nSeg) * dWinPow * dSFreq);
    psd *= dNorm;

    // One-sided: double all non-DC, non-Nyquist bins
    for (int k = 1; k < iNFreqs - 1; ++k)
        psd[k] *= 2.0;

    return psd;
}

//=============================================================================================================

WelchPsdResult WelchPsd::compute(const MatrixXd&    matData,
                                  double             dSFreq,
                                  int                iNfft,
                                  double             dOverlap,
                                  WindowType         window,
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

    const int iNFreqs = iNfft / 2 + 1;
    WelchPsdResult result;
    result.matPsd.resize(static_cast<int>(picks.size()), iNFreqs);
    result.vecFreqs = freqAxis(iNfft, dSFreq);

    for (int ci = 0; ci < static_cast<int>(picks.size()); ++ci)
        result.matPsd.row(ci) = computeVector(matData.row(picks[ci]),
                                              dSFreq, iNfft, dOverlap, window);
    return result;
}
