//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     firfilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of FirFilter.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "firfilter.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

FilterKernel FirFilter::design(int iOrder,
                               FilterType type,
                               double dCutoffLow,
                               double dCutoffHigh,
                               double dSFreq,
                               double dTransition,
                               DesignMethod method)
{
    // FilterKernel frequency encoding (all values normalised to Nyquist = sFreq/2):
    //
    //   LPF  : dCenterfreq = dCutoffLow  / nyquist,  dBandwidth = 0
    //   HPF  : dCenterfreq = dCutoffLow  / nyquist,  dBandwidth = 0
    //   BPF  : dCenterfreq = (lo+hi)     / sFreq,    dBandwidth = (hi-lo) / nyquist
    //   NOTCH: dCenterfreq = (lo+hi)     / sFreq,    dBandwidth = (hi-lo) / nyquist
    //
    // dParkswidth = dTransition / nyquist

    const double nyquist = dSFreq / 2.0;

    double dCenterfreq = 0.0;
    double dBandwidth = 0.0;
    const double dParkswidth = dTransition / nyquist;

    int iFilterType = static_cast<int>(type); // LPF=0, HPF=1, BPF=2, NOTCH=3

    switch (type) {
        case LowPass:
        case HighPass:
            dCenterfreq = dCutoffLow / nyquist;
            dBandwidth = 0.0;
            break;
        case BandPass:
        case BandStop:
            dCenterfreq = (dCutoffLow + dCutoffHigh) / dSFreq; // = centre / nyquist normalised to [0,1]
            dBandwidth = (dCutoffHigh - dCutoffLow) / nyquist;
            break;
    }

    QString sName;
    switch (type) {
        case LowPass:
            sName = QStringLiteral("LP_%1Hz").arg(dCutoffLow);
            break;
        case HighPass:
            sName = QStringLiteral("HP_%1Hz").arg(dCutoffLow);
            break;
        case BandPass:
            sName = QStringLiteral("BP_%1-%2Hz").arg(dCutoffLow).arg(dCutoffHigh);
            break;
        case BandStop:
            sName = QStringLiteral("BS_%1-%2Hz").arg(dCutoffLow).arg(dCutoffHigh);
            break;
    }

    return FilterKernel(sName,
                        iFilterType,
                        iOrder,
                        dCenterfreq,
                        dBandwidth,
                        dParkswidth,
                        dSFreq,
                        static_cast<int>(method));
}

//=============================================================================================================

RowVectorXd FirFilter::apply(const RowVectorXd& vecData,
                             FilterKernel& kernel)
{
    RowVectorXd work = vecData;
    kernel.applyFftFilter(work, /*bKeepOverhead=*/false);
    return work;
}

//=============================================================================================================

RowVectorXd FirFilter::applyZeroPhase(const RowVectorXd& vecData,
                                      FilterKernel& kernel)
{
    // Forward pass
    RowVectorXd work = vecData;
    kernel.applyFftFilter(work, false);

    // Reverse pass
    RowVectorXd rev = work.reverse();
    kernel.applyFftFilter(rev, false);

    return rev.reverse();
}

//=============================================================================================================

MatrixXd FirFilter::applyZeroPhaseMatrix(const MatrixXd& matData,
                                         FilterKernel& kernel,
                                         const RowVectorXi& vecPicks)
{
    MatrixXd result = matData;

    if (vecPicks.size() == 0) {
        // All rows
        for (int i = 0; i < result.rows(); ++i) {
            RowVectorXd row = result.row(i);
            result.row(i) = applyZeroPhase(row, kernel);
        }
    } else {
        for (int k = 0; k < vecPicks.size(); ++k) {
            int i = vecPicks(k);
            if (i < 0 || i >= result.rows())
                continue;
            RowVectorXd row = result.row(i);
            result.row(i) = applyZeroPhase(row, kernel);
        }
    }

    return result;
}

//=============================================================================================================

