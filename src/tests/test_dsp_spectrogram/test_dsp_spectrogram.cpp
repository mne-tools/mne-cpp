//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_spectrogram.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Unit tests for Spectrogram.
 */

#include <QtTest/QtTest>

#include <mne/dsp/spectrogram.h>

#include <Eigen/Core>

#include <cmath>
#include <complex>
#include <numbers>

using namespace mne;
using namespace Eigen;

namespace
{

VectorXd makeSignal(int nSamples, int periodSamples)
{
    VectorXd signal(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        signal(i) = std::sin(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(periodSamples));
    }
    return signal;
}

}

class TestDspSpectrogram : public QObject
{
    Q_OBJECT

private slots:
    void make_spectrogram_dimensions();
    void make_spectrogram_defaultWindow_runs();
    void make_spectrogram_constantSignal_isNearZero();
    void make_spectrogram_pureTone_hasDominantFrequencyRow();
    void make_spectrogram_everyColumnIsTheWindowedSpectrum();
};

void TestDspSpectrogram::make_spectrogram_dimensions()
{
    const int nSamples = 96;
    MatrixXd spec = Spectrogram::make_spectrogram(makeSignal(nSamples, 12), 16);

    QCOMPARE(spec.rows(), nSamples / 2);
    QCOMPARE(spec.cols(), nSamples);
    QVERIFY(spec.allFinite());
}

void TestDspSpectrogram::make_spectrogram_defaultWindow_runs()
{
    const int nSamples = 75;
    MatrixXd spec = Spectrogram::make_spectrogram(makeSignal(nSamples, 10), 0);

    QCOMPARE(spec.rows(), nSamples / 2);
    QCOMPARE(spec.cols(), nSamples);
    QVERIFY(spec.allFinite());
    QVERIFY((spec.array() >= 0.0).all());
}

void TestDspSpectrogram::make_spectrogram_constantSignal_isNearZero()
{
    const int nSamples = 80;
    VectorXd constant = VectorXd::Constant(nSamples, 3.0);

    MatrixXd spec = Spectrogram::make_spectrogram(constant, 12);
    QVERIFY(spec.norm() < 1e-8);
}

void TestDspSpectrogram::make_spectrogram_pureTone_hasDominantFrequencyRow()
{
    const int nSamples = 128;
    const int periodSamples = 16;
    MatrixXd spec = Spectrogram::make_spectrogram(makeSignal(nSamples, periodSamples), 14);

    VectorXd meanSpectrum = spec.rowwise().mean();
    Eigen::Index peakIndex = 0;
    meanSpectrum.maxCoeff(&peakIndex);

    QCOMPARE(static_cast<int>(peakIndex), nSamples / periodSamples);
}

void TestDspSpectrogram::make_spectrogram_everyColumnIsTheWindowedSpectrum()
{
    // Column j is |DFT(x * g_j)|^2 of the demeaned signal, g_j the Gaussian centred on sample j
    const int n = 97;
    const double scale = 9.0;
    VectorXd signal = makeSignal(n, 7) + 0.3 * makeSignal(n, 23) + VectorXd::Constant(n, 2.0);
    const MatrixXd spec = Spectrogram::make_spectrogram(signal, 9);
    signal.array() -= signal.mean();
    QCOMPARE(spec.rows(), n / 2);
    for (int j = 0; j < n; ++j) {
        for (int k = 0; k < n / 2; ++k) {
            std::complex<double> bin = 0.0;
            for (int t = 0; t < n; ++t) {
                const double u = (t - j) / scale;
                const double window = std::exp(-std::numbers::pi * u * u) * std::pow(2.0, 0.25) / std::sqrt(scale);
                bin += signal(t) * window * std::polar(1.0, -2.0 * std::numbers::pi * k * t / n);
            }
            QVERIFY2(std::abs(spec(k, j) - std::norm(bin)) <= 1e-9 * (1.0 + std::norm(bin)), qPrintable(QString("bin %1, column %2").arg(k).arg(j)));
        }
    }
}

QTEST_MAIN(TestDspSpectrogram)
#include "test_dsp_spectrogram.moc"
