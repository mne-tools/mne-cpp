//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_annotate_artifact.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for annotateMusclZscore and annotateAmplitude.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/annotate_artifact.h>
#include <dsp/firfilter.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// HELPERS
//=============================================================================================================

namespace
{

/**
 * @brief Build a minimal FiffInfo with nCh MEG channels at given sfreq.
 */
FiffInfo makeMegInfo(int nCh, double sfreq)
{
    FiffInfo info;
    info.nchan = nCh;
    info.chs.clear();
    info.ch_names.clear();
    for (int i = 0; i < nCh; ++i) {
        FiffChInfo ch;
        ch.scanNo = i + 1;
        ch.logNo = i + 1;
        ch.kind = FIFFV_MEG_CH;
        ch.range = 1.0f;
        ch.cal = 1.0f;
        ch.unit = 0;
        ch.unit_mul = 0;
        ch.ch_name = QString("MEG%1").arg(i + 1, 3, 10, QLatin1Char('0'));
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    Q_UNUSED(sfreq)
    return info;
}

/**
 * @brief FiffInfo with nCh magnetometers (unit T), as mne.create_info(..., "mag") gives.
 */
FiffInfo makeMagInfo(int nCh)
{
    FiffInfo info = makeMegInfo(nCh, 1000.0);
    for (FiffChInfo& ch : info.chs)
        ch.unit = FIFF_UNIT_T;
    return info;
}

/**
 * @brief 4 x 3000 samples at 1 kHz: eight incommensurate background sines per channel and
 * 120 Hz bursts at 0.8-1.4 s, 2.0-2.3 s and 2.6-2.62 s with 20 ms ramps (the mne oracle input).
 */
MatrixXd muscleData()
{
    const double freqs[8] = {7.3, 31.7, 57.1, 93.9, 151.3, 233.9, 317.1, 411.7};
    const double bursts[3][3] = {{0.8, 1.4, 2e-11}, {2.0, 2.3, 1.5e-11}, {2.6, 2.62, 3e-11}};
    MatrixXd data = MatrixXd::Zero(4, 3000);
    for (int s = 0; s < data.cols(); ++s) {
        const double t = s / 1000.0;
        double burst = 0.0;
        for (const auto& b : bursts)
            burst += b[2] * std::clamp(std::min(t - b[0], b[1] - t) / 0.02, 0.0, 1.0) * std::sin(2.0 * M_PI * 120.0 * t);
        for (int c = 0; c < 4; ++c) {
            for (int k = 0; k < 8; ++k)
                data(c, s) += 1e-12 * std::sin(2.0 * M_PI * freqs[k] * t + 0.7 * c + 1.3 * k);
            data(c, s) += burst;
        }
    }
    return data;
}

/**
 * @brief Create clean 10 Hz sine data.
 */
MatrixXd makeCleanSine(int nCh, int nSamples, double sfreq, double freqHz = 10.0)
{
    MatrixXd data = MatrixXd::Zero(nCh, nSamples);
    for (int s = 0; s < nSamples; ++s) {
        const double t = static_cast<double>(s) / sfreq;
        const double val = std::sin(2.0 * M_PI * freqHz * t);
        for (int ch = 0; ch < nCh; ++ch)
            data(ch, s) = val;
    }
    return data;
}

} // anonymous namespace

//=============================================================================================================
/**
 * @brief Tests for annotation-based artifact detectors.
 */
class TestDspAnnotateArtifact : public QObject
{
    Q_OBJECT

private slots:
    void testMusclZscoreMatchesPython_data();
    void testMusclZscoreMatchesPython();
    void testMusclZscoreCleanData();
    void testFirFilterMatchesPython();
    void testAmplitudeExceedMax();
    void testAmplitudeBelowMin();
    void testAmplitudeFlat();
    void testAmplitudeNoArtifact();
    void testEmptyData();
    void testAnnotationTiming();
};

//=============================================================================================================
// Muscle artifact tests
//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreMatchesPython_data()
{
    QTest::addColumn<double>("threshold");
    QTest::addColumn<QVector<double>>("onsets");
    QTest::addColumn<QVector<double>>("durations");

    // annotate_muscle_zscore(RawArray(muscleData(), 4 mags at 1 kHz), threshold=..., min_length_good=0.1)
    QTest::newRow("threshold 4") << 4.0 << QVector<double>{} << QVector<double>{};
    QTest::newRow("threshold 2.5") << 2.5 << QVector<double>{0.84, 2.117} << QVector<double>{0.521, 0.073};
}

//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreMatchesPython()
{
    QFETCH(double, threshold);
    QFETCH(QVector<double>, onsets);
    QFETCH(QVector<double>, durations);

    AnnotateMusclParams params;
    params.dThreshold = threshold;
    RowVectorXd scores;
    const FiffAnnotations annot = annotateMusclZscore(muscleData(), makeMagInfo(4), 1000.0, params, &scores);

    // scores_muscle: sum |s| 5126.557472094637, s[1000] 3.310615511574129, s[1700] -1.4815054738007716
    QVERIFY2(std::fabs(scores.cwiseAbs().sum() - 5126.557472094637) < 1e-6 * 5126.557472094637,
             qPrintable(QString("score sum %1").arg(scores.cwiseAbs().sum(), 0, 'g', 17)));
    QVERIFY(std::fabs(scores(1000) - 3.310615511574129) < 1e-6);
    QVERIFY(std::fabs(scores(1700) + 1.4815054738007716) < 1e-6);

    QCOMPARE(annot.size(), onsets.size());
    for (int i = 0; i < annot.size(); ++i) {
        QVERIFY(std::fabs(annot[i].onset - onsets[i]) < 1e-9);
        QVERIFY(std::fabs(annot[i].duration - durations[i]) < 1e-9);
        QCOMPARE(annot[i].description, QStringLiteral("BAD_muscle"));
    }
}

//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreCleanData()
{
    const double sfreq = 1000.0;
    MatrixXd data = makeCleanSine(3, 2000, sfreq, 10.0);
    // A pure tone has no band power except the filter's start transient, which mne marks too:
    // annotate_muscle_zscore on 3 mags of sin(2 pi 10 t) gives onset 0, duration 0.109.
    const FiffAnnotations annot = annotateMusclZscore(data, makeMagInfo(3), sfreq);
    QCOMPARE(annot.size(), 1);
    QVERIFY(std::fabs(annot[0].onset) < 1e-12 && std::fabs(annot[0].duration - 0.109) < 1e-9);
    // Channels that are neither MEG nor EEG give no annotation.
    QTest::ignoreMessage(QtWarningMsg, "annotateMusclZscore: no MEG or EEG channels found");
    QCOMPARE(annotateMusclZscore(data, FiffInfo(), sfreq).size(), 0);
}

//=============================================================================================================

void TestDspAnnotateArtifact::testFirFilterMatchesPython()
{
    // mne.filter.create_filter(None, 1000, l_freq, h_freq, fir_design="firwin")
    const RowVectorXd band = FirFilter::designMne(1000.0, 110.0, 140.0);
    QCOMPARE(band.size(), 121);
    QVERIFY(std::fabs(band.cwiseAbs().sum() - 1.678800064409924) < 1e-12);
    QVERIFY(std::fabs(band(60) - 0.12204211470179285) < 1e-14);
    QVERIFY(std::fabs(band(10) - 0.000833491119518251) < 1e-14);
    const RowVectorXd low = FirFilter::designMne(1000.0, -1.0, 4.0);
    QCOMPARE(low.size(), 1651);
    QVERIFY(std::fabs(low.cwiseAbs().sum() - 1.7219281349401758) < 1e-12);
    const RowVectorXd high = FirFilter::designMne(1000.0, 1.0, -1.0);
    QCOMPARE(high.size(), 3301);
    QVERIFY(std::fabs(high(1650) - 0.9989951297110371) < 1e-14);

    // mne.filter.filter_data(muscleData(), 1000, 110, 140, fir_design="firwin")
    const MatrixXd filtered = FirFilter::filterData(muscleData(), 1000.0, 110.0, 140.0);
    QVERIFY2(std::fabs(filtered.cwiseAbs().sum() - 4.506005234345971e-08) < 1e-6 * 4.506005234345971e-08,
             qPrintable(QString("filtered sum %1").arg(filtered.cwiseAbs().sum(), 0, 'g', 17)));
    QVERIFY(std::fabs(filtered(2, 1500) + 3.2877804115954136e-13) < 1e-6 * 3.2877804115954136e-13);
}

//=============================================================================================================
// Amplitude tests
//=============================================================================================================

void TestDspAnnotateArtifact::testAmplitudeExceedMax()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 1000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = MatrixXd::Constant(nCh, nSamples, 1.0);

