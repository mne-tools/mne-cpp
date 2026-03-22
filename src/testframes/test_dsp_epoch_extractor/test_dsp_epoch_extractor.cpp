//=============================================================================================================
/**
 * @file     test_dsp_epoch_extractor.cpp
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
 * @brief    Unit tests for EpochExtractor.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/epoch_extractor.h>
#include <mne/mne_epoch_data.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QVector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestDspEpochExtractor : public QObject
{
    Q_OBJECT

private slots:
    /** Basic extraction: correct number of epochs and correct dimensions. */
    void extract_basicDimensions();

    /** Epoch data matches the expected raw data slice. */
    void extract_dataValues_matchRaw();

    /** Events near boundaries that would produce out-of-range windows are skipped. */
    void extract_outOfBounds_skipped();

    /** Baseline correction zeros the mean of the baseline interval. */
    void extract_baselineCorrection_zerosMean();

    /** Amplitude rejection marks epochs above threshold. */
    void extract_rejection_marksHighAmplitude();

    /** Amplitude rejection leaves good epochs unmarked. */
    void extract_rejection_keepsgoodEpochs();

    /** Event codes are stored correctly when provided. */
    void extract_eventCodes_stored();

    /** rejectMarked() returns only non-rejected epochs. */
    void rejectMarked_returnsGoodOnly();

    /** average() returns the correct grand average of non-rejected epochs. */
    void average_correctGrandAverage();

    /** average() returns empty matrix when all epochs are rejected. */
    void average_allRejected_returnsEmpty();

    /** extract() on empty data returns empty vector. */
    void extract_emptyInput_returnsEmpty();
};

//=============================================================================================================

void TestDspEpochExtractor::extract_basicDimensions()
{
    const int nCh    = 4;
    const double sFreq = 1000.0;
    // Raw data: 3 seconds
    MatrixXd raw = MatrixXd::Random(nCh, static_cast<int>(3.0 * sFreq));

    QVector<int> events = {500, 1000, 1500, 2000};  // 4 events

    EpochExtractor::Params p;
    p.dTmin    = -0.2;
    p.dTmax    =  0.5;
    p.bApplyBaseline = false;
    p.dThreshold = 0.0;

    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 4);

    // Each epoch should be nCh × epochLen
    int expectedLen = static_cast<int>(std::round((p.dTmax - p.dTmin) * sFreq)) + 1;
    for (const auto& ep : epochs) {
        QCOMPARE(ep.epoch.rows(), nCh);
        QCOMPARE(ep.epoch.cols(), expectedLen);
    }
}

//=============================================================================================================

void TestDspEpochExtractor::extract_dataValues_matchRaw()
{
    const int nCh    = 2;
    const double sFreq = 500.0;
    const int nSamp  = 2000;
    MatrixXd raw     = MatrixXd::Random(nCh, nSamp);

    // Single event at sample 500, epoch [0, 0.1] s (no pre-stimulus)
    EpochExtractor::Params p;
    p.dTmin           = 0.0;
    p.dTmax           = 0.1;
    p.bApplyBaseline  = false;
    p.dThreshold      = 0.0;

    QVector<int> events = {500};
    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 1);

    // Expected: columns 500..550 from raw
    int epochLen = static_cast<int>(std::round(0.1 * sFreq)) + 1;
    MatrixXd expected = raw.block(0, 500, nCh, epochLen);
    QVERIFY((epochs[0].epoch - expected).norm() < 1e-10);
}

//=============================================================================================================

void TestDspEpochExtractor::extract_outOfBounds_skipped()
{
    const double sFreq = 1000.0;
    const int nSamp = 2000;
    MatrixXd raw = MatrixXd::Zero(2, nSamp);

    EpochExtractor::Params p;
    p.dTmin = -0.5;  // 500 samples pre-stimulus
    p.dTmax =  0.5;  // 500 samples post-stimulus
    p.bApplyBaseline = false;

    // event at 100: s0 = 100-500 = -400 → out of bounds (skipped)
    // event at 1000: s0=500, s1=1500 → valid
    // event at 1900: s1 = 1900+500 = 2400 >= 2000 → out of bounds (skipped)
    QVector<int> events = {100, 1000, 1900};
    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 1);
}

//=============================================================================================================

void TestDspEpochExtractor::extract_baselineCorrection_zerosMean()
{
    const double sFreq = 1000.0;
    const int nCh = 3;
    const int nSamp = 5000;

    // Raw data with a non-zero DC offset per channel
    MatrixXd raw(nCh, nSamp);
    for (int ch = 0; ch < nCh; ++ch)
        raw.row(ch) = MatrixXd::Constant(1, nSamp, static_cast<double>(ch + 1));

    QVector<int> events = {2000};
    EpochExtractor::Params p;
    p.dTmin          = -0.5;
    p.dTmax          =  0.5;
    p.dBaseMin       = -0.5;
    p.dBaseMax       =  0.0;
    p.bApplyBaseline = true;
    p.dThreshold     = 0.0;

    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 1);

    // After baseline correction the per-channel mean should be ~0
    for (int ch = 0; ch < nCh; ++ch) {
        double m = std::abs(epochs[0].epoch.row(ch).mean());
        QVERIFY2(m < 1e-10, qPrintable(QString("Channel %1 mean not zeroed: %2").arg(ch).arg(m)));
    }
}

