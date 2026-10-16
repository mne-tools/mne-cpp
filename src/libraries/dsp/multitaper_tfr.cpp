//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     multitaper_tfr.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of MultitaperTfr.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multitaper_tfr.h"
#include "dpss.h"

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
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultitaperTfrResult MultitaperTfr::compute(const MatrixXd& matData,
                                           double sfreq,
                                           int windowSize,
                                           int stepSize,
                                           double halfBandwidth,
                                           int nTapers)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());

    if (stepSize < 0)
        stepSize = windowSize / 2;

    const int nFreqs = windowSize / 2 + 1;

    // 1. Compute DPSS tapers for the window size
    DpssResult dpss = Dpss::compute(windowSize, halfBandwidth, nTapers);
    const int nTap = static_cast<int>(dpss.matTapers.rows());

    // Compute eigenvalue weight sum
    double weightSum = 0.0;
    for (int t = 0; t < nTap; ++t)
        weightSum += dpss.vecEigenvalues[t];

    // 2. Determine number of time steps and build time/freq axes
    int nSteps = 0;
    for (int start = 0; start + windowSize <= nTimes; start += stepSize)
        ++nSteps;

    MultitaperTfrResult result;

    // Frequency axis
    result.vecFreqs.resize(nFreqs);
    for (int k = 0; k < nFreqs; ++k)
        result.vecFreqs[k] = static_cast<double>(k) * sfreq / static_cast<double>(windowSize);

    // Time axis: centre of each window in seconds
    result.vecTimes.resize(nSteps);
    for (int s = 0; s < nSteps; ++s) {
        const int start = s * stepSize;
        const double centre = static_cast<double>(start) + static_cast<double>(windowSize - 1) / 2.0;
        result.vecTimes[s] = centre / sfreq;
    }

    // 3. Compute TFR for each channel
    Eigen::FFT<double> fft;

    result.tfrData.reserve(nChannels);
    for (int ch = 0; ch < nChannels; ++ch) {
        MatrixXd tfr = MatrixXd::Zero(nFreqs, nSteps);

        int stepIdx = 0;
        for (int start = 0; start + windowSize <= nTimes; start += stepSize) {
            // Extract window segment
            VectorXd segment = matData.row(ch).segment(start, windowSize).transpose();

            // Apply each taper, FFT, accumulate weighted |X|^2
            RowVectorXd psd = RowVectorXd::Zero(nFreqs);

            for (int t = 0; t < nTap; ++t) {
                VectorXd tapered = segment.array() * dpss.matTapers.row(t).transpose().array();

                VectorXcd spec;
                fft.fwd(spec, tapered);

                const double w = dpss.vecEigenvalues[t];
                for (int k = 0; k < nFreqs; ++k)
                    psd[k] += w * std::norm(spec[k]);
            }

            // Normalise: divide by (weightSum × sfreq)
            const double dNorm = 1.0 / (weightSum * sfreq);
            psd *= dNorm;

            // One-sided: double all non-DC, non-Nyquist bins
            for (int k = 1; k < nFreqs - 1; ++k)
                psd[k] *= 2.0;

            tfr.col(stepIdx) = psd.transpose();
            ++stepIdx;
        }

        result.tfrData.append(tfr);
    }

    return result;
}

//=============================================================================================================

MultitaperTfrResult MultitaperTfr::computeArray(const MatrixXd& matData,
                                                double sfreq,
                                                const RowVectorXd& vecFreqs,
                                                double nCycles,
                                                double timeBandwidth,
                                                bool zeroMean)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes = static_cast<int>(matData.cols());
    const int nFreqs = static_cast<int>(vecFreqs.size());
    const int nTap = static_cast<int>(std::floor(timeBandwidth - 1.0));

    MultitaperTfrResult result;
    result.vecFreqs = vecFreqs;
    result.vecTimes = RowVectorXd::LinSpaced(nTimes, 0.0, (nTimes - 1) / sfreq);
    for (int ch = 0; ch < nChannels; ++ch)
        result.tfrData.append(MatrixXd::Zero(nFreqs, nTimes));

    for (int fi = 0; fi < nFreqs; ++fi) {
        const double f = vecFreqs[fi];
        const double tWin = nCycles / f;
        // Length of np.arange(0.0, t_win, 1.0 / sfreq)
        const int nWav = static_cast<int>(std::ceil(tWin / (1.0 / sfreq)));
        const DpssResult dpss = Dpss::compute(nWav, timeBandwidth / 2.0, nTap, false);

        double weightSum = 0.0;
        for (int m = 0; m < nTap; ++m) {
            // Oscillation centred on the window before tapering
            VectorXcd wavelet(nWav);
            for (int k = 0; k < nWav; ++k) {
                const double phase = 2.0 * std::numbers::pi * f * (k * (1.0 / sfreq) - tWin / 2.0);
                wavelet[k] = dpss.matTapers(m, k) * std::complex<double>(std::cos(phase), std::sin(phase));
            }
            if (zeroMean)
                wavelet.array() -= wavelet.mean();
            wavelet /= std::sqrt(0.5) * wavelet.norm();

            const double weight = dpss.vecEigenvalues[m]; // squared sqrt(concentration)
            weightSum += weight;

            // 'same' convolution: centre of the full convolution
            const int offset = (nWav - 1) / 2;
            for (int ch = 0; ch < nChannels; ++ch) {
                for (int t = 0; t < nTimes; ++t) {
                    std::complex<double> sum(0.0, 0.0);
                    const int n = t + offset;
                    for (int k = std::max(0, n - nTimes + 1); k <= std::min(nWav - 1, n); ++k)
                        sum += matData(ch, n - k) * wavelet[k];
                    result.tfrData[ch](fi, t) += weight * std::norm(sum);
                }
            }
        }
        if (nTap > 1)
            for (int ch = 0; ch < nChannels; ++ch)
                result.tfrData[ch].row(fi) *= 2.0 / weightSum;
    }

    return result;
}