    // Inject spike at sample 400 on channel 0
    data(0, 400) = 100.0;

    AnnotateAmplitudeParams params;
    params.dPeakMax = 50.0;

    FiffAnnotations annot = annotateAmplitude(data, info, sfreq, params);

    QVERIFY2(annot.size() > 0, "Should detect spike exceeding max");
    bool foundSpike = false;
    for (int i = 0; i < annot.size(); ++i) {
        const double sampleOnset = annot[i].onset * sfreq;
        if (std::abs(sampleOnset - 400.0) < 1.5) {
            foundSpike = true;
            QCOMPARE(annot[i].description, QStringLiteral("BAD_amplitude"));
            QVERIFY(!annot[i].channelNames.isEmpty());
            QCOMPARE(annot[i].channelNames.first(), QStringLiteral("MEG001"));
        }
    }
    QVERIFY2(foundSpike, "Annotation should be at the spike position");
}

//=============================================================================================================

void TestDspAnnotateArtifact::testAmplitudeBelowMin()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 1000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = MatrixXd::Constant(nCh, nSamples, 1.0);

    // Inject negative spike at sample 300 on channel 1
    data(1, 300) = -200.0;

    AnnotateAmplitudeParams params;
    params.dPeakMin = -100.0;

    FiffAnnotations annot = annotateAmplitude(data, info, sfreq, params);

    QVERIFY2(annot.size() > 0, "Should detect sample below min");
    bool foundSpike = false;
    for (int i = 0; i < annot.size(); ++i) {
        if (!annot[i].channelNames.isEmpty() && annot[i].channelNames.first() == "MEG002") {
            foundSpike = true;
            QCOMPARE(annot[i].description, QStringLiteral("BAD_amplitude"));
        }
    }
    QVERIFY2(foundSpike, "Annotation should be on channel MEG002");
}

