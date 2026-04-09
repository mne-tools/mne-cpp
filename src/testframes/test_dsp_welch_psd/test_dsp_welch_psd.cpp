//=============================================================================================================
/**
 * @file     test_dsp_welch_psd.cpp
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
 * @brief    Unit tests for WelchPsd.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/welch_psd.h>

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

class TestWelchPsd : public QObject
{
    Q_OBJECT

private slots:
    // Output shape
    void testOutputDimensions();
    void testFreqAxisLength();
    void testFreqAxisValues();

    // Spectral content
    void testDCSine();
    void testPureSineFrequency();
    void testNoiselessSinePeak();
    void testTwoSinePeaks();

    // Normalisation
    void testParsevalsRelation();

    // Window types compile and run
    void testWindowTypes();

    // Multi-channel
    void testMultiChannel();
    void testChannelPicks();
};

//=============================================================================================================

static RowVectorXd makeSine(double dFreq, double dSFreq, int nSamples)
{
    RowVectorXd v(nSamples);
    for (int n = 0; n < nSamples; ++n)
        v[n] = std::sin(2.0 * M_PI * dFreq * n / dSFreq);
    return v;
}

//=============================================================================================================

void TestWelchPsd::testOutputDimensions()
{
    const int nfft = 128;
    MatrixXd mat(2, 1000);
    mat.row(0) = RowVectorXd::Random(1000);
    mat.row(1) = RowVectorXd::Random(1000);
    auto r = WelchPsd::compute(mat, 500.0, nfft);
    QCOMPARE(r.matPsd.rows(), 2);
    QCOMPARE(r.matPsd.cols(), nfft / 2 + 1);
}

//=============================================================================================================

void TestWelchPsd::testFreqAxisLength()
{
    const int nfft = 256;
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, 1000.0);
    QCOMPARE(freqs.cols(), nfft / 2 + 1);
}

//=============================================================================================================

void TestWelchPsd::testFreqAxisValues()
{
    const int    nfft  = 256;
    const double sfreq = 512.0;
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, sfreq);
    QVERIFY(std::abs(freqs[0])                    < 1e-10);   // DC = 0 Hz
    QVERIFY(std::abs(freqs[nfft/2] - sfreq / 2.0) < 1e-10);  // Nyquist
    QVERIFY(std::abs(freqs[1] - sfreq / nfft)      < 1e-10);  // bin width
}

//=============================================================================================================

void TestWelchPsd::testDCSine()
{
    // A constant offset → all power in DC bin (k=0)
    const int nSamples = 4096;
    RowVectorXd sig = RowVectorXd::Constant(nSamples, 1.0);
    RowVectorXd psd = WelchPsd::computeVector(sig, 500.0, 256, 0.5);

    // DC bin must be the maximum
    int peakBin = 0;
    psd.maxCoeff(&peakBin);
    QCOMPARE(peakBin, 0);
}

//=============================================================================================================

void TestWelchPsd::testPureSineFrequency()
{
    // A pure 50 Hz sine at 500 Hz sampling should peak near bin index 50·256/500 = ~25
    const double dSFreq = 500.0;
    const int    nfft   = 256;
    const double dFreq  = 50.0;

    RowVectorXd sig = makeSine(dFreq, dSFreq, 8192);
    RowVectorXd psd = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);

    int peakBin = 0;
    psd.maxCoeff(&peakBin);
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, dSFreq);

    // Allow ± 1 bin tolerance
    QVERIFY(std::abs(freqs[peakBin] - dFreq) <= dSFreq / nfft + 1e-6);
}

//=============================================================================================================

void TestWelchPsd::testNoiselessSinePeak()
{
    // Non-peak bins should be much smaller than the peak
    const double dSFreq = 1000.0;
    const int    nfft   = 512;
    RowVectorXd  sig    = makeSine(100.0, dSFreq, 16384);
    RowVectorXd  psd    = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);

    double peak = psd.maxCoeff();
    double mean = (psd.sum() - peak) / static_cast<double>(psd.cols() - 1);
    QVERIFY(peak / mean > 100.0);  // peak at least 100× average sidelobe
}

//=============================================================================================================

void TestWelchPsd::testTwoSinePeaks()
{
    const double dSFreq = 1000.0;
    const int    nfft   = 512;
    const int    nSamp  = 32768;

    RowVectorXd sig(nSamp);
    for (int n = 0; n < nSamp; ++n)
        sig[n] = std::sin(2.0 * M_PI * 50.0  * n / dSFreq)
               + std::sin(2.0 * M_PI * 200.0 * n / dSFreq);

    RowVectorXd psd   = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, dSFreq);

    // Find two largest peaks
    int peak1, peak2;
    psd.maxCoeff(&peak1);
    RowVectorXd psd2 = psd;
    psd2[peak1] = 0.0;
    psd2.maxCoeff(&peak2);

    QVERIFY(std::abs(freqs[peak1] - 50.0)  <= dSFreq / nfft + 1.0 ||
            std::abs(freqs[peak2] - 50.0)  <= dSFreq / nfft + 1.0);
    QVERIFY(std::abs(freqs[peak1] - 200.0) <= dSFreq / nfft + 1.0 ||
            std::abs(freqs[peak2] - 200.0) <= dSFreq / nfft + 1.0);
}

//=============================================================================================================

void TestWelchPsd::testParsevalsRelation()
{
    // For a Hann-windowed Welch PSD:
    // integral of one-sided PSD ≈ mean square of the signal (within ~10 %)
    const double dSFreq = 500.0;
    const int    nfft   = 256;
    RowVectorXd  sig    = makeSine(75.0, dSFreq, 16384);

    RowVectorXd psd   = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, dSFreq);
    const double df   = freqs[1];

    double integral = psd.sum() * df;
    double meanPow  = sig.squaredNorm() / static_cast<double>(sig.cols());

    // Pure sine: meanPow = 0.5; integral should be within ±10 %
    QVERIFY(std::abs(integral - meanPow) / meanPow < 0.10);
}

//=============================================================================================================

void TestWelchPsd::testWindowTypes()
{
    RowVectorXd sig = makeSine(60.0, 500.0, 4096);
    // All window types should run without throwing and return finite values
    for (int w = 0; w <= 3; ++w) {
        RowVectorXd psd = WelchPsd::computeVector(sig, 500.0, 256, 0.5,
                                                   static_cast<WelchPsd::WindowType>(w));
        QVERIFY(psd.allFinite());
        QVERIFY((psd.array() >= 0.0).all());
    }
}

//=============================================================================================================

void TestWelchPsd::testMultiChannel()
{
    const int nCh   = 5;
    const int nSamp = 4096;
    const int nfft  = 256;
    MatrixXd mat = MatrixXd::Random(nCh, nSamp);
    auto r = WelchPsd::compute(mat, 500.0, nfft);
    QCOMPARE(r.matPsd.rows(), nCh);
    QCOMPARE(r.matPsd.cols(), nfft / 2 + 1);
    QCOMPARE(r.vecFreqs.cols(), nfft / 2 + 1);
}

//=============================================================================================================

void TestWelchPsd::testChannelPicks()
{
    const int nCh   = 10;
    const int nSamp = 4096;
    const int nfft  = 256;
    MatrixXd mat = MatrixXd::Random(nCh, nSamp);

    RowVectorXi picks(3);
    picks << 0, 4, 9;
    auto r = WelchPsd::compute(mat, 500.0, nfft, 0.5, WelchPsd::Hann, picks);
    QCOMPARE(r.matPsd.rows(), 3);
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestWelchPsd)
#include "test_dsp_welch_psd.moc"
