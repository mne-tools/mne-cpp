//=============================================================================================================
/**
 * @file     test_dsp_morlet_tfr.cpp
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
 * @brief    Unit tests for MorletTfr.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/morlet_tfr.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestMorletTfr : public QObject
{
    Q_OBJECT

private slots:
    // Output shape
    void testOutputRows();
    void testOutputCols();
    void testFreqsStored();

    // Power properties
    void testPowerNonNegative();
    void testPowerFinite();
    void testSinePowerPeakFrequency();
    void testSinePowerTemporalMean();

    // Number of cycles
    void testMoreCyclesBetterFrequencyResolution();
    void testFewerCyclesBetterTimeResolution();

    // Multi-channel
    void testMultiChannelCount();
    void testMultiChannelPicks();
    void testMultiChannelMatchesSingle();
};

//=============================================================================================================

static RowVectorXd makeSine(double dFreq, double dSFreq, int nSamples, double dAmpl = 1.0)
{
    RowVectorXd v(nSamples);
    for (int n = 0; n < nSamples; ++n)
        v[n] = dAmpl * std::sin(2.0 * M_PI * dFreq * n / dSFreq);
    return v;
}

//=============================================================================================================

void TestMorletTfr::testOutputRows()
{
    RowVectorXd freqs = RowVectorXd::LinSpaced(8, 4.0, 40.0);
    auto r = MorletTfr::compute(makeSine(10.0, 250.0, 1000), 250.0, freqs);
    QCOMPARE(r.matPower.rows(), 8);
}

//=============================================================================================================

void TestMorletTfr::testOutputCols()
{
    const int nSamp = 1234;
    RowVectorXd freqs = RowVectorXd::LinSpaced(4, 5.0, 20.0);
    auto r = MorletTfr::compute(makeSine(10.0, 200.0, nSamp), 200.0, freqs);
    QCOMPARE(r.matPower.cols(), nSamp);
}

//=============================================================================================================

void TestMorletTfr::testFreqsStored()
{
    RowVectorXd freqs = RowVectorXd::LinSpaced(10, 1.0, 50.0);
    auto r = MorletTfr::compute(makeSine(25.0, 500.0, 2000), 500.0, freqs);
    QCOMPARE(r.vecFreqs.cols(), freqs.cols());
    QVERIFY((r.vecFreqs - freqs).norm() < 1e-10);
}

//=============================================================================================================

void TestMorletTfr::testPowerNonNegative()
{
    RowVectorXd freqs = RowVectorXd::LinSpaced(6, 5.0, 30.0);
    auto r = MorletTfr::compute(makeSine(15.0, 300.0, 3000), 300.0, freqs);
    QVERIFY((r.matPower.array() >= 0.0).all());
}

//=============================================================================================================

void TestMorletTfr::testPowerFinite()
{
    RowVectorXd freqs = RowVectorXd::LinSpaced(6, 5.0, 30.0);
    auto r = MorletTfr::compute(RowVectorXd::Random(2000), 500.0, freqs);
    QVERIFY(r.matPower.allFinite());
}

//=============================================================================================================

void TestMorletTfr::testSinePowerPeakFrequency()
{
    // A sine at 20 Hz should produce maximum mean power at the 20 Hz row
    const double dSFreq  = 500.0;
    const double dFreq   = 20.0;
    const int    nSamp   = 5000;
    RowVectorXd  sig     = makeSine(dFreq, dSFreq, nSamp);

    // Frequencies 5, 10, 15, 20, 25, 30, 35, 40 Hz
    RowVectorXd freqs = RowVectorXd::LinSpaced(8, 5.0, 40.0);
    auto r = MorletTfr::compute(sig, dSFreq, freqs);

    // Mean power per frequency
    RowVectorXd meanPow = r.matPower.rowwise().mean().transpose();
    int peakRow = 0;
    meanPow.maxCoeff(&peakRow);

    // Allow ±1 frequency bin tolerance
    QVERIFY(std::abs(r.vecFreqs[peakRow] - dFreq) <= (freqs[1] - freqs[0]) + 1e-6);
}

//=============================================================================================================

void TestMorletTfr::testSinePowerTemporalMean()
{
    // For a stationary sine the mean power should not vary much over time
    const double dSFreq = 500.0;
    const double dFreq  = 30.0;
    const int    nSamp  = 5000;
    RowVectorXd  sig    = makeSine(dFreq, dSFreq, nSamp);

    RowVectorXd freqs(1);
    freqs << dFreq;
    auto r = MorletTfr::compute(sig, dSFreq, freqs);

    // Ignore first/last 100 samples (edge artefacts)
    RowVectorXd mid = r.matPower.row(0).segment(100, nSamp - 200);
    auto amp = mid.array().sqrt();
    double ampMean = amp.mean();
    double ampStd  = std::sqrt((amp - ampMean).square().mean());
    double cv = ampMean == 0.0 ? 0.0 : ampStd / ampMean;
    QVERIFY(cv < 0.10);  // coefficient of variation < 10 %
}

//=============================================================================================================

void TestMorletTfr::testMoreCyclesBetterFrequencyResolution()
{
    // Higher nCycles → sharper frequency peak
    const double dSFreq  = 500.0;
    const int    nSamp   = 8000;
    // Two close frequencies
    RowVectorXd sig(nSamp);
    for (int n = 0; n < nSamp; ++n)
        sig[n] = std::sin(2.0 * M_PI * 20.0 * n / dSFreq)
               + std::sin(2.0 * M_PI * 25.0 * n / dSFreq);

    RowVectorXd freqs = RowVectorXd::LinSpaced(20, 10.0, 40.0);

    auto rFew  = MorletTfr::compute(sig, dSFreq, freqs, 3.0);
    auto rMany = MorletTfr::compute(sig, dSFreq, freqs, 14.0);

    // Sharper peak → lower variance across mean-power bins (relative to total)
    RowVectorXd powFew  = rFew.matPower.rowwise().mean().transpose();
    RowVectorXd powMany = rMany.matPower.rowwise().mean().transpose();

    double varFew  = (powFew.array()  - powFew.mean()).square().mean();
    double varMany = (powMany.array() - powMany.mean()).square().mean();

    QVERIFY(varMany > varFew);  // more cycles → more pronounced frequency peaks
}

//=============================================================================================================

void TestMorletTfr::testFewerCyclesBetterTimeResolution()
{
    // Fewer nCycles → shorter Gaussian envelope → power rises faster at burst onset.
    // Test: at a fixed time shortly after burst onset, the 3-cycle power (relative to
    // its own max) should be closer to maximum than the 14-cycle power.
    const double dSFreq   = 500.0;
    const int    nSamp    = 4000;
    const int    onsetSmp = 1500;

    // 20 Hz burst from onsetSmp to end (long enough that both wavelets reach full power)
    RowVectorXd sig = RowVectorXd::Zero(nSamp);
    for (int n = onsetSmp; n < nSamp - 200; ++n)
        sig[n] = std::sin(2.0 * M_PI * 20.0 * n / dSFreq);

    RowVectorXd freqs(1);
    freqs << 20.0;

    // 3-cycle wavelet: sigma_t ≈ 24 ms → halfLen ≈ 48 samples at 500 Hz
    // 14-cycle wavelet: sigma_t ≈ 111 ms → halfLen ≈ 223 samples at 500 Hz
    auto rFew  = MorletTfr::compute(sig, dSFreq, freqs, 3.0);
    auto rMany = MorletTfr::compute(sig, dSFreq, freqs, 14.0);

    const RowVectorXd& powFew  = rFew.matPower.row(0);
    const RowVectorXd& powMany = rMany.matPower.row(0);

    // Probe 80 samples after onset (well within the 3-cycle rise but mid-rise for 14-cycle)
    const int probeIdx = onsetSmp + 80;
    double ratioFew  = powFew[probeIdx]  / powFew.maxCoeff();
    double ratioMany = powMany[probeIdx] / powMany.maxCoeff();

    // With fewer cycles the power reaches its peak faster → higher ratio at the probe point
    QVERIFY(ratioFew > ratioMany);
}

//=============================================================================================================

void TestMorletTfr::testMultiChannelCount()
{
    const int    nCh    = 6;
    const int    nSamp  = 2000;
    MatrixXd     mat    = MatrixXd::Random(nCh, nSamp);
    RowVectorXd  freqs  = RowVectorXd::LinSpaced(5, 5.0, 25.0);

    auto results = MorletTfr::computeMultiChannel(mat, 300.0, freqs);
    QCOMPARE(results.size(), nCh);
}

//=============================================================================================================

void TestMorletTfr::testMultiChannelPicks()
{
    const int   nCh   = 8;
    MatrixXd    mat   = MatrixXd::Random(nCh, 2000);
    RowVectorXd freqs = RowVectorXd::LinSpaced(4, 4.0, 20.0);

    RowVectorXi picks(3);
    picks << 1, 3, 7;
    auto results = MorletTfr::computeMultiChannel(mat, 300.0, freqs, 7.0, picks);
    QCOMPARE(results.size(), 3);
}

//=============================================================================================================

void TestMorletTfr::testMultiChannelMatchesSingle()
{
    const int   nCh   = 4;
    const int   nSamp = 1500;
    MatrixXd    mat   = MatrixXd::Random(nCh, nSamp);
    RowVectorXd freqs = RowVectorXd::LinSpaced(5, 5.0, 25.0);
    const double sfreq = 300.0;

    auto multi = MorletTfr::computeMultiChannel(mat, sfreq, freqs);

    for (int ch = 0; ch < nCh; ++ch) {
        auto single = MorletTfr::compute(mat.row(ch), sfreq, freqs);
        QVERIFY((multi[ch].matPower - single.matPower).norm() < 1e-8);
    }
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestMorletTfr)
#include "test_dsp_morlet_tfr.moc"
