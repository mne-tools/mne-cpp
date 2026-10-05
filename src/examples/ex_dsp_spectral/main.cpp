//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Spectral and filtering tools of the dsp library on signals with known content.
 *
 * The test signal is a 20 Hz sine plus a weaker 55 Hz sine. Expected values
 * come from SciPy (DPSS concentration ratios, Butterworth sections and gain,
 * peak finding), MNE-Python 1.11 (multitaper PSD) or closed forms. The
 * example exits non-zero if any value disagrees.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/csd.h>
#include <dsp/dpss.h>
#include <dsp/iirfilter.h>
#include <dsp/morlet_tfr.h>
#include <dsp/multitaper_psd.h>
#include <dsp/multitaper_tfr.h>
#include <dsp/peak_finder.h>
#include <dsp/resample.h>
#include <dsp/spectrogram.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <complex>
#include <cstdlib>
#include <initializer_list>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr double kSFreq = 200.0;

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

//=============================================================================================================

double maxError(const VectorXd& actual, std::initializer_list<double> expected)
{
    double err = actual.size() == static_cast<Index>(expected.size()) ? 0.0 : 1e300;
    int k = 0;
    for (double value : expected) {
        if (k < actual.size()) {
            err = std::max(err, std::fabs(actual(k) - value));
        }
        ++k;
    }
    return err;
}

//=============================================================================================================

double frequencyOfMax(const RowVectorXd& values, const RowVectorXd& freqs)
{
    Index peak = 0;
    values.maxCoeff(&peak);
    return freqs(peak);
}

//=============================================================================================================

