//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_artifact_detect.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.10
 * @date     March, 2026
 * @brief    Unit tests for ArtifactDetect (ECG/EOG event detection).
 */

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <dsp/artifact_detect.h>
#include <dsp/epoch_extractor.h>
#include <dsp/peak_finder.h>
#include <fiff/fiff_raw_data.h>
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
        ch.unit = 112; // Tesla
        info.chs.append(ch);
    }

    if (hasEcg) {
        FiffChInfo ch;
        ch.kind = FIFFV_ECG_CH;
        ch.ch_name = "ECG001";
        info.chs.append(ch);
        ++idx;
    }

    if (hasEog) {
        FiffChInfo ch;
        ch.kind = FIFFV_EOG_CH;
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
                                    double noiseLevel = 0.05)
{
    RowVectorXd sig = RowVectorXd::Zero(nSamp);
    double rrSamples = fs * 60.0 / bpm;
    double sigma = fs * 0.02; // 20 ms wide Gaussian
    int firstPeak = static_cast<int>(rrSamples * 0.3);

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
    double sigma = fs * 0.05; // 50 ms blink
    int firstBlink = static_cast<int>(blinkPeriod * 0.5);

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
        const double fs = 1000.0;
        const double bpm = 70.0;
        const int nSamp = 10000; // 10 s
        const int expectedPeaks = static_cast<int>(bpm / 60.0 * (nSamp / fs));

        FiffInfo info = makeInfoWithEcgEog(0, /*hasEcg=*/true, /*hasEog=*/false);

        MatrixXd data(1, nSamp);
        data.row(0) = makeSyntheticEcg(nSamp, fs, bpm, 1.0, 0.02);

        QVector<int> peaks = ArtifactDetect::detectEcg(data, info, fs);

        // Allow ±2 peaks tolerance
        QVERIFY2(std::abs(peaks.size() - expectedPeaks) <= 2,
                 qPrintable(QString("Expected ~%1 peaks, got %2").arg(expectedPeaks).arg(peaks.size())));
    }

    void ecg_peakSpacingReasonable()
    {
        const double fs = 1000.0;
        const double bpm = 60.0; // 1 Hz → 1000 samples between peaks
        const int nSamp = 8000;

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
        const double fs = 1000.0;
        const int nSamp = 5000;
        const double bpm = 65.0;

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
        const double fs = 1000.0;
        const double blinkRate = 0.3; // 0.3 Hz → ~3 blinks in 10 s
        const int nSamp = 10000;
        const int expectedBlinks = static_cast<int>(blinkRate * (nSamp / fs));

        FiffInfo info = makeInfoWithEcgEog(0, /*hasEcg=*/false, /*hasEog=*/true);

        MatrixXd data(1, nSamp);
        data.row(0) = makeSyntheticEog(nSamp, fs, blinkRate, 200e-6, 1e-7);

        ArtifactDetect::EogParams params;
        params.dThreshold = 100e-6;
        QVector<int> events = ArtifactDetect::detectEog(data, info, fs, params);

        QVERIFY2(std::abs(events.size() - expectedBlinks) <= 1,
                 qPrintable(QString("Expected ~%1 blinks, got %2")
                                .arg(expectedBlinks)
                                .arg(events.size())));
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
        const double fs = 1000.0;
        const int nSamp = 5000;

        FiffInfo info;
        // Two EOG channels
        for (int i = 0; i < 2; ++i) {
            FiffChInfo ch;
            ch.kind = FIFFV_EOG_CH;
            ch.ch_name = QString("EOG%1").arg(i + 1);
            info.chs.append(ch);
        }
        info.nchan = 2;

        MatrixXd data(2, nSamp);
        // Channel 0: large blinks
        data.row(0) = makeSyntheticEog(nSamp, fs, 0.5, 300e-6, 1e-7);
        // Channel 1: tiny signal
        data.row(1) = RowVectorXd::Constant(nSamp, 1e-9);

        QVector<int> events = ArtifactDetect::detectEog(data, info, fs);

        // Must detect events from the large channel
        QVERIFY2(events.size() >= 1, "Should detect blinks from the high-amplitude EOG channel");
    }

    //=========================================================================
    // MNE-Python
    //=========================================================================
    /**
     * Reference values produced by mne.preprocessing find_ecg_events / find_eog_events (mne 1.11), ev[:, 0]:
     *   sf = 250.0; t = np.arange(3000) / sf
     *   beat = sum(np.exp(-((t - c) ** 2) / (2 * 0.012 ** 2)) for c in np.arange(0.31, 12, 0.83))
     *   ecg = 1e-3 * beat + 1e-4 * np.sin(2 pi 0.3 t) + 2e-5 * np.sin(2 pi 50 t)
     *   blink = sum(np.exp(-((t - c) ** 2) / (2 * 0.06 ** 2)) for c in (1.7, 5.2, 9.9))
     *   eog1 = 2e-4 * blink + 1e-5 * np.sin(2 pi 0.2 t); eog2 = -0.3e-4 * blink + 3e-6 * np.cos(2 pi 3 t)
     *   mag k = 1e-12 * beat * (1 + 0.1 k) + 1e-13 * np.sin(2 pi (0.5 + k) t), k = 0..2
     *   channels M1 M2 M3 (mag), ECG, EOG1, EOG2
     */
    void eventsMatchMnePython_data()
    {
        QTest::addColumn<QString>("call");
        QTest::addColumn<QVector<int>>("expected");
        const QVector<int> beats{78, 285, 492, 700, 907, 1115, 1323, 1530, 1738, 1945, 2152, 2360, 2568, 2775};
        QTest::newRow("find_ecg_events") << "ecg" << beats;
        QTest::newRow("find_ecg_events, no ECG channel") << "ecg-mag" << beats;
        QTest::newRow("find_ecg_events(qrs_threshold=0.6, tstart=1)") << "ecg-0.6-t1" << beats.mid(1);
        QTest::newRow("find_eog_events") << "eog" << QVector<int>{214, 425, 632, 1300, 1508, 2475, 2683};
        // mne.preprocessing.ica_find_ecg_events(raw, 3 * ecg / ecg.std()) and
        // ica_find_eog_events(raw, eog2 / eog2.std()): the same detectors run on a single
        // ICA source time course.
        QTest::newRow("ica_find_ecg_events") << "ica-ecg" << beats;
        QTest::newRow("ica_find_eog_events") << "ica-eog" << QVector<int>{209, 426, 625, 1299, 2291, 2473, 2960};
    }

    void eventsMatchMnePython()
    {
        QFETCH(QString, call);
        QFETCH(QVector<int>, expected);

        const double sf = 250.0;
        const int n = 3000;
        MatrixXd data(6, n);
        for (int i = 0; i < n; ++i) {
            const double t = i / sf;
            double beat = 0.0;
            for (int j = 0; 0.31 + 0.83 * j < 12.0; ++j) {
                const double c = 0.31 + 0.83 * j;
                beat += std::exp(-(t - c) * (t - c) / (2 * 0.012 * 0.012));
            }
            double blink = 0.0;
            for (double c : {1.7, 5.2, 9.9})
                blink += std::exp(-(t - c) * (t - c) / (2 * 0.06 * 0.06));
            for (int k = 0; k < 3; ++k)
                data(k, i) = 1e-12 * beat * (1 + 0.1 * k) + 1e-13 * std::sin(2 * M_PI * (0.5 + k) * t);
            data(3, i) = 1e-3 * beat + 1e-4 * std::sin(2 * M_PI * 0.3 * t) + 2e-5 * std::sin(2 * M_PI * 50 * t);
            data(4, i) = 2e-4 * blink + 1e-5 * std::sin(2 * M_PI * 0.2 * t);
            data(5, i) = -0.3e-4 * blink + 3e-6 * std::cos(2 * M_PI * 3 * t);
        }
        FiffInfo info;
        const QStringList names{"M1", "M2", "M3", "ECG", "EOG1", "EOG2"};
        const int kinds[] = {FIFFV_MEG_CH, FIFFV_MEG_CH, FIFFV_MEG_CH, FIFFV_ECG_CH, FIFFV_EOG_CH, FIFFV_EOG_CH};
        for (int k = 0; k < 6; ++k) {
            FiffChInfo ch;
            ch.kind = kinds[k];
            ch.unit = k < 3 ? FIFF_UNIT_T : FIFF_UNIT_V;
            ch.ch_name = names[k];
            info.chs.append(ch);
            info.ch_names.append(names[k]);
        }
        info.nchan = 6;

        QVector<int> got;
        if (call.startsWith("ica-")) {
            // One ICA source, unit-free, carried as the only ECG / EOG channel.
            const bool ecg = call == "ica-ecg";
            const RowVectorXd row = data.row(ecg ? 3 : 5);
            const double mean = row.mean();
            const double std = std::sqrt((row.array() - mean).square().mean());
            const MatrixXd source = (ecg ? 3.0 : 1.0) * row / std;
            FiffInfo srcInfo;
            FiffChInfo ch;
            ch.kind = ecg ? FIFFV_ECG_CH : FIFFV_EOG_CH;
            ch.ch_name = "ICA000";
            srcInfo.chs.append(ch);
            srcInfo.ch_names.append(ch.ch_name);
            srcInfo.nchan = 1;
            got = ecg ? ArtifactDetect::detectEcg(source, srcInfo, sf) : ArtifactDetect::detectEog(source, srcInfo, sf);
        } else if (call == "eog") {
            got = ArtifactDetect::detectEog(data, info, sf);
        } else {
            ArtifactDetect::EcgParams params;
            if (call == "ecg-mag")
                info.bads << "ECG";
            if (call == "ecg-0.6-t1") {
                params.dQrsThreshold = 0.6;
                params.dTStart = 1.0;
            }
            got = ArtifactDetect::detectEcg(data, info, sf, params);
        }
        QCOMPARE(got, expected);
    }

    /**
     * Reference values produced by mne.preprocessing.create_ecg_epochs(raw) and
     * create_eog_epochs(raw) (mne 1.11) on sample_audvis_trunc_raw.fif: events found with
     * the 8-16 Hz (ECG) / 1-10 Hz (EOG) band, unfiltered epochs -0.5 ... 0.5 s, no baseline,
     * proj=False, all channels. Values: events - first_samp, d[0,0,0], d[-1,315,10] and the
     * norm of the 306 MEG channels over all epochs.
     */
    void createEpochsMatchesMnePython_data()
    {
        QTest::addColumn<bool>("ecg");
        QTest::addColumn<QVector<int>>("events");
        QTest::addColumn<double>("first");
        QTest::addColumn<double>("last");
        QTest::addColumn<double>("norm");
        QTest::newRow("create_ecg_epochs")
            << true
            << QVector<int>{241, 565, 876, 1173, 1464, 1754, 2060, 2379, 2697, 2976,
                            3237, 3531, 3845, 4162, 4465, 4745, 5033, 5333, 5644}
            << -4.216080203896694e-15 << -3.9512457299874155e-09 << 5.1752236221310362e-12;
        QTest::newRow("create_eog_epochs")
            << false << QVector<int>{150, 693, 1101, 4341}
            << -7.8953024303960885e-16 << -4.1027486890738421e-09 << 2.3859462017389855e-12;
    }

    void createEpochsMatchesMnePython()
    {
        QFETCH(bool, ecg);
        QFETCH(QVector<int>, events);
        QFETCH(double, first);
        QFETCH(double, last);
        QFETCH(double, norm);

        QFile file(QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif");
        const FiffRawData raw(file);
        MatrixXd data;
        MatrixXd times;
        QVERIFY(raw.read_raw_segment(data, times));
        const double sf = raw.info.sfreq;

        QVector<int> found;
        if (ecg) {
            ArtifactDetect::EcgParams params;
            params.dFilterLow = 8.0;
            params.dFilterHigh = 16.0;
            found = ArtifactDetect::detectEcg(data, raw.info, sf, params);
        } else {
            found = ArtifactDetect::detectEog(data, raw.info, sf);
        }
        QCOMPARE(found, events);

        EpochExtractor::Params params;
        params.dTmin = -0.5;
        params.dTmax = 0.5;
        params.bApplyBaseline = false;
        const QVector<MNELIB::MNEEpochData> epochs = EpochExtractor::extract(data, found, sf, params);
        QCOMPARE(epochs.size(), events.size());
        QCOMPARE(epochs.first().epoch.cols(), 301);
        double sumSq = 0.0;
        for (const MNELIB::MNEEpochData& e : epochs) {
            sumSq += e.epoch.topRows(306).squaredNorm();
        }
        QVERIFY(qAbs(epochs.first().epoch(0, 0) / first - 1.0) < 1e-5);
        QVERIFY(qAbs(epochs.last().epoch(315, 10) / last - 1.0) < 1e-5);
        QVERIFY(qAbs(std::sqrt(sumSq) / norm - 1.0) < 1e-5);
    }

    /** Reference values produced by mne.preprocessing.peak_finder (mne 1.11) on x below, default and (1.5, -1). */
    void peakFinderMatchesMnePython()
    {
        const VectorXd x = (VectorXd(14) << 0, 1, 3, 2, 2, 5, 1, 0, 4, 4, 1, 0, 2.5, 0.0).finished();
        const QList<QPair<int, double>> maxima = peakFinderMne(x);
        QCOMPARE(maxima, (QList<QPair<int, double>>{{5, 5.0}, {8, 4.0}, {12, 2.5}}));
        const QList<QPair<int, double>> minima = peakFinderMne(x, 1.5, -1);
        QCOMPARE(minima, (QList<QPair<int, double>>{{0, 0.0}, {7, 0.0}, {11, 0.0}, {13, 0.0}}));
    }
};

QTEST_MAIN(TestDspArtifactDetect)
#include "test_dsp_artifact_detect.moc"
