//=============================================================================================================
/**
 * @file     test_dsp_bad_channel_detect.cpp
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
 * @brief    Unit tests for BadChannelDetect.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/bad_channel_detect.h>

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
// HELPERS
//=============================================================================================================

namespace {

/** Build an n_ch × n_samp matrix where all channels are correlated sinusoids. */
MatrixXd makeSineMatrix(int nCh, int nSamp, double sFreq, double freqHz)
{
    MatrixXd m(nCh, nSamp);
    for (int ch = 0; ch < nCh; ++ch) {
        double phase = ch * 0.1;  // small phase offset so channels are distinct but correlated
        for (int t = 0; t < nSamp; ++t)
            m(ch, t) = std::sin(2.0 * M_PI * freqHz * t / sFreq + phase);
    }
    return m;
}

} // anonymous namespace

//=============================================================================================================

class TestDspBadChannelDetect : public QObject
{
    Q_OBJECT

private slots:
    // ---- detectFlat ----
    void detectFlat_identifiesZeroChannel();
    void detectFlat_identifiesSubThresholdChannel();
    void detectFlat_leavesNormalChannelUnflagged();
    void detectFlat_allFlatReturnsAllIndices();

    // ---- detectHighVariance ----
    void detectHighVariance_identifiesNoisyChannel();
    void detectHighVariance_leavesNormalChannelsUnflagged();
    void detectHighVariance_tooFewChannels_returnsEmpty();

    // ---- detectLowCorrelation ----
    void detectLowCorrelation_identifiesUncorrelatedChannel();
    void detectLowCorrelation_correlatedChannels_unflagged();
    void detectLowCorrelation_singleChannel_returnsEmpty();

    // ---- detect (combined) ----
    void detect_unionOfAllThree();
    void detect_noDefaults_clean_returnsEmpty();
};

//=============================================================================================================
// detectFlat tests
//=============================================================================================================

void TestDspBadChannelDetect::detectFlat_identifiesZeroChannel()
{
    const int nCh = 5, nSamp = 1000;
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);
    // Set channel 2 to zero (flat)
    data.row(2).setZero();

    auto bad = BadChannelDetect::detectFlat(data, 1e-10);
    QVERIFY(bad.contains(2));
    QVERIFY(!bad.contains(0));
}

//=============================================================================================================

void TestDspBadChannelDetect::detectFlat_identifiesSubThresholdChannel()
{
    const int nCh = 4, nSamp = 500;
    MatrixXd data = MatrixXd::Ones(nCh, nSamp);
    // Channel 1 has PTP = 1e-15 < threshold 1e-13
    data.row(1) = RowVectorXd::Constant(nSamp, 0.0) + RowVectorXd::LinSpaced(nSamp, 0.0, 1e-15);

    auto bad = BadChannelDetect::detectFlat(data, 1e-13);
    QVERIFY(bad.contains(1));
}

//=============================================================================================================

void TestDspBadChannelDetect::detectFlat_leavesNormalChannelUnflagged()
{
    const int nCh = 3, nSamp = 500;
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 20.0);

    auto bad = BadChannelDetect::detectFlat(data, 1e-13);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================

void TestDspBadChannelDetect::detectFlat_allFlatReturnsAllIndices()
{
    MatrixXd data = MatrixXd::Zero(4, 200);
    auto bad = BadChannelDetect::detectFlat(data, 1e-13);
    QCOMPARE(bad.size(), 4);
}

//=============================================================================================================
// detectHighVariance tests
//=============================================================================================================

void TestDspBadChannelDetect::detectHighVariance_identifiesNoisyChannel()
{
    const int nCh = 20, nSamp = 2000;
    // All channels: small sine (amplitude ~1e-12, representing fT)
    MatrixXd data(nCh, nSamp);
    for (int ch = 0; ch < nCh; ++ch)
        for (int t = 0; t < nSamp; ++t)
            data(ch, t) = 1e-12 * std::sin(2.0 * M_PI * 10.0 * t / 1000.0);

    // Channel 5: large noise (1000x higher amplitude)
    for (int t = 0; t < nSamp; ++t)
        data(5, t) = 1e-9 * std::sin(2.0 * M_PI * 50.0 * t / 1000.0 + 0.7);

    auto bad = BadChannelDetect::detectHighVariance(data, 4.0);
    QVERIFY(bad.contains(5));
}

