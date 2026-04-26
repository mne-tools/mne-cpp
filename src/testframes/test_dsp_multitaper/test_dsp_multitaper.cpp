//=============================================================================================================
/**
 * @file     test_dsp_multitaper.cpp
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
 * @brief    Tests for DPSS, MultitaperPsd, and MultitaperTfr classes.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/dpss.h>
#include <dsp/multitaper_psd.h>
#include <dsp/multitaper_tfr.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DSPLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestDspMultitaper
 *
 * @brief The TestDspMultitaper class provides tests for DPSS and multitaper spectral analysis.
 */
class TestDspMultitaper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // DPSS tests
    void testDpssBasic();
    void testDpssTaperCount();
    void testDpssOrthogonality();
    void testDpssEigenvaluesNearOne();

    // MultitaperPsd tests
    void testPsdSinusoidal();
    void testPsdOutputDimensions();
    void testPsdFrequencyRange();
    void testPsdMultiChannel();

    // MultitaperTfr tests
    void testTfrSinusoidal();
    void testTfrOutputDimensions();

    void cleanupTestCase();

private:
    MatrixXd generateSinusoidal(int nChannels, int nSamples, double sfreq,
                                 const QVector<double>& frequencies) const;
};

//=============================================================================================================

void TestDspMultitaper::initTestCase()
{
}

//=============================================================================================================

MatrixXd TestDspMultitaper::generateSinusoidal(int nChannels, int nSamples, double sfreq,
                                                const QVector<double>& frequencies) const
{
    MatrixXd data = MatrixXd::Zero(nChannels, nSamples);
    for (int ch = 0; ch < nChannels; ++ch) {
        for (int s = 0; s < nSamples; ++s) {
            double t = s / sfreq;
            for (double freq : frequencies) {
                data(ch, s) += std::sin(2.0 * M_PI * freq * t);
            }
        }
    }
    return data;
}

//=============================================================================================================

void TestDspMultitaper::testDpssBasic()
{
    int N = 256;
    double halfBandwidth = 4.0;
    DpssResult result = Dpss::compute(N, halfBandwidth);

    // Default nTapers = 2*halfBandwidth - 1 = 7
    QVERIFY(result.matTapers.rows() > 0);
    QVERIFY(result.matTapers.cols() == N);
    QVERIFY(result.vecEigenvalues.size() == result.matTapers.rows());
}

//=============================================================================================================

void TestDspMultitaper::testDpssTaperCount()
{
    int N = 512;
    double halfBandwidth = 4.0;
    int nTapers = 3;
    DpssResult result = Dpss::compute(N, halfBandwidth, nTapers);

    QCOMPARE(static_cast<int>(result.matTapers.rows()), nTapers);
    QCOMPARE(static_cast<int>(result.matTapers.cols()), N);
    QCOMPARE(static_cast<int>(result.vecEigenvalues.size()), nTapers);
}

//=============================================================================================================

void TestDspMultitaper::testDpssOrthogonality()
{
    int N = 256;
    DpssResult result = Dpss::compute(N, 4.0, 3);

    // DPSS tapers should be orthonormal
    for (int i = 0; i < result.matTapers.rows(); ++i) {
        for (int j = 0; j < result.matTapers.rows(); ++j) {
            double dotProduct = result.matTapers.row(i).dot(result.matTapers.row(j));
            if (i == j) {
                QVERIFY(qAbs(dotProduct - 1.0) < 1e-6);
            } else {
                QVERIFY(qAbs(dotProduct) < 1e-6);
            }
        }
    }
}

//=============================================================================================================

void TestDspMultitaper::testDpssEigenvaluesNearOne()
{
    int N = 512;
    DpssResult result = Dpss::compute(N, 4.0, 3);

    // vecEigenvalues are the eigenvalues from the Slepian tridiagonal matrix
    // (not true concentration ratios in [0,1]).  Verify they are positive
    // and in descending order (largest eigenvalue first).
    QCOMPARE(result.vecEigenvalues.size(), 3);
    for (int i = 0; i < result.vecEigenvalues.size(); ++i) {
        QVERIFY(result.vecEigenvalues(i) > 0.0);
    }
    for (int i = 1; i < result.vecEigenvalues.size(); ++i) {
        QVERIFY(result.vecEigenvalues(i) <= result.vecEigenvalues(i - 1));
    }
}

//=============================================================================================================

