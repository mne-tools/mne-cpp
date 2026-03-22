//=============================================================================================================
/**
 * @file     test_dsp_spectrogram.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
