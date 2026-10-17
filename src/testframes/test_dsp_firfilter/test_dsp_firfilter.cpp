//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_firfilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Unit tests for FirFilter — the discoverable façade over FilterKernel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/firfilter.h>
#include <dsp/filterkernel.h>
#include <dsp/parksmcclellan.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#include <cmath>
#include <complex>

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

namespace
{

/**
 * @brief Compute the power of a signal in a frequency band [fLow, fHigh] Hz using a simple DFT.
 */
double bandPower(const RowVectorXd& sig, double sFreq, double fLow, double fHigh)
{
    const int N = static_cast<int>(sig.size());
    double power = 0.0;

    for (int k = 0; k < N / 2; ++k) {
        double freq = static_cast<double>(k) * sFreq / static_cast<double>(N);
        if (freq < fLow || freq > fHigh)
            continue;

        double re = 0.0, im = 0.0;
        for (int n = 0; n < N; ++n) {
            double angle = 2.0 * M_PI * k * n / static_cast<double>(N);
            re += sig(n) * std::cos(angle);
            im -= sig(n) * std::sin(angle);
        }
        power += (re * re + im * im) / static_cast<double>(N * N);
    }
    return power;
}

/**
 * @brief Generate a sine wave at the given frequency.
 */
RowVectorXd makeSine(double freqHz, double sFreq, int nSamples)
{
    RowVectorXd v(nSamples);
    for (int i = 0; i < nSamples; ++i)
        v(i) = std::sin(2.0 * M_PI * freqHz * i / sFreq);
    return v;
}

} // anonymous namespace

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestDspFirFilter : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================================================
    /** design() returns a FilterKernel with the correct order. */
    void filterOrder_isPreserved();

    //=========================================================================================================
    /** apply() returns a vector of the same length as the input. */
    void apply_preservesLength();

    //=========================================================================================================
    /** applyZeroPhase() returns a vector of the same length as the input. */
    void applyZeroPhase_preservesLength();

    //=========================================================================================================
    /** applyZeroPhaseMatrix() returns a matrix with the same dimensions as the input. */
    void applyZeroPhaseMatrix_preservesDimensions();

    //=========================================================================================================
    /** Low-pass filter passes frequencies below cutoff and attenuates above. */
    void lowPass_frequencySelectivity();

    //=========================================================================================================
    /** High-pass filter passes frequencies above cutoff and attenuates below. */
    void highPass_frequencySelectivity();

    //=========================================================================================================
    /** Band-pass filter passes the band and attenuates outside it. */
    void bandPass_frequencySelectivity();

    //=========================================================================================================
    /** applyZeroPhaseMatrix() with vecPicks only filters the specified rows. */
    void applyZeroPhaseMatrix_picks_onlyFiltersSelectedRows();

    //=========================================================================================================
    /** applyZeroPhaseMatrix() with empty picks filters all rows. */
    void applyZeroPhaseMatrix_emptyPicks_filtersAllRows();

    //=========================================================================================================
    /** ParksMcClellan design method produces a valid kernel. */
    void design_parksMcClellan_producesValidKernel();

    /** Each Parks-McClellan pass type meets its bands; tap counts outside 9..256 are clamped. */
    void parksMcClellan_passTypes_data();
    void parksMcClellan_passTypes();

    /** firwin2 and filterDataZeroDouble equal scipy.signal.firwin2 and mne.filter.filter_data. */
    void firwin2_matchesScipy();
    void filterDataZeroDouble_matchesMnePython_data();
    void filterDataZeroDouble_matchesMnePython();
};

//=============================================================================================================
// TEST IMPLEMENTATIONS
//=============================================================================================================

void TestDspFirFilter::firwin2_matchesScipy()
{
    // Reference values produced by scipy.signal.firwin2(16, [0, 0.2, 0.3, 1.0], [1, 1, 0, 0], window="hann")[[0, 3, 7, 8, 15]]
    const double expected[] = {0.0, -0.00873571094037, 0.240768387452, 0.240768387452, 0.0};
    const int taps[] = {0, 3, 7, 8, 15};
    const RowVectorXd h = FirFilter::firwin2(16, {0.0, 0.2, 0.3, 1.0}, {1.0, 1.0, 0.0, 0.0});
    QCOMPARE(h.size(), 16);
    for (int i = 0; i < 5; ++i)
        QVERIFY2(std::abs(h(taps[i]) - expected[i]) < 1e-12, qPrintable(QString::number(h(taps[i]), 'g', 12)));
}

//=============================================================================================================