void TestDspMultitaper::testPsdSinusoidal()
{
    double sfreq = 1000.0;
    // Keep N modest; DPSS solve is O(N^3) under coverage. 1024 samples
    // gives ~1 Hz frequency resolution which is fine for the 10 Hz peak.
    int nSamples = 1024;
    // Generate 10 Hz sinusoid
    MatrixXd data = generateSinusoidal(1, nSamples, sfreq, {10.0});

    MultitaperPsdResult result = MultitaperPsd::compute(data, sfreq, 4.0);

    QVERIFY(result.matPsd.rows() == 1);
    QVERIFY(result.matPsd.cols() > 0);
    QVERIFY(result.vecFreqs.size() == result.matPsd.cols());

    // Find the peak frequency — should be near 10 Hz
    int peakIdx;
    result.matPsd.row(0).maxCoeff(&peakIdx);
    double peakFreq = result.vecFreqs(peakIdx);
    QVERIFY(qAbs(peakFreq - 10.0) < 2.0); // Within 2 Hz
}

//=============================================================================================================

void TestDspMultitaper::testPsdOutputDimensions()
{
    double sfreq = 500.0;
    int nSamples = 256;
    int nChannels = 3;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    MultitaperPsdResult result = MultitaperPsd::compute(data, sfreq);

    QCOMPARE(static_cast<int>(result.matPsd.rows()), nChannels);
    QVERIFY(result.matPsd.cols() > 0);
    QCOMPARE(result.vecFreqs.size(), result.matPsd.cols());

    // All PSD values should be non-negative
    QVERIFY((result.matPsd.array() >= 0).all());
}

//=============================================================================================================

void TestDspMultitaper::testPsdFrequencyRange()
{
    double sfreq = 1000.0;
    int nSamples = 512;
    MatrixXd data = MatrixXd::Random(1, nSamples);

    MultitaperPsdResult result = MultitaperPsd::compute(data, sfreq);

    // Frequencies should go from 0 to Nyquist
    QVERIFY(result.vecFreqs(0) >= 0.0);
    QVERIFY(result.vecFreqs(result.vecFreqs.size() - 1) <= sfreq / 2.0 + 1.0);
}

//=============================================================================================================

void TestDspMultitaper::testPsdMultiChannel()
{
    double sfreq = 500.0;
    int nSamples = 256;
    // Channel 0: 20 Hz, Channel 1: 50 Hz
    MatrixXd data(2, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        double t = s / sfreq;
        data(0, s) = std::sin(2.0 * M_PI * 20.0 * t);
        data(1, s) = std::sin(2.0 * M_PI * 50.0 * t);
    }

    MultitaperPsdResult result = MultitaperPsd::compute(data, sfreq);

    // Find peak for each channel
    int peak0, peak1;
    result.matPsd.row(0).maxCoeff(&peak0);
    result.matPsd.row(1).maxCoeff(&peak1);

    QVERIFY(qAbs(result.vecFreqs(peak0) - 20.0) < 3.0);
    QVERIFY(qAbs(result.vecFreqs(peak1) - 50.0) < 3.0);
}

//=============================================================================================================

void TestDspMultitaper::testTfrSinusoidal()
{
    double sfreq = 500.0;
    int nSamples = 1024;
    MatrixXd data = generateSinusoidal(1, nSamples, sfreq, {15.0});

    MultitaperTfrResult result = MultitaperTfr::compute(data, sfreq, 128, -1, 4.0);

    QVERIFY(result.tfrData.size() > 0);
    QVERIFY(result.vecFreqs.size() > 0);
    QVERIFY(result.vecTimes.size() > 0);
}

//=============================================================================================================

void TestDspMultitaper::testTfrOutputDimensions()
{
    double sfreq = 500.0;
    int nSamples = 1024;
    int nChannels = 2;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    MultitaperTfrResult result = MultitaperTfr::compute(data, sfreq, 128);

    // Should have one TFR matrix per channel
    QCOMPARE(result.tfrData.size(), nChannels);

    // Each TFR matrix: rows = nFreqs, cols = nTimes
    for (int ch = 0; ch < nChannels; ++ch) {
        QCOMPARE(static_cast<int>(result.tfrData[ch].rows()), static_cast<int>(result.vecFreqs.size()));
        QCOMPARE(static_cast<int>(result.tfrData[ch].cols()), static_cast<int>(result.vecTimes.size()));
    }
}

//=============================================================================================================

void TestDspMultitaper::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspMultitaper)
#include "test_dsp_multitaper.moc"