//=============================================================================================================

void TestDspEpochExtractor::extract_rejection_marksHighAmplitude()
{
    const double sFreq = 1000.0;
    const int nSamp    = 5000;
    const int nCh     = 2;

    MatrixXd raw = MatrixXd::Zero(nCh, nSamp);
    // Insert a large spike in channel 0, in the epoch window
    raw(0, 2100) = 1.0;   // 1 V spike

    QVector<int> events = {2000};
    EpochExtractor::Params p;
    p.dTmin      = -0.5;
    p.dTmax      =  0.5;
    p.dThreshold = 0.5;   // 0.5 V threshold — spike exceeds this
    p.bApplyBaseline = false;

    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 1);
    QVERIFY(epochs[0].bReject);
}

//=============================================================================================================

void TestDspEpochExtractor::extract_rejection_keepsgoodEpochs()
{
    const double sFreq = 1000.0;
    const int nSamp    = 5000;
    const int nCh     = 2;

    // Small amplitude data — well within threshold
    MatrixXd raw = MatrixXd::Constant(nCh, nSamp, 1e-7);

    QVector<int> events = {2000};
    EpochExtractor::Params p;
    p.dTmin      = -0.5;
    p.dTmax      =  0.5;
    p.dThreshold = 150e-6;  // 150 µV — data is 100 nV, well below
    p.bApplyBaseline = false;

    auto epochs = EpochExtractor::extract(raw, events, sFreq, p);
    QCOMPARE(epochs.size(), 1);
    QVERIFY(!epochs[0].bReject);
}

//=============================================================================================================

void TestDspEpochExtractor::extract_eventCodes_stored()
{
    const double sFreq = 1000.0;
    MatrixXd raw = MatrixXd::Zero(2, 5000);

    QVector<int> events = {1000, 2000, 3000};
    QVector<int> codes  = {1, 2, 3};

    EpochExtractor::Params p;
    p.dTmin = -0.1; p.dTmax = 0.1; p.bApplyBaseline = false;

    auto epochs = EpochExtractor::extract(raw, events, sFreq, p, codes);
    QCOMPARE(epochs.size(), 3);
    QCOMPARE(epochs[0].event, 1);
    QCOMPARE(epochs[1].event, 2);
    QCOMPARE(epochs[2].event, 3);
}

//=============================================================================================================

void TestDspEpochExtractor::rejectMarked_returnsGoodOnly()
{
    MNEEpochData good1, bad1, good2;
    good1.bReject = false;
    bad1.bReject  = true;
    good2.bReject = false;

    QVector<MNEEpochData> all = {good1, bad1, good2};
    auto clean = EpochExtractor::rejectMarked(all);

    QCOMPARE(clean.size(), 2);
    for (const auto& ep : clean)
        QVERIFY(!ep.bReject);
}

//=============================================================================================================

void TestDspEpochExtractor::average_correctGrandAverage()
{
    const int nCh = 3, nSamp = 100;

    // Two epochs: one all-ones, one all-twos → mean = 1.5
    MNEEpochData ep1, ep2;
    ep1.epoch  = MatrixXd::Ones(nCh, nSamp);
    ep1.bReject = false;
    ep2.epoch  = MatrixXd::Constant(nCh, nSamp, 2.0);
    ep2.bReject = false;

    auto avg = EpochExtractor::average({ep1, ep2});

    QCOMPARE(avg.rows(), nCh);
    QCOMPARE(avg.cols(), nSamp);
    QVERIFY((avg.array() - 1.5).abs().maxCoeff() < 1e-10);
}

//=============================================================================================================

void TestDspEpochExtractor::average_allRejected_returnsEmpty()
{
    MNEEpochData ep;
    ep.bReject = true;
    ep.epoch   = MatrixXd::Ones(3, 100);

    auto avg = EpochExtractor::average({ep});
    QVERIFY(avg.size() == 0);
}

//=============================================================================================================

void TestDspEpochExtractor::extract_emptyInput_returnsEmpty()
{
    MatrixXd raw;
    QVector<int> events = {100};
    auto epochs = EpochExtractor::extract(raw, events, 1000.0);
    QVERIFY(epochs.isEmpty());
}

//=============================================================================================================

QTEST_MAIN(TestDspEpochExtractor)
#include "test_dsp_epoch_extractor.moc"