//=============================================================================================================

void TestDspBadChannelDetect::detectHighVariance_leavesNormalChannelsUnflagged()
{
    const int nCh = 10, nSamp = 1000;
    // All channels identical amplitude sine
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);

    auto bad = BadChannelDetect::detectHighVariance(data, 4.0);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================

void TestDspBadChannelDetect::detectHighVariance_tooFewChannels_returnsEmpty()
{
    MatrixXd data = MatrixXd::Random(2, 500);
    auto bad = BadChannelDetect::detectHighVariance(data, 4.0);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================
// detectLowCorrelation tests
//=============================================================================================================

void TestDspBadChannelDetect::detectLowCorrelation_identifiesUncorrelatedChannel()
{
    const int nCh = 10, nSamp = 2000;
    // All channels: correlated (same 10 Hz sine with small phase offsets)
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);

    // Channel 4: replace with uncorrelated white noise
    srand(42);
    for (int t = 0; t < nSamp; ++t)
        data(4, t) = static_cast<double>(rand()) / RAND_MAX - 0.5;

    auto bad = BadChannelDetect::detectLowCorrelation(data, 0.4, 3);
    QVERIFY(bad.contains(4));
}

//=============================================================================================================

void TestDspBadChannelDetect::detectLowCorrelation_correlatedChannels_unflagged()
{
    const int nCh = 8, nSamp = 2000;
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);

    auto bad = BadChannelDetect::detectLowCorrelation(data, 0.4, 3);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================

void TestDspBadChannelDetect::detectLowCorrelation_singleChannel_returnsEmpty()
{
    MatrixXd data = MatrixXd::Random(1, 500);
    auto bad = BadChannelDetect::detectLowCorrelation(data, 0.4, 3);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================
// Combined detect() tests
//=============================================================================================================

void TestDspBadChannelDetect::detect_unionOfAllThree()
{
    const int nCh = 12, nSamp = 2000;
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);

    // Channel 0: flat
    data.row(0).setZero();
    // Channel 6: high variance (noisy)
    for (int t = 0; t < nSamp; ++t)
        data(6, t) = 1e3 * (static_cast<double>(rand()) / RAND_MAX - 0.5);

    BadChannelDetect::Params p;
    p.dFlatThreshold = 1e-10;
    p.dVarZThresh    = 4.0;
    p.dCorrThresh    = 0.4;
    p.iNeighbours    = 3;

    auto bad = BadChannelDetect::detect(data, p);

    // Both flagged channels must appear exactly once
    QVERIFY(bad.contains(0));
    QVERIFY(bad.contains(6));

    // No duplicates
    QVector<int> sorted = bad;
    std::sort(sorted.begin(), sorted.end());
    auto it = std::unique(sorted.begin(), sorted.end());
    QCOMPARE(it, sorted.end());
}

//=============================================================================================================

void TestDspBadChannelDetect::detect_noDefaults_clean_returnsEmpty()
{
    // Completely clean data — all channels correlated, moderate amplitude
    const int nCh = 8, nSamp = 2000;
    MatrixXd data = makeSineMatrix(nCh, nSamp, 1000.0, 10.0);

    BadChannelDetect::Params p;
    p.dFlatThreshold = 1e-13;
    p.dVarZThresh    = 4.0;
    p.dCorrThresh    = 0.4;

    auto bad = BadChannelDetect::detect(data, p);
    QVERIFY(bad.isEmpty());
}

//=============================================================================================================

QTEST_MAIN(TestDspBadChannelDetect)
#include "test_dsp_bad_channel_detect.moc"
