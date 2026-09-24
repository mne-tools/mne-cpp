//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_dsp_iirfilter.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.10
 * @date     March, 2026
 * @brief    Unit tests for IirFilter (Butterworth SOS design and application).
 */

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <cmath>

#include <dsp/iirfilter.h>

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// Helper: compute power of a signal in a frequency band via DFT
static double bandPower(const RowVectorXd& sig, double freqLow, double freqHigh, double fs)
{
    int N = static_cast<int>(sig.size());
    double power = 0.0;
    for (int k = 0; k < N / 2; ++k) {
        double freq = static_cast<double>(k) * fs / N;
        if (freq >= freqLow && freq <= freqHigh) {
            double re = 0.0, im = 0.0;
            for (int n = 0; n < N; ++n) {
                double angle = 2.0 * M_PI * k * n / N;
                re += sig(n) * std::cos(angle);
                im -= sig(n) * std::sin(angle);
            }
            power += (re * re + im * im) / (N * N);
        }
    }
    return power;
}

//=============================================================================================================

class TestDspIirFilter : public QObject
{
    Q_OBJECT

private slots:
    //=========================================================================
    // Design: section count
    //=========================================================================
    void lpDesign_sectionCount()
    {
        // Order 4 LP → 2 biquads
        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        QCOMPARE(sos.size(), 2);
    }

    void hpDesign_sectionCount()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::HighPass, 1.0, 0.0, 1000.0);
        QCOMPARE(sos.size(), 2);
    }

    void bpDesign_sectionCount()
    {
        // BP order 4 → 8 biquads (2 per LP prototype pole, 4 poles)
        auto sos = IirFilter::designButterworth(4, IirFilter::BandPass, 1.0, 40.0, 1000.0);
        QCOMPARE(sos.size(), 8);
    }

    void bsDesign_sectionCount()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::BandStop, 49.0, 51.0, 1000.0);
        QCOMPARE(sos.size(), 8);
    }

    void oddOrderDesign_sectionCount()
    {
        // Order 3 LP → 1 biquad + 1 first-order (stored as biquad with b2=a2=0) = 2 sections
        auto sos = IirFilter::designButterworth(3, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        QCOMPARE(sos.size(), 2);
    }

    //=========================================================================
    // Output length preservation
    //=========================================================================
    void applySos_lengthPreserved()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        RowVectorXd data = RowVectorXd::Random(512);
        RowVectorXd out  = IirFilter::applySos(data, sos);
        QCOMPARE(out.size(), data.size());
    }

    void applyZeroPhase_lengthPreserved()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        RowVectorXd data = RowVectorXd::Random(512);
        RowVectorXd out  = IirFilter::applyZeroPhase(data, sos);
        QCOMPARE(out.size(), data.size());
    }

    void applyZeroPhaseMatrix_dimensionsPreserved()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        MatrixXd data = MatrixXd::Random(6, 512);
        MatrixXd out  = IirFilter::applyZeroPhaseMatrix(data, sos);
        QCOMPARE(out.rows(), data.rows());
        QCOMPARE(out.cols(), data.cols());
    }

    //=========================================================================
    // LPF: pass low, attenuate high
    //=========================================================================
    void lpFilter_passbandAttenuatesStopband()
    {
        const double fs   = 1000.0;
        const double fc   = 40.0;    // cutoff
        const int    N    = 4096;
        const double fLow = 10.0;    // well inside passband
        const double fHi  = 200.0;   // well inside stopband

        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, fc, 0.0, fs);

        RowVectorXd sig(N);
        for (int i = 0; i < N; ++i) {
            sig(i) = std::sin(2 * M_PI * fLow * i / fs)
                   + std::sin(2 * M_PI * fHi  * i / fs);
        }

        RowVectorXd filtered = IirFilter::applyZeroPhase(sig, sos);

        double pLow = bandPower(filtered, fLow - 2.0, fLow + 2.0, fs);
        double pHi  = bandPower(filtered, fHi  - 5.0, fHi  + 5.0, fs);

        QVERIFY2(pLow > pHi * 100.0,
                 qPrintable(QString("LP: passband power %1 should dominate stopband %2").arg(pLow).arg(pHi)));
    }

    //=========================================================================
    // HPF: pass high, attenuate low
    //=========================================================================
    void hpFilter_passbandAttenuatesStopband()
    {
        const double fs   = 1000.0;
        const double fc   = 50.0;
        const int    N    = 4096;

        auto sos = IirFilter::designButterworth(4, IirFilter::HighPass, fc, 0.0, fs);

        RowVectorXd sig(N);
        for (int i = 0; i < N; ++i) {
            sig(i) = std::sin(2 * M_PI * 5.0   * i / fs)   // stopband (5 Hz)
                   + std::sin(2 * M_PI * 200.0  * i / fs);  // passband (200 Hz)
        }

        RowVectorXd filtered = IirFilter::applyZeroPhase(sig, sos);

        double pPass = bandPower(filtered, 195.0, 205.0, fs);
        double pStop = bandPower(filtered, 3.0,   7.0,   fs);

        QVERIFY2(pPass > pStop * 100.0,
                 qPrintable(QString("HP: passband %1 should dominate stopband %2").arg(pPass).arg(pStop)));
    }

    //=========================================================================
    // BPF: pass band, attenuate outside
    //=========================================================================
    void bpFilter_passbandAttenuatesOutside()
    {
        const double fs   = 1000.0;
        const int    N    = 4096;

        auto sos = IirFilter::designButterworth(4, IirFilter::BandPass, 1.0, 40.0, fs);

        RowVectorXd sig(N);
        for (int i = 0; i < N; ++i) {
            sig(i) = std::sin(2 * M_PI * 0.05  * i / fs)    // below passband (0.05 Hz)
                   + std::sin(2 * M_PI * 10.0   * i / fs)    // inside passband (10 Hz)
                   + std::sin(2 * M_PI * 200.0  * i / fs);   // above passband (200 Hz)
        }

        RowVectorXd filtered = IirFilter::applyZeroPhase(sig, sos);

        double pIn   = bandPower(filtered, 8.0,   12.0,  fs);
        double pHigh = bandPower(filtered, 195.0, 205.0, fs);

        QVERIFY2(pIn > pHigh * 10.0,
                 qPrintable(QString("BP: in-band %1 vs high %2").arg(pIn).arg(pHigh)));
    }

    //=========================================================================
    // Edge: empty / trivial inputs
    //=========================================================================
    void emptyInput_returnsEmpty()
    {
        auto sos = IirFilter::designButterworth(4, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        RowVectorXd empty;
        RowVectorXd out = IirFilter::applyZeroPhase(empty, sos);
        QCOMPARE(out.size(), 0);
    }

    void emptySos_returnsInput()
    {
        QVector<IirBiquad> empty;
        RowVectorXd data = RowVectorXd::Random(128);
        RowVectorXd out  = IirFilter::applyZeroPhase(data, empty);
        QCOMPARE((out - data).cwiseAbs().maxCoeff(), 0.0);
    }

    void invalidOrder_returnsEmptySos()
    {
        auto sos = IirFilter::designButterworth(0, IirFilter::LowPass, 40.0, 0.0, 1000.0);
        QVERIFY(sos.isEmpty());
    }
};

QTEST_MAIN(TestDspIirFilter)
#include "test_dsp_iirfilter.moc"
