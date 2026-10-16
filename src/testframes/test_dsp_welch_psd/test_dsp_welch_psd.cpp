//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_welch_psd.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
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

    // MNE-Python
    void testMatchesMnePython_data();
    void testMatchesMnePython();
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
    const int nfft = 256;
    const double sfreq = 512.0;
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, sfreq);
    QVERIFY(std::abs(freqs[0]) < 1e-10);                      // DC = 0 Hz
    QVERIFY(std::abs(freqs[nfft / 2] - sfreq / 2.0) < 1e-10); // Nyquist
    QVERIFY(std::abs(freqs[1] - sfreq / nfft) < 1e-10);       // bin width
}

//=============================================================================================================

void TestWelchPsd::testDCSine()
{
    // A constant offset → all power in DC bin (k=0) when the DC is kept
    const int nSamples = 4096;
    RowVectorXd sig = RowVectorXd::Constant(nSamples, 1.0);
    RowVectorXd psd = WelchPsd::computeVector(sig, 500.0, 256, 0.5, WelchPsd::Hann, false);

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
    const int nfft = 256;
    const double dFreq = 50.0;

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
    const int nfft = 512;
    RowVectorXd sig = makeSine(100.0, dSFreq, 16384);
    RowVectorXd psd = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);

    double peak = psd.maxCoeff();
    double mean = (psd.sum() - peak) / static_cast<double>(psd.cols() - 1);
    QVERIFY(peak / mean > 100.0); // peak at least 100× average sidelobe
}

//=============================================================================================================

void TestWelchPsd::testTwoSinePeaks()
{
    const double dSFreq = 1000.0;
    const int nfft = 512;
    const int nSamp = 32768;

    RowVectorXd sig(nSamp);
    for (int n = 0; n < nSamp; ++n)
        sig[n] = std::sin(2.0 * M_PI * 50.0 * n / dSFreq) + std::sin(2.0 * M_PI * 200.0 * n / dSFreq);

    RowVectorXd psd = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, dSFreq);

    // Find two largest peaks
    int peak1, peak2;
    psd.maxCoeff(&peak1);
    RowVectorXd psd2 = psd;
    psd2[peak1] = 0.0;
    psd2.maxCoeff(&peak2);

    QVERIFY(std::abs(freqs[peak1] - 50.0) <= dSFreq / nfft + 1.0 ||
            std::abs(freqs[peak2] - 50.0) <= dSFreq / nfft + 1.0);
    QVERIFY(std::abs(freqs[peak1] - 200.0) <= dSFreq / nfft + 1.0 ||
            std::abs(freqs[peak2] - 200.0) <= dSFreq / nfft + 1.0);
}

//=============================================================================================================

void TestWelchPsd::testParsevalsRelation()
{
    // For a Hann-windowed Welch PSD:
    // integral of one-sided PSD ≈ mean square of the signal (within ~10 %)
    const double dSFreq = 500.0;
    const int nfft = 256;
    RowVectorXd sig = makeSine(75.0, dSFreq, 16384);

    RowVectorXd psd = WelchPsd::computeVector(sig, dSFreq, nfft, 0.5);
    RowVectorXd freqs = WelchPsd::freqAxis(nfft, dSFreq);
    const double df = freqs[1];

    double integral = psd.sum() * df;
    double meanPow = sig.squaredNorm() / static_cast<double>(sig.cols());

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
    const int nCh = 5;
    const int nSamp = 4096;
    const int nfft = 256;
    MatrixXd mat = MatrixXd::Random(nCh, nSamp);
    auto r = WelchPsd::compute(mat, 500.0, nfft);
    QCOMPARE(r.matPsd.rows(), nCh);
    QCOMPARE(r.matPsd.cols(), nfft / 2 + 1);
    QCOMPARE(r.vecFreqs.cols(), nfft / 2 + 1);
}

//=============================================================================================================