RowVectorXd FirFilter::designMne(double dSFreq, double dLFreq, double dHFreq)
{
    // Adapted from mne.filter create_filter / _triage_filter_params / _firwin_design
    // (MNE-Python, BSD-3-Clause), fir_window="hamming", fir_design="firwin", phase="zero".
    const double nyquist = dSFreq / 2.0;
    const bool highPass = dLFreq > 0.0;
    const bool lowPass = dHFreq > 0.0;
    const double lTrans = highPass ? std::min(std::max(0.25 * dLFreq, 2.0), dLFreq) : 0.0;
    const double hTrans = lowPass ? std::min(std::max(0.25 * dHFreq, 2.0), nyquist - dHFreq) : 0.0;
    const double narrowest = std::min(highPass ? lTrans : INFINITY, lowPass ? hTrans : INFINITY);

    int nTaps = std::max(static_cast<int>(std::ceil(3.3 / narrowest * dSFreq)), 1);
    nTaps += (nTaps - 1) % 2;

    // Piecewise-constant gain as (frequency, gain) points from 0 to Nyquist
    std::vector<double> freq{0.0};
    std::vector<double> gain{highPass ? 0.0 : 1.0};
    if (highPass) {
        freq.insert(freq.end(), {dLFreq - lTrans, dLFreq});
        gain.insert(gain.end(), {0.0, 1.0});
    }
    if (lowPass) {
        freq.insert(freq.end(), {dHFreq, dHFreq + hTrans});
        gain.insert(gain.end(), {1.0, 0.0});
    }
    freq.push_back(nyquist);
    gain.push_back(gain.back());

    // Each gain step is one windowed-sinc low-pass of the length its transition needs, centred.
    RowVectorXd h = RowVectorXd::Zero(nTaps);
    if (gain.back() == 1.0) {
        h(nTaps / 2) = 1.0;
    }
    for (int k = static_cast<int>(freq.size()) - 2; k >= 0; --k) {
        if (gain[k] == gain[k + 1]) {
            continue;
        }
        const double transition = (freq[k + 1] - freq[k]) / nyquist / 2.0;
        int nStep = static_cast<int>(std::nearbyint(3.3 / transition)); // Python round(): half to even
        nStep += 1 - nStep % 2;
        const double cutoff = (freq[k + 1] + freq[k]) / 2.0 / nyquist;
        RowVectorXd step(nStep);
        for (int n = 0; n < nStep; ++n) {
            const double m = n - (nStep - 1) / 2.0;
            const double sinc = m == 0.0 ? 1.0 : std::sin(M_PI * cutoff * m) / (M_PI * cutoff * m);
            const double window = nStep > 1 ? 0.54 - 0.46 * std::cos(2.0 * M_PI * n / (nStep - 1)) : 1.0;
            step(n) = cutoff * sinc * window;
        }
        step /= step.sum();
        const int offset = (nTaps - nStep) / 2;
        h.segment(offset, nStep) += (gain[k] == 0.0 ? -1.0 : 1.0) * step;
    }
    return h;
}

//=============================================================================================================

MatrixXd FirFilter::filterData(const MatrixXd& matData, double dSFreq, double dLFreq, double dHFreq)
{
    const RowVectorXd h = designMne(dSFreq, dLFreq, dHFreq);
    return overlapAddZeroPhase(matData, h, std::min<Index>(h.size(), matData.cols()) - 1);
}

//=============================================================================================================

RowVectorXd FirFilter::firwin2(int nTaps, const std::vector<double>& freq, const std::vector<double>& gain, FirWindow window)
{
    // Adapted from scipy.signal.firwin2 (SciPy, BSD-3-Clause) for nyq = 1, no antisymmetry.
    const int nFreqs = 1 + (1 << static_cast<int>(std::ceil(std::log2(nTaps))));
    RowVectorXcd fx(nFreqs);
    std::size_t k = 0;
    for (int i = 0; i < nFreqs; ++i) {
        const double x = static_cast<double>(i) / (nFreqs - 1);
        while (k + 2 < freq.size() && x > freq[k + 1])
            ++k;
        const double span = freq[k + 1] - freq[k];
        const double g = span > 0.0 ? gain[k] + (gain[k + 1] - gain[k]) * (x - freq[k]) / span : gain[k + 1];
        const double phase = -(nTaps - 1) / 2.0 * M_PI * x;
        fx(i) = g * std::complex<double>(std::cos(phase), std::sin(phase));
    }
    FFT<double> fft;
    fft.SetFlag(FFT<double>::HalfSpectrum);
    RowVectorXd full;
    fft.inv(full, fx, 2 * (nFreqs - 1));

    const double a0 = window == FirWindow::Hamming ? 0.54 : 0.5;
    RowVectorXd h(nTaps);
    for (int n = 0; n < nTaps; ++n) {
        const double w = nTaps > 1 ? a0 - (1.0 - a0) * std::cos(2.0 * M_PI * n / (nTaps - 1)) : 1.0;
        h(n) = full(n) * w;
    }
    return h;
}

