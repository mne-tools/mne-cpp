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

#include <dsp/spectrogram.h>

#include <Eigen/Core>

#include <cmath>

using namespace UTILSLIB;
using namespace Eigen;

namespace {

VectorXd makeSignal(int nSamples, int periodSamples)
{
    VectorXd signal(nSamples);
    for (int i = 0; i < nSamples; ++i) {
        signal(i) = std::sin(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(periodSamples));
    }
    return signal;
}

}

class TestDspSpectrogram : public QObject
{
    Q_OBJECT

private slots:
    void makeSpectrogram_dimensions();
    void makeSpectrogram_defaultWindow_runs();
    void makeSpectrogram_constantSignal_isNearZero();
    void makeSpectrogram_pureTone_hasDominantFrequencyRow();
};

void TestDspSpectrogram::makeSpectrogram_dimensions()
{
    const int nSamples = 96;
    MatrixXd spec = Spectrogram::makeSpectrogram(makeSignal(nSamples, 12), 16);

    QCOMPARE(spec.rows(), nSamples / 2);
    QCOMPARE(spec.cols(), nSamples);
    QVERIFY(spec.allFinite());
}

void TestDspSpectrogram::makeSpectrogram_defaultWindow_runs()
{
    const int nSamples = 75;
    MatrixXd spec = Spectrogram::makeSpectrogram(makeSignal(nSamples, 10), 0);

    QCOMPARE(spec.rows(), nSamples / 2);
    QCOMPARE(spec.cols(), nSamples);
    QVERIFY(spec.allFinite());
    QVERIFY((spec.array() >= 0.0).all());
}

void TestDspSpectrogram::makeSpectrogram_constantSignal_isNearZero()
{
    const int nSamples = 80;
    VectorXd constant = VectorXd::Constant(nSamples, 3.0);

    MatrixXd spec = Spectrogram::makeSpectrogram(constant, 12);
    QVERIFY(spec.norm() < 1e-8);
}

void TestDspSpectrogram::makeSpectrogram_pureTone_hasDominantFrequencyRow()
{
    const int nSamples = 128;
    const int periodSamples = 16;
    MatrixXd spec = Spectrogram::makeSpectrogram(makeSignal(nSamples, periodSamples), 14);

    VectorXd meanSpectrum = spec.rowwise().mean();
    Eigen::Index peakIndex = 0;
    meanSpectrum.maxCoeff(&peakIndex);

    QCOMPARE(static_cast<int>(peakIndex), nSamples / periodSamples);
}

QTEST_MAIN(TestDspSpectrogram)
#include "test_dsp_spectrogram.moc"
