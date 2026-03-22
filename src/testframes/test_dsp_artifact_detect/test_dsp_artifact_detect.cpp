//=============================================================================================================
/**
 * @file     test_dsp_artifact_detect.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.10
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Unit tests for ArtifactDetect (ECG/EOG event detection).
 */

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <dsp/artifact_detect.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// Helpers

static FiffInfo makeInfoWithEcgEog(int nMeg, bool hasEcg, bool hasEog)
{
    FiffInfo info;
    int idx = 0;

    // MEG channels
    for (int i = 0; i < nMeg; ++i, ++idx) {
        FiffChInfo ch;
        ch.kind = FIFFV_MEG_CH;
        ch.unit = 112;   // Tesla
        info.chs.append(ch);
    }

    if (hasEcg) {
        FiffChInfo ch;
        ch.kind    = FIFFV_ECG_CH;
        ch.ch_name = "ECG001";
        info.chs.append(ch);
        ++idx;
    }

    if (hasEog) {
        FiffChInfo ch;
        ch.kind    = FIFFV_EOG_CH;
        ch.ch_name = "EOG001";
        info.chs.append(ch);
        ++idx;
    }

    info.nchan = static_cast<int>(info.chs.size());
    return info;
}

// Build synthetic ECG-like signal: Gaussian R-peaks at regular RR intervals
static RowVectorXd makeSyntheticEcg(int nSamp, double fs, double bpm,
                                     double peakAmplitude = 1.0,
                                     double noiseLevel    = 0.05)
{
    RowVectorXd sig = RowVectorXd::Zero(nSamp);
    double rrSamples = fs * 60.0 / bpm;
    double sigma     = fs * 0.02;  // 20 ms wide Gaussian
    int firstPeak    = static_cast<int>(rrSamples * 0.3);

    for (int peak = firstPeak; peak < nSamp; peak += static_cast<int>(rrSamples)) {
        for (int j = -static_cast<int>(3 * sigma); j <= static_cast<int>(3 * sigma); ++j) {
            int s = peak + j;
            if (s >= 0 && s < nSamp) {
                sig(s) += peakAmplitude * std::exp(-0.5 * (j * j) / (sigma * sigma));
            }
        }
    }

    // Add noise
    for (int i = 0; i < nSamp; ++i) {
        sig(i) += noiseLevel * (2.0 * rand() / RAND_MAX - 1.0) * peakAmplitude;
    }

    return sig;
}

// Build synthetic EOG-like signal: periodic blinks
static RowVectorXd makeSyntheticEog(int nSamp, double fs, double blinkRateHz,
                                     double amplitude = 200e-6,
                                     double noiseLevel = 1e-6)
{
    RowVectorXd sig = RowVectorXd::Zero(nSamp);
    double blinkPeriod = fs / blinkRateHz;
    double sigma       = fs * 0.05;  // 50 ms blink
    int firstBlink     = static_cast<int>(blinkPeriod * 0.5);

    for (int blink = firstBlink; blink < nSamp; blink += static_cast<int>(blinkPeriod)) {
        for (int j = -static_cast<int>(3 * sigma); j <= static_cast<int>(3 * sigma); ++j) {
            int s = blink + j;
            if (s >= 0 && s < nSamp) {
                sig(s) += amplitude * std::exp(-0.5 * (j * j) / (sigma * sigma));
            }
        }
    }

    for (int i = 0; i < nSamp; ++i) {
        sig(i) += noiseLevel * (2.0 * rand() / RAND_MAX - 1.0);
    }

    return sig;
}

//=============================================================================================================