//=============================================================================================================

MatrixXd FirFilter::filterDataZeroDouble(const MatrixXd& matData, double dSFreq, double dLFreq, double dHFreq, FirWindow window)
{
    // Adapted from mne.filter._triage_filter_params / _construct_fir_filter (MNE-Python, BSD-3-Clause):
    // filter_length "10s" rounded up to a power of two for zero-double, 0.5 Hz transition bands.
    const int nTaps = 1 << static_cast<int>(std::ceil(std::log2(std::ceil(10.0 * dSFreq))));
    const double nyquist = dSFreq / 2.0;
    const RowVectorXd h = firwin2(nTaps,
                                  {0.0, (dLFreq - 0.5) / nyquist, dLFreq / nyquist, dHFreq / nyquist, (dHFreq + 0.5) / nyquist, 1.0},
                                  {0.0, 0.0, 1.0, 1.0, 0.0, 0.0},
                                  window);

    // Forward and backward pass = one pass with h convolved with its reverse; padding follows the single h.
    const Index nH = h.size();
    FFT<double> fft;
    RowVectorXd padded = RowVectorXd::Zero(2 * nH);
    padded.head(nH) = h;
    RowVectorXcd spec;
    fft.fwd(spec, padded);
    spec = spec.cwiseAbs2().cast<std::complex<double>>();
    RowVectorXd autocorr;
    fft.inv(autocorr, spec);
    // Lags -(nH - 1) .. nH - 1
    RowVectorXd hh(2 * nH - 1);
    hh << autocorr.tail(nH - 1), autocorr.head(nH);
    return overlapAddZeroPhase(matData, hh, std::min<Index>(nH, matData.cols()) - 1);
}

//=============================================================================================================

MatrixXd FirFilter::overlapAddZeroPhase(const MatrixXd& matData, const RowVectorXd& h, Index nEdge)
{
    // Adapted from mne.filter._overlap_add_filter / mne.cuda._smart_pad (MNE-Python, BSD-3-Clause).
    const Index nTimes = matData.cols();
    const Index nH = h.size();
    if (nTimes == 0 || nH == 1) {
        return matData * (nH == 1 ? h(0) : 1.0);
    }
    const Index nExt = nTimes + 2 * nEdge;
    // A power of two keeps kissfft fast; any length >= nExt + nH - 1 gives the same linear convolution
    Index nFft = 1;
    while (nFft < nExt + nH - 1)
        nFft <<= 1;

    FFT<double> fft;
    RowVectorXd hPadded = RowVectorXd::Zero(nFft);
    hPadded.head(nH) = h;
    RowVectorXcd hSpec;
    fft.fwd(hSpec, hPadded);

    MatrixXd out(matData.rows(), nTimes);
    RowVectorXd ext = RowVectorXd::Zero(nFft);
    RowVectorXcd spec;
    RowVectorXd conv;
    for (Index r = 0; r < matData.rows(); ++r) {
        const RowVectorXd x = matData.row(r);
        // "reflect_limited" = odd reflection about the end samples (nEdge < nTimes, so no zero part)
        ext.setZero();
        for (Index i = 0; i < nEdge; ++i) {
            ext(i) = 2.0 * x(0) - x(nEdge - i);
            ext(nEdge + nTimes + i) = 2.0 * x(nTimes - 1) - x(nTimes - 2 - i);
        }
        ext.segment(nEdge, nTimes) = x;
        fft.fwd(spec, ext);
        spec = spec.cwiseProduct(hSpec);
        fft.inv(conv, spec);
        out.row(r) = conv.segment(nEdge + (nH - 1) / 2, nTimes);
    }
    return out;
}
