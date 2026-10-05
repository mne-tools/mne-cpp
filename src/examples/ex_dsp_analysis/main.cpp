//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    DSP library: Welch PSD, FIR filtering, peak finding, envelope correlation, SPHARA and simulation.
 *
 * Every check runs on a synthetic signal and compares with SciPy 1.15
 * (signal.welch, signal.find_peaks, signal.peak_prominences, signal.hilbert)
 * or a closed form. Exits non-zero on any mismatch.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/connectivity_aec.h>
#include <dsp/filterkernel.h>
#include <dsp/firfilter.h>
#include <dsp/peak_finder.h>
#include <dsp/simulate.h>
#include <dsp/sphara.h>
#include <dsp/welch_psd.h>

#include <inv/inv_source_estimate.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <cstdlib>

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

bool expect(bool condition, const QString& what)
{
    qInfo().noquote() << (condition ? "  ok  " : "  FAIL") << what;
    return condition;
}

QList<int> indices(const QList<QPair<int, double>>& peaks)
{
    QList<int> out;
    for (const auto& peak : peaks) {
        out.append(peak.first);
    }
    return out;
}

/** Spectral amplitude of @p signal at @p freqHz, by projection onto sin/cos. */
double toneAmplitude(const RowVectorXd& signal, double freqHz, double sFreq)
{
    double re = 0.0;
    double im = 0.0;
    for (Index n = 0; n < signal.size(); ++n) {
        re += signal(n) * std::cos(2.0 * kPi * freqHz * static_cast<double>(n) / sFreq);
        im += signal(n) * std::sin(2.0 * kPi * freqHz * static_cast<double>(n) / sFreq);
    }
    return 2.0 * std::hypot(re, im) / static_cast<double>(signal.size());
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    bool ok = true;

    // sin(10 Hz) + 0.5 cos(40 Hz) at 250 Hz for 4 s
    const double sFreq = 250.0;
    RowVectorXd tones(1000);
    for (int n = 0; n < tones.size(); ++n) {
        tones(n) = std::sin(2.0 * kPi * 10.0 * n / sFreq) + 0.5 * std::cos(2.0 * kPi * 40.0 * n / sFreq);
    }

    //! [welch_psd_compute]
    const WelchPsdResult welch = WelchPsd::compute(tones, sFreq, 250, 0.5, WelchPsd::Hann); // 1 Hz bins, 50 % overlap
    // welch.matPsd: n_channels x 126 one-sided PSD (unit^2/Hz); welch.vecFreqs: 0 ... 125 Hz
    //! [welch_psd_compute]
    // scipy.signal.welch(x, 250, window=get_window("hann", 250, fftbins=False), nperseg=250, noverlap=125, detrend=False)
    ok &= expect(welch.vecFreqs.size() == 126 && std::fabs(welch.matPsd(0, 10) - 0.331993789047378) < 1e-9 && std::fabs(welch.matPsd(0, 40) - 0.08300028717365324) < 1e-9 && std::fabs(welch.matPsd.sum() - 0.6250000465680238) < 1e-9,
                 QString("WelchPsd: P(10 Hz) = %1, P(40 Hz) = %2 match scipy").arg(welch.matPsd(0, 10)).arg(welch.matPsd(0, 40)));

    //! [fir_filter_usage]
    FilterKernel lowPass = FirFilter::design(256, FirFilter::LowPass, 20.0, 0.0, sFreq, 5.0); // order, type, cutoff(s), Hz, transition
    const RowVectorXd smooth = FirFilter::applyZeroPhase(tones, lowPass);
    //! [fir_filter_usage]
    const double kept = toneAmplitude(smooth.segment(250, 500), 10.0, sFreq);
    const double removed = toneAmplitude(smooth.segment(250, 500), 40.0, sFreq);
    ok &= expect(smooth.size() == tones.size() && std::fabs(kept - 1.0) < 0.02 && removed < 0.01,
                 QString("FirFilter 20 Hz low-pass keeps 10 Hz (%1) and removes 40 Hz (%2)").arg(kept, 0, 'f', 4).arg(removed, 0, 'f', 4));

    // sin(t/40) + 0.3 sin(t/7) + slow drift
    VectorXd wave(200);
    for (int t = 0; t < wave.size(); ++t) {
        wave(t) = std::sin(2.0 * kPi * t / 40.0) + 0.3 * std::sin(2.0 * kPi * t / 7.0) + 0.002 * t;
    }

    //! [peak_finder_usage]
    PeakFinderParams params;
    params.dMinHeight = 0.5; // like scipy.signal.find_peaks(height=0.5, distance=30)
    params.iMinDistance = 30;
    const QList<QPair<int, double>> peaks = peakFinder(wave, params); // (sample, value), sorted by sample
    const VectorXd prominence = peakProminences(wave, {2, 9, 15});
    //! [peak_finder_usage]
    PeakFinderParams byProminence;
    byProminence.dProminence = 0.5;
    PeakFinderParams byDistance;
    byDistance.iMinDistance = 10;
    ok &= expect(indices(peaks) == QList<int>({9, 51, 92, 128, 170}) && indices(peakFinder(wave, byProminence)) == QList<int>({9, 51, 92, 128, 170}) && indices(peakFinder(wave, byDistance)) == QList<int>({9, 22, 37, 51, 64, 79, 92, 114, 128, 141, 156, 170, 183, 198}) && (prominence - Vector3d(0.18086696, 1.298166714, 0.289199146)).cwiseAbs().maxCoeff() < 1e-8,
                 "peakFinder (height, distance, prominence) and peakProminences match scipy");

    // a and b share a 1/128 envelope; c has its own 1/50 envelope
    MatrixXd envelopeSignals(3, 256);
    for (int m = 0; m < 256; ++m) {
        envelopeSignals(0, m) = std::sin(2.0 * kPi * m / 16.0) * (1.0 + 0.8 * std::sin(2.0 * kPi * m / 128.0));
        envelopeSignals(1, m) = std::cos(2.0 * kPi * m / 16.0 + 0.3) * (1.0 + 0.8 * std::sin(2.0 * kPi * m / 128.0 + 0.2));
        envelopeSignals(2, m) = std::sin(2.0 * kPi * m / 12.0) * (1.0 + 0.8 * std::cos(2.0 * kPi * m / 50.0));
    }

    //! [connectivity_aec_compute]
    const MatrixXd aec = ConnectivityAec::compute(envelopeSignals);                   // Pearson r of Hilbert envelopes
    const MatrixXd aecOrth = ConnectivityAec::computeOrthogonalized(envelopeSignals); // leakage-corrected
    //! [connectivity_aec_compute]
    // numpy corrcoef(|scipy.signal.hilbert(x)|): r(a,b) = 0.98006658, r(a,c) = -0.00701172; orthogonalized r(a,c) = 0.09799620
    ok &= expect(std::fabs(aec(0, 1) - 0.9800665778412415) < 1e-9 && std::fabs(aec(0, 2) + 0.007011719607347298) < 1e-9 && std::fabs(aecOrth(0, 2) - 0.09799619541278072) < 1e-9 && aecOrth(0, 1) > 0.98,
                 QString("ConnectivityAec: r(a,b) = %1, r(a,c) = %2 match scipy").arg(aec(0, 1), 0, 'f', 6).arg(aec(0, 2), 0, 'f', 6));

    // Orthonormal basis of a 6-channel layout: constant pattern plus a gradient
    MatrixXd basis = MatrixXd::Zero(3, 3);
    basis.col(0).setConstant(1.0 / std::sqrt(3.0));
    basis.col(1) << -1.0 / std::sqrt(2.0), 0.0, 1.0 / std::sqrt(2.0);
    basis.col(2) << 1.0 / std::sqrt(6.0), -2.0 / std::sqrt(6.0), 1.0 / std::sqrt(6.0);

    //! [sphara_projector_usage]
    const VectorXi channels = (VectorXi(3) << 1, 3, 5).finished();         // rows the basis belongs to
    const MatrixXd projector = makeSpharaProjector(basis, channels, 6, 1); // keep the first basis function
    //! [sphara_projector_usage]
    VectorXd sample(6);
    sample << 7.0, 1.0, 7.0, 2.0, 7.0, 6.0;
    const VectorXd filtered = projector * sample;
    ok &= expect((filtered - (VectorXd(6) << 7.0, 3.0, 7.0, 3.0, 7.0, 3.0).finished()).cwiseAbs().maxCoeff() < 1e-12,
                 "SPHARA projector replaces channels 1, 3, 5 by their mean and leaves the others");

    //! [simulate_stc_usage]
    const VectorXi vertices = VectorXi::LinSpaced(5, 100, 104);
    MatrixXd waveforms(2, 4);
    waveforms << 1.0, 2.0, 3.0, 4.0,
        -1.0, 0.0, 1.0, 0.0;
    const INVLIB::InvSourceEstimate stc = simulateStcFromWaveforms(waveforms, (VectorXi(2) << 101, 104).finished(), vertices, 0.0f, 0.01f);

    SimulateStcParams gaussians; // one Gaussian bump per active vertex, seeded
    gaussians.sfreq = 200.0f;
    gaussians.duration = 0.5f;
    const INVLIB::InvSourceEstimate random = simulateStc((VectorXi(1) << 102).finished(), vertices, gaussians);
    //! [simulate_stc_usage]
    ok &= expect(stc.data.rows() == 5 && stc.data.row(1) == waveforms.row(0) && stc.data.row(4) == waveforms.row(1) && stc.data.row(0).isZero() && std::fabs(stc.tstep - 0.01f) < 1e-7f && random.data.cols() == 100 && std::fabs(random.data.row(2).maxCoeff() - 1.0) < 0.01 && random.data.row(0).isZero(),
                 "simulateStcFromWaveforms places rows on their vertices; simulateStc draws a unit Gaussian bump");

    qInfo().noquote() << (ok ? "All dsp analysis checks passed." : "dsp analysis checks FAILED.");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