void TestWelchPsd::testChannelPicks()
{
    const int nCh = 10;
    const int nSamp = 4096;
    const int nfft = 256;
    MatrixXd mat = MatrixXd::Random(nCh, nSamp);

    RowVectorXi picks(3);
    picks << 0, 4, 9;
    auto r = WelchPsd::compute(mat, 500.0, nfft, 0.5, WelchPsd::Hann, picks);
    QCOMPARE(r.matPsd.rows(), 3);
}

//=============================================================================================================

void TestWelchPsd::testMatchesMnePython_data()
{
    // Reference values produced by mne.time_frequency.psd_array_welch(x, 250.0, n_fft, n_overlap, window,
    // remove_dc) (mne 1.11) at bins 0, 1, 10, 24 and the last one, channels x0 then x1:
    //   t = np.arange(1000) / 250.0
    //   x0 = np.sin(2 pi 10 t) + 0.3 np.cos(2 pi 23 t) + 0.5 + 0.2 t, x1 = 0.5 np.sin(2 pi 40 t + 0.7) - 0.1 t
    QTest::addColumn<int>("nfft");
    QTest::addColumn<double>("overlap");
    QTest::addColumn<int>("window");
    QTest::addColumn<bool>("removeDc");
    QTest::addColumn<QList<double>>("expected");
    QTest::newRow("hann, 256, overlap 128") << 256 << 0.5 << int(WelchPsd::Hann) << true
                                            << QList<double>{0.00018283535121, 3.61734797521e-08, 0.000900780662577, 0.000204004574454,
                                                             0.316790270176, 3.70288157762e-10, 0.0236334812474, 7.59634972257e-12,
                                                             2.62350962604e-15, 4.99857893909e-17};
    QTest::newRow("hamming, odd 255, no overlap") << 255 << 0.0 << int(WelchPsd::Hamming) << true
                                                  << QList<double>{6.61039503348e-05, 1.54419093012e-06, 0.00103816438256, 0.000245680289208,
                                                                   0.350874383167, 1.72374808472e-07, 0.0209306129309, 2.61512677902e-07,
                                                                   1.05076548145e-07, 3.43270088045e-08};
    QTest::newRow("hann, DC kept") << 256 << 0.5 << int(WelchPsd::Hann) << false
                                   << QList<double>{0.523893900234, 0.0271418054475, 0.262773709564, 0.0137748918611,
                                                    0.316790270176, 3.70288157762e-10, 0.0236334812474, 7.59634972261e-12,
                                                    2.62350962604e-15, 4.99857893954e-17};
}

//=============================================================================================================

void TestWelchPsd::testMatchesMnePython()
{
    QFETCH(int, nfft);
    QFETCH(double, overlap);
    QFETCH(int, window);
    QFETCH(bool, removeDc);
    QFETCH(QList<double>, expected);

    MatrixXd x(2, 1000);
    for (int i = 0; i < 1000; ++i) {
        const double t = i / 250.0;
        x(0, i) = std::sin(2.0 * M_PI * 10.0 * t) + 0.3 * std::cos(2.0 * M_PI * 23.0 * t) + 0.5 + 0.2 * t;
        x(1, i) = 0.5 * std::sin(2.0 * M_PI * 40.0 * t + 0.7) - 0.1 * t;
    }
    const WelchPsdResult r = WelchPsd::compute(x, 250.0, nfft, overlap, static_cast<WelchPsd::WindowType>(window),
                                               RowVectorXi(), removeDc);
    QCOMPARE(r.matPsd.cols(), Eigen::Index(nfft / 2 + 1));
    const int bins[] = {0, 1, 10, 24, nfft / 2};
    for (int k = 0; k < 5; ++k) {
        for (int ch = 0; ch < 2; ++ch) {
            const double got = r.matPsd(ch, bins[k]);
            const double want = expected[2 * k + ch];
            QVERIFY2(std::abs(got - want) <= 1e-8 * r.matPsd.row(ch).maxCoeff() + 1e-9 * std::abs(want),
                     qPrintable(QString("bin %1 ch %2: %3 vs mne %4").arg(bins[k]).arg(ch).arg(got, 0, 'g', 12).arg(want, 0, 'g', 12)));
        }
    }
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestWelchPsd)
#include "test_dsp_welch_psd.moc"