class TestDspArtifactDetect : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // ECG with dedicated channel
    //=========================================================================
    void ecg_detectsCorrectCount_dedicatedChannel()
    {
        const double fs   = 1000.0;
        const double bpm  = 70.0;
        const int    nSamp = 10000;  // 10 s
        const int    expectedPeaks = static_cast<int>(bpm / 60.0 * (nSamp / fs));

        FiffInfo info = makeInfoWithEcgEog(0, /*hasEcg=*/true, /*hasEog=*/false);

        MatrixXd data(1, nSamp);
        data.row(0) = makeSyntheticEcg(nSamp, fs, bpm, 1.0, 0.02);

        ArtifactDetect::EcgParams params;
        params.dThreshFactor = 0.4;
        QVector<int> peaks = ArtifactDetect::detectEcg(data, info, fs, params);

        // Allow ±2 peaks tolerance
        QVERIFY2(std::abs(peaks.size() - expectedPeaks) <= 2,
                 qPrintable(QString("Expected ~%1 peaks, got %2").arg(expectedPeaks).arg(peaks.size())));
    }

    void ecg_peakSpacingReasonable()
    {
        const double fs   = 1000.0;
        const double bpm  = 60.0;  // 1 Hz → 1000 samples between peaks
        const int    nSamp = 8000;

        FiffInfo info = makeInfoWithEcgEog(0, /*hasEcg=*/true, /*hasEog=*/false);

        MatrixXd data(1, nSamp);
        data.row(0) = makeSyntheticEcg(nSamp, fs, bpm, 1.0, 0.01);

        QVector<int> peaks = ArtifactDetect::detectEcg(data, info, fs);
        QVERIFY2(peaks.size() >= 2, "Need at least 2 peaks to check spacing");

        for (int i = 1; i < peaks.size(); ++i) {
            int spacing = peaks[i] - peaks[i - 1];
            // Spacing should be roughly 1000 ± 200 samples
            QVERIFY2(spacing > 700 && spacing < 1300,
                     qPrintable(QString("Unreasonable peak spacing: %1").arg(spacing)));
        }
    }

    void ecg_noEcgChannel_fallsBackToMeg()
    {
        const double fs    = 1000.0;
        const int    nSamp = 5000;
        const double bpm   = 65.0;

        // 5 MEG magnetometers, each carrying a scaled ECG artefact
        FiffInfo info = makeInfoWithEcgEog(5, /*hasEcg=*/false, /*hasEog=*/false);

        MatrixXd data(5, nSamp);
        RowVectorXd cardiac = makeSyntheticEcg(nSamp, fs, bpm, 1e-12, 1e-14);
        for (int i = 0; i < 5; ++i) {
            data.row(i) = cardiac * (1.0 + 0.01 * i);
        }

        // Should not crash and should detect some peaks
        QVector<int> peaks = ArtifactDetect::detectEcg(data, info, fs);
        QVERIFY2(peaks.size() > 0, "Expected some ECG peaks from MEG fallback");
    }

    void ecg_emptyChannels_returnsEmpty()
    {
        FiffInfo info;
        info.nchan = 0;
        MatrixXd data;
        QVector<int> peaks = ArtifactDetect::detectEcg(data, info, 1000.0);
        QVERIFY(peaks.isEmpty());
    }

    //=========================================================================
    // EOG
    //=========================================================================
    void eog_detectsCorrectCount()
    {
        const double fs         = 1000.0;
        const double blinkRate  = 0.3;   // 0.3 Hz → ~3 blinks in 10 s
        const int    nSamp      = 10000;
        const int    expectedBlinks = static_cast<int>(blinkRate * (nSamp / fs));

        FiffInfo info = makeInfoWithEcgEog(0, /*hasEcg=*/false, /*hasEog=*/true);

        MatrixXd data(1, nSamp);
        data.row(0) = makeSyntheticEog(nSamp, fs, blinkRate, 200e-6, 1e-7);

        ArtifactDetect::EogParams params;
        params.dThresholdV = 100e-6;
        QVector<int> events = ArtifactDetect::detectEog(data, info, fs, params);

        QVERIFY2(std::abs(events.size() - expectedBlinks) <= 1,
                 qPrintable(QString("Expected ~%1 blinks, got %2")
                            .arg(expectedBlinks).arg(events.size())));
    }

    void eog_noEogChannel_returnsEmpty()
    {
        FiffInfo info = makeInfoWithEcgEog(5, /*hasEcg=*/true, /*hasEog=*/false);
        MatrixXd data = MatrixXd::Random(6, 2000);
        QVector<int> events = ArtifactDetect::detectEog(data, info, 1000.0);
        QVERIFY(events.isEmpty());
    }

    void eog_multipleChannels_usesLargestAmplitude()
    {
        const double fs    = 1000.0;
        const int    nSamp = 5000;

        FiffInfo info;
        // Two EOG channels
        for (int i = 0; i < 2; ++i) {
            FiffChInfo ch;
            ch.kind    = FIFFV_EOG_CH;
            ch.ch_name = QString("EOG%1").arg(i + 1);
            info.chs.append(ch);
        }
        info.nchan = 2;

        MatrixXd data(2, nSamp);
        // Channel 0: large blinks
        data.row(0) = makeSyntheticEog(nSamp, fs, 0.5, 300e-6, 1e-7);
        // Channel 1: tiny signal
        data.row(1) = RowVectorXd::Constant(nSamp, 1e-9);

        ArtifactDetect::EogParams params;
        params.dThresholdV = 150e-6;
        QVector<int> events = ArtifactDetect::detectEog(data, info, fs, params);

        // Must detect events from the large channel
        QVERIFY2(events.size() >= 1, "Should detect blinks from the high-amplitude EOG channel");
    }
};

QTEST_MAIN(TestDspArtifactDetect)
#include "test_dsp_artifact_detect.moc"