void TestDspFirFilter::filterDataZeroDouble_matchesMnePython_data()
{
    // Reference values produced by mne.filter.filter_data(x, 100.0, 5, 20, None, "10s", 0.5, 0.5,
    // phase="zero-double", fir_window="hann" | "hamming", fir_design="firwin2") (mne 1.11), with t = np.arange(n) / 100
    // and x = np.sin(2 pi 8 t) + 0.5 np.sin(2 pi 30 t) + 0.3 t + 0.2 np.cos(2 pi 1.5 t):
    // sum |y|, then y[3], y[113], y[n // 2 + 7], y[n - 2]. The 1024-tap filter is shorter / longer than the data.
    QTest::addColumn<int>("n");
    QTest::addColumn<bool>("hamming");
    QTest::addColumn<QList<double>>("expected");
    QTest::newRow("1200 samples") << 1200 << false << QList<double>{760.416254023, 1.00313539474, 0.24845613157, -0.368126558459, -0.119258490373};
    QTest::newRow("700 samples") << 700 << false << QList<double>{442.540025113, 1.00313543814, 0.248460245928, -0.36768758107, -0.12477357814};
    QTest::newRow("1200 samples, hamming") << 1200 << true << QList<double>{760.43763656, 1.00317665173, 0.248464568063, -0.368144974104, -0.11921357396};
}

//=============================================================================================================

void TestDspFirFilter::filterDataZeroDouble_matchesMnePython()
{
    QFETCH(int, n);
    QFETCH(bool, hamming);
    QFETCH(QList<double>, expected);

    MatrixXd x(1, n);
    for (int i = 0; i < n; ++i) {
        const double t = i / 100.0;
        x(0, i) = std::sin(2.0 * M_PI * 8.0 * t) + 0.5 * std::sin(2.0 * M_PI * 30.0 * t) + 0.3 * t + 0.2 * std::cos(2.0 * M_PI * 1.5 * t);
    }
    const MatrixXd y = FirFilter::filterDataZeroDouble(x, 100.0, 5.0, 20.0, hamming ? FirFilter::FirWindow::Hamming : FirFilter::FirWindow::Hann);
    const double got[] = {y.cwiseAbs().sum(), y(0, 3), y(0, 113), y(0, n / 2 + 7), y(0, n - 2)};
    for (int i = 0; i < 5; ++i)
        QVERIFY2(std::abs(got[i] - expected[i]) <= 1e-8 * std::max(1.0, std::abs(expected[i])),
                 qPrintable(QString("value %1: %2 vs mne %3").arg(i).arg(got[i], 0, 'g', 12).arg(expected[i], 0, 'g', 12)));
}

void TestDspFirFilter::filterOrder_isPreserved()
{
    const int order = 100;
    FilterKernel k = FirFilter::design(order, FirFilter::LowPass, 20.0, 0.0, 1000.0);
    QCOMPARE(k.getFilterOrder(), order);
}

//=============================================================================================================

void TestDspFirFilter::apply_preservesLength()
{
    const int N = 512;
    FilterKernel k = FirFilter::design(64, FirFilter::LowPass, 50.0, 0.0, 1000.0);

    RowVectorXd input = RowVectorXd::Random(N);
    RowVectorXd output = FirFilter::apply(input, k);

    QCOMPARE(output.size(), N);
}

//=============================================================================================================

void TestDspFirFilter::applyZeroPhase_preservesLength()
{
    const int N = 1024;
    FilterKernel k = FirFilter::design(64, FirFilter::HighPass, 10.0, 0.0, 500.0);

    RowVectorXd input = RowVectorXd::Random(N);
    RowVectorXd output = FirFilter::applyZeroPhase(input, k);

    QCOMPARE(output.size(), N);
}

//=============================================================================================================

void TestDspFirFilter::applyZeroPhaseMatrix_preservesDimensions()
{
    const int nCh = 8, nSamp = 512;
    FilterKernel k = FirFilter::design(64, FirFilter::BandPass, 1.0, 40.0, 500.0);

    MatrixXd data = MatrixXd::Random(nCh, nSamp);
    MatrixXd out = FirFilter::applyZeroPhaseMatrix(data, k);

    QCOMPARE(out.rows(), nCh);
    QCOMPARE(out.cols(), nSamp);
}

//=============================================================================================================

void TestDspFirFilter::lowPass_frequencySelectivity()
{
    // Signal: 5 Hz (pass) + 200 Hz (stop) at 1000 Hz sampling
    const double sFreq = 1000.0;
    const int nSamp = 4096;
    const double cutoff = 40.0;

    RowVectorXd sig = makeSine(5.0, sFreq, nSamp) + makeSine(200.0, sFreq, nSamp);

    FilterKernel k = FirFilter::design(256, FirFilter::LowPass, cutoff, 0.0, sFreq, 5.0, FirFilter::Cosine);
    RowVectorXd filtered = FirFilter::applyZeroPhase(sig, k);

    double passband = bandPower(filtered, sFreq, 0.0, cutoff - 5.0);
    double stopband = bandPower(filtered, sFreq, cutoff + 30.0, sFreq / 2.0);

    QVERIFY2(passband > stopband * 10.0,
             qPrintable(QString("LP: passband %1 should >> stopband %2").arg(passband).arg(stopband)));
}