//=============================================================================================================

void TestDspAnnotateArtifact::testAmplitudeFlat()
{
    const double sfreq = 1000.0;
    const int nCh = 1;
    const int nSamples = 1000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = MatrixXd::Zero(nCh, nSamples); // completely flat

    AnnotateAmplitudeParams params;
    params.dFlatMin = 1e-6; // anything below this p2p is flat
    params.dWindowSec = 0.1;

    FiffAnnotations annot = annotateAmplitude(data, info, sfreq, params);

    QVERIFY2(annot.size() > 0, "Should detect flat segment");
    for (int i = 0; i < annot.size(); ++i) {
        QCOMPARE(annot[i].description, QStringLiteral("BAD_flat"));
    }
}

//=============================================================================================================

void TestDspAnnotateArtifact::testAmplitudeNoArtifact()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 1000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = makeCleanSine(nCh, nSamples, sfreq, 10.0);

    AnnotateAmplitudeParams params;
    params.dPeakMax = 1000.0;
    params.dPeakMin = -1000.0;

    FiffAnnotations annot = annotateAmplitude(data, info, sfreq, params);
    QCOMPARE(annot.size(), 0);
}

//=============================================================================================================

void TestDspAnnotateArtifact::testEmptyData()
{
    FiffInfo info = makeMegInfo(3, 1000.0);

    MatrixXd emptyData(0, 0);

    AnnotateMusclParams mParams;
    FiffAnnotations mAnnot = annotateMusclZscore(emptyData, info, 1000.0, mParams);
    QCOMPARE(mAnnot.size(), 0);

    AnnotateAmplitudeParams aParams;
    aParams.dPeakMax = 10.0;
    FiffAnnotations aAnnot = annotateAmplitude(emptyData, info, 1000.0, aParams);
    QCOMPARE(aAnnot.size(), 0);
}

//=============================================================================================================

void TestDspAnnotateArtifact::testAnnotationTiming()
{
    const double sfreq = 1000.0;
    const int nCh = 1;
    const int nSamples = 2000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = MatrixXd::Constant(nCh, nSamples, 5.0);

    // Create a spike block at samples 100-109 (10 ms at 1000 Hz)
    for (int s = 100; s <= 109; ++s)
        data(0, s) = 1000.0;

    AnnotateAmplitudeParams params;
    params.dPeakMax = 500.0;

    FiffAnnotations annot = annotateAmplitude(data, info, sfreq, params);

    QVERIFY2(annot.size() >= 1, "Should find at least one annotation");

    // Find annotation covering the 100-109 block
    bool foundTiming = false;
    for (int i = 0; i < annot.size(); ++i) {
        const double expectedOnset = 100.0 / sfreq; // 0.1 s
        const double expectedDur = 10.0 / sfreq;    // 0.01 s

        if (std::abs(annot[i].onset - expectedOnset) < 1e-6 &&
            std::abs(annot[i].duration - expectedDur) < 1e-6) {
            foundTiming = true;
        }
    }
    QVERIFY2(foundTiming, "Annotation onset and duration should match sample positions exactly");
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspAnnotateArtifact)

#include "test_dsp_annotate_artifact.moc"
