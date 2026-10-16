//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_multitaper.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for DPSS, MultitaperPsd, and MultitaperTfr classes.
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
    void testDpssPeriodicMatchesScipy();

    // MultitaperPsd tests
    void testPsdMatchesMnePython();
    void testPsdSinusoidal();
    void testPsdOutputDimensions();
    void testPsdFrequencyRange();
    void testPsdMultiChannel();

    // MultitaperTfr tests
    void testTfrSinusoidal();
    void testTfrOutputDimensions();
    void testTfrArrayMatchesMnePython();

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

    // Concentration ratios from scipy.signal.windows.dpss(512, 4.0, 3, return_ratios=True)
    const double expected[] = {0.9999999997060645, 0.9999999723665234, 0.999998791537088};
    QCOMPARE(result.vecEigenvalues.size(), 3);
    for (int i = 0; i < result.vecEigenvalues.size(); ++i) {
        QVERIFY2(std::fabs(result.vecEigenvalues(i) - expected[i]) < 1e-10, qPrintable(QString::number(result.vecEigenvalues(i), 'g', 17)));
    }
}

//=============================================================================================================

void TestDspMultitaper::testDpssPeriodicMatchesScipy()
{
    // Reference values produced by scipy.signal.windows.dpss(64, 2.5, 4, sym=False, return_ratios=True), as
    // mne.time_frequency.dpss_windows uses them: ratios, then |w[0, 0]|, |w[0, 31]|, |w[1, 10]|, |w[3, 63]|
    const double ratios[] = {0.999997274616, 0.999846440863, 0.996265292814, 0.952416752379};
    const double values[] = {0.00121212706954, 0.216767222399, 0.101811363563, 0.119848232192};

    const DpssResult r = Dpss::compute(64, 2.5, 4, false);
    QCOMPARE(r.matTapers.cols(), 64);
    for (int k = 0; k < 4; ++k)
        QVERIFY2(std::abs(r.vecEigenvalues[k] - ratios[k]) < 1e-10, qPrintable(QString::number(r.vecEigenvalues[k], 'g', 15)));
    const double got[] = {r.matTapers(0, 0), r.matTapers(0, 31), r.matTapers(1, 10), r.matTapers(3, 63)};
    for (int i = 0; i < 4; ++i)
        QVERIFY2(std::abs(std::abs(got[i]) - values[i]) < 1e-10, qPrintable(QString::number(got[i], 'g', 15)));
}

//=============================================================================================================

void TestDspMultitaper::testTfrArrayMatchesMnePython()
{
    // Reference values produced by mne.time_frequency.tfr_array_multitaper (mne 1.11), samples 0, 60, 250, 499:
    //   t = np.arange(500) / 250.0
    //   x = np.sin(2 * np.pi * 10 * t) + 0.5 * np.sin(2 * np.pi * 23 * t + 0.3) + 0.2 * t
    //   X = np.vstack([x, 0.5 * x[::-1]])[None]
    //   p = tfr_array_multitaper(X, 250.0, [10.0, 23.0], n_cycles=7.0, output="power")[0]
    //   p2 = tfr_array_multitaper(X, 250.0, [12.0], n_cycles=4.0, time_bandwidth=2.0, output="power")[0]  (one taper)
    const double expected[][4] = {{25.99696018, 44.57379396, 55.89384535, 25.30231975},
                                  {2.526671131, 5.75941098, 5.659913868, 2.363407711},
                                  {6.299555182, 11.08129722, 13.96705792, 6.528146109},
                                  {0.5866075026, 1.42166134, 1.417204589, 0.6387035949}};
    const double oneTaper[] = {4.812554785, 11.73626228, 11.93555391, 5.32965808};
    const int samples[] = {0, 60, 250, 499};

    const double sfreq = 250.0;
    MatrixXd data(2, 500);
    for (int n = 0; n < 500; ++n) {
        const double t = n / sfreq;
        data(0, n) = std::sin(2.0 * M_PI * 10.0 * t) + 0.5 * std::sin(2.0 * M_PI * 23.0 * t + 0.3) + 0.2 * t;
    }
    data.row(1) = 0.5 * data.row(0).reverse();

    RowVectorXd freqs(2);
    freqs << 10.0, 23.0;
    const MultitaperTfrResult r = MultitaperTfr::computeArray(data, sfreq, freqs);
    RowVectorXd freq12(1);
    freq12 << 12.0;
    const MultitaperTfrResult r1 = MultitaperTfr::computeArray(data, sfreq, freq12, 4.0, 2.0);

    for (int s = 0; s < 4; ++s) {
        for (int row = 0; row < 4; ++row) {
            const double got = r.tfrData[row / 2](row % 2, samples[s]);
            QVERIFY2(std::abs(got - expected[row][s]) <= 1e-6 * expected[row][s],
                     qPrintable(QString("ch %1 freq %2 sample %3: %4 vs mne %5").arg(row / 2).arg(row % 2).arg(samples[s]).arg(got, 0, 'g', 10).arg(expected[row][s], 0, 'g', 10)));
        }
        const double got = r1.tfrData[0](0, samples[s]);
        QVERIFY2(std::abs(got - oneTaper[s]) <= 1e-6 * oneTaper[s],
                 qPrintable(QString("one taper sample %1: %2 vs mne %3").arg(samples[s]).arg(got, 0, 'g', 10).arg(oneTaper[s], 0, 'g', 10)));
    }
}

//=============================================================================================================

void TestDspMultitaper::testPsdMatchesMnePython()
{
    // Reference values produced by mne.time_frequency.psd_array_multitaper(X, 250.0, normalization="full") (mne 1.11):
    //   t = np.arange(256) / 250.0
    //   x0 = np.sin(2 * np.pi * 10 * t) + 0.3 * np.cos(2 * np.pi * 23 * t) + 0.5
    //   x1 = 0.5 * np.sin(2 * np.pi * 10 * t + 0.7) + 0.1 * t
    // Rows: frequency bin, then x0, x1
    const int bins[] = {0, 1, 10, 24, 128};
    const double expected[][2] = {{3.977295001e-05, 0.0001130743836},
                                  {8.109891216e-05, 0.0002244708389},
                                  {0.07014372543, 0.01770142371},
                                  {0.006175187056, 6.249414777e-06},
                                  {2.321390722e-07, 9.35205691e-08}};

    const double sfreq = 250.0;
    MatrixXd data(2, 256);
    for (int n = 0; n < data.cols(); ++n) {
        const double t = n / sfreq;
        data(0, n) = std::sin(2.0 * M_PI * 10.0 * t) + 0.3 * std::cos(2.0 * M_PI * 23.0 * t) + 0.5;
        data(1, n) = 0.5 * std::sin(2.0 * M_PI * 10.0 * t + 0.7) + 0.1 * t;
    }

    const MultitaperPsdResult r = MultitaperPsd::compute(data, sfreq);
    QCOMPARE(r.matPsd.cols(), 129);
    for (int i = 0; i < 5; ++i)
        for (int ch = 0; ch < 2; ++ch)
            QVERIFY2(std::abs(r.matPsd(ch, bins[i]) - expected[i][ch]) <= 1e-6 * expected[i][ch],
                     qPrintable(QString("bin %1 ch %2: %3 vs mne %4").arg(bins[i]).arg(ch).arg(r.matPsd(ch, bins[i]), 0, 'g', 10).arg(expected[i][ch], 0, 'g', 10)));
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