double gainAt(const QVector<IirBiquad>& sos, double freq)
{
    const std::complex<double> z = std::polar(1.0, -2.0 * kPi * freq / kSFreq); // z^-1
    std::complex<double> h(1.0, 0.0);
    for (const IirBiquad& s : sos) {
        h *= (s.b0 + s.b1 * z + s.b2 * z * z) / (1.0 + s.a1 * z + s.a2 * z * z);
    }
    return std::abs(h);
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    const int n = 400;
    RowVectorXd signal(n);
    for (int t = 0; t < n; ++t) {
        signal(t) = std::sin(2.0 * kPi * 20.0 * t / kSFreq) + 0.5 * std::sin(2.0 * kPi * 55.0 * t / kSFreq);
    }

    //! [dpss_compute]
    const DpssResult dpss = Dpss::compute(64, 2.5, 4); // N = 64 samples, NW = 2.5, 4 tapers
    //! [dpss_compute]
    // scipy.signal.windows.dpss(64, 2.5, 4, return_ratios=True)
    const double dpssErr = maxError(dpss.vecEigenvalues, {0.9999972774351693, 0.9998465484588153, 0.9962668597877571, 0.9524259615013084});
    ok &= expect(dpss.matTapers.rows() == 4 && dpssErr < 1e-8, QString("DPSS concentration ratios = scipy (max error %1)").arg(dpssErr));
    ok &= expect((dpss.matTapers * dpss.matTapers.transpose() - MatrixXd::Identity(4, 4)).cwiseAbs().maxCoeff() < 1e-10,
                 "DPSS tapers are orthonormal");

    //! [multitaper_psd_compute]
    const MultitaperPsdResult psd = MultitaperPsd::compute(signal, kSFreq, 2.0); // NW = 2, i.e. +-1 Hz resolution
    const double binWidth = psd.vecFreqs(1) - psd.vecFreqs(0);
    const double power = psd.matPsd.row(0).sum() * binWidth; // one-sided PSD integrates to the variance
    //! [multitaper_psd_compute]
    // mne.time_frequency.psd_array_multitaper(bandwidth=4 Hz, adaptive=False): 20 Hz 0.31659, 55 Hz 0.079135
    const double psd20 = psd.matPsd(0, 40);
    const double psd55 = psd.matPsd(0, 110);
    ok &= expect(frequencyOfMax(psd.matPsd.row(0), psd.vecFreqs) == 20.0 && std::fabs(psd20 - 0.316593412495819) < 1e-3 && std::fabs(psd55 - 0.07913494397010905) < 1e-3 && std::fabs(power - 0.625) < 0.01,
                 QString("multitaper PSD at 20/55 Hz %1/%2 = mne, integral %3").arg(psd20).arg(psd55).arg(power));

    //! [multitaper_tfr_compute]
    const MultitaperTfrResult tfr = MultitaperTfr::compute(signal, kSFreq, 128, 32); // 128-sample windows, 32-sample hop
    //! [multitaper_tfr_compute]
    const RowVectorXd tfrMean = tfr.tfrData[0].rowwise().mean().transpose();
    ok &= expect(std::fabs(frequencyOfMax(tfrMean, tfr.vecFreqs) - 20.0) < 2.0 && tfr.vecTimes.size() == tfr.tfrData[0].cols(),
                 QString("multitaper TFR: %1 windows, strongest band near 20 Hz").arg(tfr.vecTimes.size()));

    //! [morlet_tfr_compute]
    RowVectorXd morletFreqs(3);
    morletFreqs << 20.0, 35.0, 55.0;
    const MorletTfrResult morlet = MorletTfr::compute(signal, kSFreq, morletFreqs, 7.0);
    //! [morlet_tfr_compute]
    const VectorXd middle = morlet.matPower.col(n / 2);
    // Power of a sine of amplitude A is ~ A^2 times a common wavelet gain: 20 Hz carries 4x the 55 Hz power
    ok &= expect(middle(0) > 3.0 * middle(2) && middle(2) > 10.0 * middle(1),
                 QString("Morlet power at 20/35/55 Hz: %1 / %2 / %3").arg(middle(0)).arg(middle(1)).arg(middle(2)));

    //! [spectrogram_make]
    const MatrixXd spectrogram = Spectrogram::makeSpectrogram(signal.transpose(), 64); // frequency bins x samples
    //! [spectrogram_make]
    ok &= expect(spectrogram.cols() == n && spectrogram.rows() > 0 && spectrogram.allFinite(),
                 QString("spectrogram is %1 x %2 and finite").arg(spectrogram.rows()).arg(spectrogram.cols()));

    //! [csd_compute]
    MatrixXd twoChannels(2, n);
    twoChannels.row(0) = signal;
    for (int t = 0; t < n; ++t) {
        twoChannels(1, t) = std::cos(2.0 * kPi * 20.0 * t / kSFreq); // 20 Hz, 90 degrees ahead of channel 0
    }
    const CsdResult csd = Csd::computeMultitaper(twoChannels, kSFreq, 18.0, 22.0, 2.0);
    const std::complex<double> cross = csd.matCsd(0, 1);
    //! [csd_compute]
    ok &= expect(csd.matCsd.rows() == 2 && std::fabs(std::fabs(std::arg(cross)) - kPi / 2.0) < 0.1,
                 QString("CSD phase between sine and cosine at 20 Hz: %1 rad (pi/2)").arg(std::arg(cross)));

    //! [resample_usage]
    const RowVectorXd downsampled = Resample::resample(signal, 100.0, kSFreq); // 200 Hz -> 100 Hz
    //! [resample_usage]
    // 55 Hz is above the new Nyquist (50 Hz) and must be removed; the 20 Hz sine stays
    RowVectorXd expected20(downsampled.size());
    for (int t = 0; t < downsampled.size(); ++t) {
        expected20(t) = std::sin(2.0 * kPi * 20.0 * t / 100.0);
    }
    const double resampleErr = (downsampled - expected20).segment(20, downsampled.size() - 40).cwiseAbs().maxCoeff();
    ok &= expect(downsampled.size() == n / 2 && resampleErr < 0.05,
                 QString("resampled to 100 Hz: 20 Hz kept, 55 Hz removed (max error %1)").arg(resampleErr));

    //! [iir_filter_usage]
    const QVector<IirBiquad> sos = IirFilter::designButterworth(4, IirFilter::LowPass, 30.0, 0.0, kSFreq);
    const RowVectorXd lowPassed = IirFilter::applyZeroPhase(signal, sos); // forward-backward, no phase shift
    //! [iir_filter_usage]
    // scipy.signal.butter(4, 30, 'low', fs=200, output='sos') and sosfreqz: |H| = 0.99996, 0.70711, 0.01878
    ok &= expect(sos.size() == 2 && std::fabs(gainAt(sos, 10.0) - 0.9999564172449813) < 1e-9 && std::fabs(gainAt(sos, 30.0) - 0.7071067811865477) < 1e-9 && std::fabs(gainAt(sos, 60.0) - 0.018777212012474308) < 1e-9,
                 "Butterworth gain at 10/30/60 Hz = scipy");
    RowVectorXd only20(n);
    for (int t = 0; t < n; ++t) {
        only20(t) = std::sin(2.0 * kPi * 20.0 * t / kSFreq);
    }
    // scipy.signal.sosfiltfilt deviates from the 20 Hz sine by 0.02593 in the interior (|H(20 Hz)|^2 < 1)
    const double lowPassErr = (lowPassed - only20).segment(50, n - 100).cwiseAbs().maxCoeff();
    ok &= expect(std::fabs(lowPassErr - 0.02592545936305224) < 1e-3,
                 QString("zero-phase low-pass: 55 Hz removed, 20 Hz in phase (deviation %1, scipy 0.02593)").arg(lowPassErr));

    VectorXd trace(10);
    trace << 0, 1, 0, 3, 0, 2, 0, 5, 1, 0;
    const QList<QPair<int, double>> allPeaks = peakFinder(trace);
    PeakFinderParams params;
    params.dProminence = 2.5;
    const QList<QPair<int, double>> strongPeaks = peakFinder(trace, params);
    const VectorXd prominences = peakProminences(trace, {1, 3, 5, 7});
    // scipy.signal.find_peaks / peak_prominences
    ok &= expect(allPeaks.size() == 4 && allPeaks[3].first == 7 && strongPeaks.size() == 2 && strongPeaks[0].first == 3 && maxError(prominences, {1.0, 3.0, 2.0, 5.0}) == 0.0,
                 "peaks [1, 3, 5, 7], prominences [1, 3, 2, 5], prominence >= 2.5 keeps [3, 7] (scipy)");

    qInfo() << (ok ? "All dsp spectral checks passed." : "dsp spectral checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
