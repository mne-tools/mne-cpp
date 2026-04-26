//=============================================================================================================
/**
 * @file     test_dsp_csd.cpp
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
 * @brief    Tests for Cross-Spectral Density (CSD) computation.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/csd.h>

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
 * DECLARE CLASS TestDspCsd
 *
 * @brief The TestDspCsd class provides tests for CSD computation.
 */
class TestDspCsd : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // computeMultitaper
    void testMultitaperBasic();
    void testMultitaperDimensions();
    void testMultitaperHermitian();
    void testMultitaperDiagonalPositive();

    // computeFourier
    void testFourierBasic();
    void testFourierDimensions();
    void testFourierHermitian();

    // computeMorlet
    void testMorletBasic();
    void testMorletDimensions();

    // Frequency range filtering
    void testFreqRangeFiltering();

    void cleanupTestCase();
};

//=============================================================================================================

void TestDspCsd::initTestCase()
{
}

//=============================================================================================================

void TestDspCsd::testMultitaperBasic()
{
    int nChannels = 3;
    // DPSS solve is slow under coverage instrumentation; keep N modest
    int nSamples = 256;
    double sfreq = 500.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeMultitaper(data, sfreq);

    QVERIFY(result.matCsd.rows() == nChannels);
    QVERIFY(result.matCsd.cols() == nChannels);
    QVERIFY(result.vecFreqs.size() > 0);
}

//=============================================================================================================

void TestDspCsd::testMultitaperDimensions()
{
    int nChannels = 4;
    // DPSS computation is O(N^3); keep nSamples modest to keep tests fast
    int nSamples = 512;
    double sfreq = 1000.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeMultitaper(data, sfreq);

    // Averaged CSD should be nChannels x nChannels
    QCOMPARE(static_cast<int>(result.matCsd.rows()), nChannels);
    QCOMPARE(static_cast<int>(result.matCsd.cols()), nChannels);

    // CSD-by-freq should have one matrix per frequency bin
    if (result.csdByFreq.size() > 0) {
        QCOMPARE(result.csdByFreq.size(), static_cast<int>(result.vecFreqs.size()));
        QCOMPARE(static_cast<int>(result.csdByFreq[0].rows()), nChannels);
        QCOMPARE(static_cast<int>(result.csdByFreq[0].cols()), nChannels);
    }
}

//=============================================================================================================

void TestDspCsd::testMultitaperHermitian()
{
    // CSD matrix should be Hermitian: CSD(i,j) = conj(CSD(j,i))
    int nChannels = 3;
    int nSamples = 256;
    double sfreq = 500.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeMultitaper(data, sfreq);

    for (int i = 0; i < nChannels; ++i) {
        for (int j = i + 1; j < nChannels; ++j) {
            std::complex<double> cij = result.matCsd(i, j);
            std::complex<double> cji = result.matCsd(j, i);
            QVERIFY(std::abs(cij - std::conj(cji)) < 1e-10);
        }
    }
}

//=============================================================================================================

void TestDspCsd::testMultitaperDiagonalPositive()
{
    // Auto-spectral density (diagonal) should be real and non-negative
    int nChannels = 3;
    int nSamples = 256;
    double sfreq = 500.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeMultitaper(data, sfreq);

    for (int i = 0; i < nChannels; ++i) {
        QVERIFY(result.matCsd(i, i).real() >= 0.0);
        QVERIFY(qAbs(result.matCsd(i, i).imag()) < 1e-10);
    }
}

//=============================================================================================================

void TestDspCsd::testFourierBasic()
{
    int nChannels = 2;
    int nSamples = 2048;
    double sfreq = 500.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeFourier(data, sfreq);

    QVERIFY(result.matCsd.rows() == nChannels);
    QVERIFY(result.matCsd.cols() == nChannels);
    QVERIFY(result.vecFreqs.size() > 0);
}

//=============================================================================================================

void TestDspCsd::testFourierDimensions()
{
    int nChannels = 3;
    int nSamples = 1024;
    double sfreq = 500.0;
    int nFft = 256;

    MatrixXd data = MatrixXd::Random(nChannels, nSamples);
    CsdResult result = Csd::computeFourier(data, sfreq, 0.0, -1.0, nFft, 0.5);

    QCOMPARE(static_cast<int>(result.matCsd.rows()), nChannels);
    QCOMPARE(static_cast<int>(result.matCsd.cols()), nChannels);
}

//=============================================================================================================

void TestDspCsd::testFourierHermitian()
{
    int nChannels = 3;
    int nSamples = 1024;
    double sfreq = 500.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult result = Csd::computeFourier(data, sfreq);

    for (int i = 0; i < nChannels; ++i) {
        for (int j = i + 1; j < nChannels; ++j) {
            std::complex<double> cij = result.matCsd(i, j);
            std::complex<double> cji = result.matCsd(j, i);
            QVERIFY(std::abs(cij - std::conj(cji)) < 1e-10);
        }
    }
}

//=============================================================================================================

void TestDspCsd::testMorletBasic()
{
    int nChannels = 2;
    int nSamples = 1000;
    double sfreq = 500.0;
    RowVectorXd frequencies(3);
    frequencies << 10.0, 20.0, 30.0;

    MatrixXd data = MatrixXd::Random(nChannels, nSamples);
    CsdResult result = Csd::computeMorlet(data, sfreq, frequencies);

    QVERIFY(result.matCsd.rows() == nChannels);
    QVERIFY(result.matCsd.cols() == nChannels);
}

//=============================================================================================================

void TestDspCsd::testMorletDimensions()
{
    int nChannels = 2;
    int nSamples = 1000;
    double sfreq = 500.0;
    RowVectorXd frequencies(5);
    frequencies << 5.0, 10.0, 15.0, 20.0, 25.0;

    MatrixXd data = MatrixXd::Random(nChannels, nSamples);
    CsdResult result = Csd::computeMorlet(data, sfreq, frequencies);

    QCOMPARE(static_cast<int>(result.matCsd.rows()), nChannels);
    QCOMPARE(static_cast<int>(result.matCsd.cols()), nChannels);
    QCOMPARE(static_cast<int>(result.vecFreqs.size()), 5);
}

//=============================================================================================================

void TestDspCsd::testFreqRangeFiltering()
{
    int nChannels = 2;
    // Keep nSamples small to avoid slow DPSS solve
    int nSamples = 512;
    double sfreq = 1000.0;
    MatrixXd data = MatrixXd::Random(nChannels, nSamples);

    CsdResult resultFull = Csd::computeMultitaper(data, sfreq, 0.0, -1.0);
    CsdResult resultBand = Csd::computeMultitaper(data, sfreq, 8.0, 30.0);

    // Band-limited result should have fewer frequency bins
    QVERIFY(resultBand.vecFreqs.size() <= resultFull.vecFreqs.size());

    // All frequencies should be within the requested range
    for (int i = 0; i < resultBand.vecFreqs.size(); ++i) {
        QVERIFY(resultBand.vecFreqs(i) >= 8.0 - 1.0);
        QVERIFY(resultBand.vecFreqs(i) <= 30.0 + 1.0);
    }
}

//=============================================================================================================

void TestDspCsd::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspCsd)
#include "test_dsp_csd.moc"