//=============================================================================================================

void TestDspFirFilter::highPass_frequencySelectivity()
{
    // Signal: 5 Hz (stop) + 200 Hz (pass) at 1000 Hz sampling
    const double sFreq = 1000.0;
    const int nSamp = 4096;
    const double cutoff = 40.0;

    RowVectorXd sig = makeSine(5.0, sFreq, nSamp) + makeSine(200.0, sFreq, nSamp);

    FilterKernel k = FirFilter::design(256, FirFilter::HighPass, cutoff, 0.0, sFreq, 5.0, FirFilter::ParksMcClellan);
    RowVectorXd filtered = FirFilter::applyZeroPhase(sig, k);

    double passband = bandPower(filtered, sFreq, cutoff + 30.0, sFreq / 2.0);
    double stopband = bandPower(filtered, sFreq, 0.0, cutoff - 10.0);

    QVERIFY2(passband > stopband * 10.0,
             qPrintable(QString("HP: passband %1 should >> stopband %2").arg(passband).arg(stopband)));
}

//=============================================================================================================

void TestDspFirFilter::bandPass_frequencySelectivity()
{
    // Signal: 5 Hz (stop) + 40 Hz (pass) + 200 Hz (stop) at 1000 Hz sampling
    const double sFreq = 1000.0;
    const int nSamp = 4096;
    const double fLow = 20.0;
    const double fHigh = 80.0;

    RowVectorXd sig = makeSine(5.0, sFreq, nSamp) + makeSine(40.0, sFreq, nSamp) + makeSine(200.0, sFreq, nSamp);

    FilterKernel k = FirFilter::design(256, FirFilter::BandPass, fLow, fHigh, sFreq, 5.0, FirFilter::ParksMcClellan);
    RowVectorXd filtered = FirFilter::applyZeroPhase(sig, k);

    double passband = bandPower(filtered, sFreq, fLow + 5.0, fHigh - 5.0);
    double stopLow = bandPower(filtered, sFreq, 0.0, fLow - 10.0);
    double stopHigh = bandPower(filtered, sFreq, fHigh + 30.0, sFreq / 2.0);

    QVERIFY2(passband > stopLow * 10.0,
             qPrintable(QString("BP lower stop: passband %1 vs stopLow %2").arg(passband).arg(stopLow)));
    QVERIFY2(passband > stopHigh * 10.0,
             qPrintable(QString("BP upper stop: passband %1 vs stopHigh %2").arg(passband).arg(stopHigh)));
}

//=============================================================================================================

void TestDspFirFilter::applyZeroPhaseMatrix_picks_onlyFiltersSelectedRows()
{
    const int nCh = 4, nSamp = 512;
    FilterKernel k = FirFilter::design(64, FirFilter::LowPass, 50.0, 0.0, 1000.0);

    // Use a constant signal so filtered == original (all-pass by content)
    MatrixXd data = MatrixXd::Ones(nCh, nSamp);
    MatrixXd orig = data;

    // Only filter row 1
    RowVectorXi picks(1);
    picks(0) = 1;
    MatrixXd out = FirFilter::applyZeroPhaseMatrix(data, k, picks);

    // Rows 0, 2, 3 must be unchanged
    for (int ch : {0, 2, 3}) {
        QVERIFY((out.row(ch) - orig.row(ch)).norm() < 1e-10);
    }
    // Shape preserved
    QCOMPARE(out.rows(), nCh);
    QCOMPARE(out.cols(), nSamp);
}

//=============================================================================================================

void TestDspFirFilter::applyZeroPhaseMatrix_emptyPicks_filtersAllRows()
{
    // A pure-sinusoid input at the pass-band centre should survive; a stop-band sinusoid
    // should be attenuated in ALL rows when no picks are specified.
    const double sFreq = 1000.0;
    const int nCh = 3;
    const int nSamp = 2048;

    FilterKernel k = FirFilter::design(256, FirFilter::LowPass, 40.0, 0.0, sFreq, 5.0, FirFilter::Cosine);

    // Each row is the same stop-band signal (200 Hz)
    MatrixXd data(nCh, nSamp);
    for (int ch = 0; ch < nCh; ++ch)
        data.row(ch) = makeSine(200.0, sFreq, nSamp);

    MatrixXd out = FirFilter::applyZeroPhaseMatrix(data, k);

    for (int ch = 0; ch < nCh; ++ch) {
        double powerOut = out.row(ch).squaredNorm() / nSamp;
        double powerIn = data.row(ch).squaredNorm() / nSamp;
        // Stop-band signal should be strongly attenuated
        QVERIFY2(powerOut < powerIn * 0.01,
                 qPrintable(QString("Row %1 not filtered: powerOut=%2 powerIn=%3").arg(ch).arg(powerOut).arg(powerIn)));
    }
}

