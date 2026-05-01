//=============================================================================================================
/**
 * @file     test_dsp_annotate_artifact.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    Tests for annotateMusclZscore and annotateAmplitude.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/annotate_artifact.h>
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

namespace {

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

/**
 * @brief Inject a high-frequency burst into data.
 */
void injectBurst(MatrixXd& data, double sfreq, double burstFreq,
                 int startSample, int endSample, double amplitude = 10.0)
{
    for (int s = startSample; s <= endSample && s < static_cast<int>(data.cols()); ++s) {
        const double t = static_cast<double>(s) / sfreq;
        const double val = amplitude * std::sin(2.0 * M_PI * burstFreq * t);
        for (Eigen::Index ch = 0; ch < data.rows(); ++ch)
            data(ch, s) += val;
    }
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
    void testMusclZscoreDetectsHighFreq();
    void testMusclZscoreCleanData();
    void testMusclZscoreThreshold();
    void testMusclZscoreMinDuration();
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

void TestDspAnnotateArtifact::testMusclZscoreDetectsHighFreq()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 2000; // 2 seconds
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = makeCleanSine(nCh, nSamples, sfreq, 10.0);

    // Inject strong 120 Hz burst at samples 500-700 (200 ms)
    injectBurst(data, sfreq, 120.0, 500, 700, 50.0);

    AnnotateMusclParams params;
    params.dThreshold = 3.0;
    params.dMinDuration = 0.02; // 20 ms — ensure burst is long enough
    FiffAnnotations annot = annotateMusclZscore(data, info, sfreq, params);

    QVERIFY2(annot.size() > 0, "Should detect muscle artifact from 120 Hz burst");

    // The annotation should overlap with the burst region (0.5 - 0.7 s)
    bool foundOverlap = false;
    for (int i = 0; i < annot.size(); ++i) {
        const double aStart = annot[i].onset;
        const double aEnd = annot[i].onset + annot[i].duration;
        if (aEnd > 0.5 && aStart < 0.7) {
            foundOverlap = true;
            QCOMPARE(annot[i].description, QStringLiteral("BAD_muscle"));
        }
    }
    QVERIFY2(foundOverlap, "Annotation should overlap with the burst region");
}

//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreCleanData()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 2000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = makeCleanSine(nCh, nSamples, sfreq, 10.0);

    AnnotateMusclParams params;
    params.dThreshold = 5.0;
    FiffAnnotations annot = annotateMusclZscore(data, info, sfreq, params);

    QCOMPARE(annot.size(), 0);
}

//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreThreshold()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 2000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = makeCleanSine(nCh, nSamples, sfreq, 10.0);
    injectBurst(data, sfreq, 120.0, 500, 700, 50.0);

    // Low threshold — should detect
    AnnotateMusclParams paramsLow;
    paramsLow.dThreshold = 2.0;
    paramsLow.dMinDuration = 0.01;
    FiffAnnotations annotLow = annotateMusclZscore(data, info, sfreq, paramsLow);

    // Very high threshold — should not detect
    AnnotateMusclParams paramsHigh;
    paramsHigh.dThreshold = 100.0;
    paramsHigh.dMinDuration = 0.01;
    FiffAnnotations annotHigh = annotateMusclZscore(data, info, sfreq, paramsHigh);

    QVERIFY2(annotLow.size() > 0, "Low threshold should detect the burst");
    QCOMPARE(annotHigh.size(), 0);
}

//=============================================================================================================

void TestDspAnnotateArtifact::testMusclZscoreMinDuration()
{
    const double sfreq = 1000.0;
    const int nCh = 3;
    const int nSamples = 2000;
    FiffInfo info = makeMegInfo(nCh, sfreq);
    MatrixXd data = makeCleanSine(nCh, nSamples, sfreq, 10.0);

    // Very short burst: only 10 samples = 10 ms
    injectBurst(data, sfreq, 120.0, 500, 509, 80.0);

    AnnotateMusclParams params;
    params.dThreshold = 3.0;
    params.dMinDuration = 0.5; // 500 ms minimum — should filter out the short burst

    FiffAnnotations annot = annotateMusclZscore(data, info, sfreq, params);
    QCOMPARE(annot.size(), 0);
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
        const double expectedOnset = 100.0 / sfreq;     // 0.1 s
        const double expectedDur   = 10.0 / sfreq;      // 0.01 s

        if (std::abs(annot[i].onset - expectedOnset) < 1e-6 &&
            std::abs(annot[i].duration - expectedDur) < 1e-6)
        {
            foundTiming = true;
        }
    }
    QVERIFY2(foundTiming, "Annotation onset and duration should match sample positions exactly");
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspAnnotateArtifact)

#include "test_dsp_annotate_artifact.moc"
