//=============================================================================================================
/**
 * @file     csd.cpp
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
 * @brief    Implementation of Csd — Cross-Spectral Density computation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "csd.h"
#include "dpss.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>
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
constexpr double CSD_PI = 3.14159265358979323846;
}

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CsdResult Csd::computeMultitaper(const MatrixXd& matData,
                                 double          sfreq,
                                 double          fmin,
                                 double          fmax,
                                 double          halfBandwidth,
                                 int             nTapers)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes    = static_cast<int>(matData.cols());
    const int nFreqsFull = nTimes / 2 + 1;

    if (fmax < 0.0)
        fmax = sfreq / 2.0;

    // 1. Compute DPSS tapers
    DpssResult dpss = Dpss::compute(nTimes, halfBandwidth, nTapers);
    const int nTap = static_cast<int>(dpss.matTapers.rows());

    // Compute eigenvalue weight sum
    double weightSum = 0.0;
    for (int t = 0; t < nTap; ++t)
        weightSum += dpss.vecEigenvalues[t];

    // Build full frequency axis
    const double freqRes = sfreq / static_cast<double>(nTimes);

    // Determine frequency bin range
    const int iBinMin = std::max(0, static_cast<int>(std::ceil(fmin / freqRes)));
    const int iBinMax = std::min(nFreqsFull - 1, static_cast<int>(std::floor(fmax / freqRes)));
    const int nFreqsSel = iBinMax - iBinMin + 1;

    // Initialize per-frequency CSD accumulators
    std::vector<MatrixXcd> csdAccum(static_cast<std::size_t>(nFreqsSel),
                                    MatrixXcd::Zero(nChannels, nChannels));

    Eigen::FFT<double> fft;

    // 2. For each taper, compute FFT for all channels, then accumulate CSD
    for (int t = 0; t < nTap; ++t) {
        const double w = dpss.vecEigenvalues[t];

        // Compute FFT spectra for all channels with this taper
        MatrixXcd spectra(nChannels, nFreqsFull);

        for (int ch = 0; ch < nChannels; ++ch) {
            VectorXd tapered = matData.row(ch).transpose().array()
                             * dpss.matTapers.row(t).transpose().array();

            VectorXcd spec;
            fft.fwd(spec, tapered);

            spectra.row(ch) = spec.head(nFreqsFull).transpose();
        }

        // Accumulate weighted CSD: X(:,f) * X(:,f)^H for selected frequency bins
        for (int fi = 0; fi < nFreqsSel; ++fi) {
            const int fBin = iBinMin + fi;
            VectorXcd col = spectra.col(fBin);
            csdAccum[static_cast<std::size_t>(fi)] += w * (col * col.adjoint());
        }
    }

    // 3. Assemble result
    CsdResult result;
    result.vecFreqs.resize(nFreqsSel);
    result.csdByFreq.resize(nFreqsSel);

    MatrixXcd meanCsd = MatrixXcd::Zero(nChannels, nChannels);

    for (int fi = 0; fi < nFreqsSel; ++fi) {
        csdAccum[static_cast<std::size_t>(fi)] /= weightSum;
        result.csdByFreq[fi] = csdAccum[static_cast<std::size_t>(fi)];
        result.vecFreqs[fi] = (iBinMin + fi) * freqRes;
        meanCsd += result.csdByFreq[fi];
    }

    if (nFreqsSel > 0)
        meanCsd /= static_cast<double>(nFreqsSel);

    result.matCsd = meanCsd;
    return result;
}

//=============================================================================================================

CsdResult Csd::computeFourier(const MatrixXd& matData,
                               double          sfreq,
                               double          fmin,
                               double          fmax,
                               int             nFft,
                               double          overlap)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes    = static_cast<int>(matData.cols());

    if (nFft > nTimes)
        nFft = nTimes;

    const int nFreqsFull = nFft / 2 + 1;

    if (fmax < 0.0)
        fmax = sfreq / 2.0;

    const double freqRes = sfreq / static_cast<double>(nFft);
    const int iBinMin = std::max(0, static_cast<int>(std::ceil(fmin / freqRes)));
    const int iBinMax = std::min(nFreqsFull - 1, static_cast<int>(std::floor(fmax / freqRes)));
    const int nFreqsSel = iBinMax - iBinMin + 1;

    // Build Hann window
    VectorXd hannWin(nFft);
    for (int n = 0; n < nFft; ++n) {
        const double t = static_cast<double>(n) / static_cast<double>(nFft - 1);
        hannWin[n] = 0.5 * (1.0 - std::cos(2.0 * CSD_PI * t));
    }
    const double winPow = hannWin.squaredNorm();

    const int iStep = std::max(1, static_cast<int>(std::round(
                          static_cast<double>(nFft) * (1.0 - overlap))));

    // Initialize per-frequency CSD accumulators
    std::vector<MatrixXcd> csdAccum(static_cast<std::size_t>(nFreqsSel),
                                    MatrixXcd::Zero(nChannels, nChannels));

    Eigen::FFT<double> fft;
    int nSeg = 0;

    // Process each segment
    for (int start = 0; start + nFft <= nTimes; start += iStep) {
        // FFT all channels for this segment
        MatrixXcd spectra(nChannels, nFreqsFull);

        for (int ch = 0; ch < nChannels; ++ch) {
            VectorXd seg = matData.row(ch).segment(start, nFft).transpose().array()
                         * hannWin.array();

            VectorXcd spec;
            fft.fwd(spec, seg);

            spectra.row(ch) = spec.head(nFreqsFull).transpose();
        }

        // Accumulate CSD for selected frequency bins
        for (int fi = 0; fi < nFreqsSel; ++fi) {
            const int fBin = iBinMin + fi;
            VectorXcd col = spectra.col(fBin);
            csdAccum[static_cast<std::size_t>(fi)] += col * col.adjoint();
        }

        ++nSeg;
    }

    // 3. Assemble result
    CsdResult result;
    result.vecFreqs.resize(nFreqsSel);
    result.csdByFreq.resize(nFreqsSel);

    MatrixXcd meanCsd = MatrixXcd::Zero(nChannels, nChannels);

    const double dNorm = (nSeg > 0) ? 1.0 / (static_cast<double>(nSeg) * winPow) : 0.0;

    for (int fi = 0; fi < nFreqsSel; ++fi) {
        csdAccum[static_cast<std::size_t>(fi)] *= dNorm;
        result.csdByFreq[fi] = csdAccum[static_cast<std::size_t>(fi)];
        result.vecFreqs[fi] = (iBinMin + fi) * freqRes;
        meanCsd += result.csdByFreq[fi];
    }

    if (nFreqsSel > 0)
        meanCsd /= static_cast<double>(nFreqsSel);

    result.matCsd = meanCsd;
    return result;
}

//=============================================================================================================

CsdResult Csd::computeMorlet(const MatrixXd&      matData,
                              double               sfreq,
                              const RowVectorXd&   frequencies,
                              int                  nCycles)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes    = static_cast<int>(matData.cols());
    const int nFreqs    = static_cast<int>(frequencies.size());

    CsdResult result;
    result.vecFreqs = frequencies;
    result.csdByFreq.resize(nFreqs);

    MatrixXcd meanCsd = MatrixXcd::Zero(nChannels, nChannels);

    for (int fi = 0; fi < nFreqs; ++fi) {
        const double f = frequencies[fi];
        const double sigma = static_cast<double>(nCycles) / (2.0 * CSD_PI * f);

        // Determine wavelet length: ±3σ in samples
        const int halfLen = static_cast<int>(std::ceil(3.0 * sigma * sfreq));
        const int wavLen  = 2 * halfLen + 1;

        // Build Morlet wavelet
        VectorXcd wavelet(wavLen);
        double normFactor = 0.0;
        for (int n = 0; n < wavLen; ++n) {
            const double t = (static_cast<double>(n) - static_cast<double>(halfLen)) / sfreq;
            const double gauss = std::exp(-t * t / (2.0 * sigma * sigma));
            wavelet[n] = std::complex<double>(gauss * std::cos(2.0 * CSD_PI * f * t),
                                              gauss * std::sin(2.0 * CSD_PI * f * t));
            normFactor += gauss * gauss;
        }
        // Normalize wavelet for unit energy
        normFactor = std::sqrt(normFactor);
        if (normFactor > 0.0)
            wavelet /= normFactor;

        // Convolve each channel with the wavelet → complex analytic signal
        MatrixXcd analytic(nChannels, nTimes);

        for (int ch = 0; ch < nChannels; ++ch) {
            for (int t = 0; t < nTimes; ++t) {
                std::complex<double> sum(0.0, 0.0);
                for (int k = 0; k < wavLen; ++k) {
                    const int idx = t - halfLen + k;
                    if (idx >= 0 && idx < nTimes) {
                        sum += matData(ch, idx) * std::conj(wavelet[k]);
                    }
                }
                analytic(ch, t) = sum;
            }
        }

        // CSD at this frequency = mean over time of C(:,t) * C(:,t)^H
        MatrixXcd csdAtFreq = MatrixXcd::Zero(nChannels, nChannels);
        for (int t = 0; t < nTimes; ++t) {
            VectorXcd col = analytic.col(t);
            csdAtFreq += col * col.adjoint();
        }
        csdAtFreq /= static_cast<double>(nTimes);

        result.csdByFreq[fi] = csdAtFreq;
        meanCsd += csdAtFreq;
    }

    if (nFreqs > 0)
        meanCsd /= static_cast<double>(nFreqs);

    result.matCsd = meanCsd;
    return result;
}