//=============================================================================================================

void TestDspFirFilter::design_parksMcClellan_producesValidKernel()
{
    FilterKernel k = FirFilter::design(128, FirFilter::BandPass,
                                       1.0, 40.0, 600.0,
                                       3.0, FirFilter::ParksMcClellan);
    QCOMPARE(k.getFilterOrder(), 128);
    // Coefficient vector must be non-zero
    QVERIFY(k.getCoefficients().norm() > 1e-12);
}

//=============================================================================================================

void TestDspFirFilter::parksMcClellan_passTypes_data()
{
    // Frequencies normalized to Nyquist; pass and stop are checked at band centres away from the transitions
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("taps");
    QTest::addColumn<double>("omega");
    QTest::addColumn<double>("bw");
    QTest::addColumn<QList<double>>("pass");
    QTest::addColumn<QList<double>>("stop");
    QTest::newRow("lowpass") << int(ParksMcClellan::LPF) << 101 << 0.3 << 0.0 << QList<double>{0.0, 0.15} << QList<double>{0.6, 0.9};
    QTest::newRow("highpass, even taps") << int(ParksMcClellan::HPF) << 100 << 0.5 << 0.0 << QList<double>{0.75, 0.95} << QList<double>{0.0, 0.2};
    QTest::newRow("bandpass") << int(ParksMcClellan::BPF) << 121 << 0.5 << 0.2 << QList<double>{0.5} << QList<double>{0.05, 0.95};
    QTest::newRow("notch, even taps") << int(ParksMcClellan::NOTCH) << 120 << 0.5 << 0.2 << QList<double>{0.05, 0.95} << QList<double>{0.5};
}

void TestDspFirFilter::parksMcClellan_passTypes()
{
    QFETCH(int, type);
    QFETCH(int, taps);
    QFETCH(double, omega);
    QFETCH(double, bw);
    QFETCH(QList<double>, pass);
    QFETCH(QList<double>, stop);
    const auto passType = static_cast<ParksMcClellan::TPassType>(type);
    ParksMcClellan pm(taps, omega, bw, 0.1, passType);
    // High-pass and notch filters need a symmetric odd-length kernel
    const int expectedTaps = (passType == ParksMcClellan::HPF || passType == ParksMcClellan::NOTCH) && taps % 2 == 0 ? taps - 1 : taps;
    QCOMPARE(pm.FirCoeff.size(), Eigen::Index(expectedTaps));
    const auto gain = [](const Eigen::RowVectorXd& h, double w) {
        std::complex<double> sum = 0.0;
        for (Eigen::Index n = 0; n < h.size(); ++n)
            sum += h(n) * std::polar(1.0, -M_PI * w * static_cast<double>(n));
        return std::abs(sum);
    };
    for (double w : pass)
        QVERIFY2(std::abs(gain(pm.FirCoeff, w) - 1.0) < 0.05, qPrintable(QString("pass %1: %2").arg(w).arg(gain(pm.FirCoeff, w))));
    for (double w : stop)
        QVERIFY2(gain(pm.FirCoeff, w) < 0.05, qPrintable(QString("stop %1: %2").arg(w).arg(gain(pm.FirCoeff, w))));

    // Too few taps are raised to 9, too many lowered to 256, and init() on a default object designs the same kernel
    const Eigen::RowVectorXd nine = ParksMcClellan(9, omega, bw, 0.1, passType).FirCoeff;
    QCOMPARE(ParksMcClellan(4, omega, bw, 0.1, passType).FirCoeff, nine);
    const Eigen::RowVectorXd many = ParksMcClellan(256, omega, bw, 0.1, passType).FirCoeff;
    QCOMPARE(ParksMcClellan(300, omega, bw, 0.1, passType).FirCoeff, many);
    ParksMcClellan later;
    later.init(taps, omega, bw, 0.1, passType);
    QCOMPARE(later.FirCoeff, pm.FirCoeff);
}

//=============================================================================================================

QTEST_MAIN(TestDspFirFilter)
#include "test_dsp_firfilter.moc"
