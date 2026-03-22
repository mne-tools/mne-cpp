//=============================================================================================================
/**
 * @file     test_dsp_firfilter.cpp
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
 * @brief    Unit tests for FirFilter — the discoverable façade over FilterKernel.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/firfilter.h>
#include <dsp/filterkernel.h>

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

/**
 * @brief Compute the power of a signal in a frequency band [fLow, fHigh] Hz using a simple DFT.
 */
double bandPower(const RowVectorXd& sig, double sFreq, double fLow, double fHigh)
{
    const int N = static_cast<int>(sig.size());
    double power = 0.0;

    for (int k = 0; k < N / 2; ++k) {
        double freq = static_cast<double>(k) * sFreq / static_cast<double>(N);
        if (freq < fLow || freq > fHigh) continue;

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
};

//=============================================================================================================
// TEST IMPLEMENTATIONS
//=============================================================================================================

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
    MatrixXd out  = FirFilter::applyZeroPhaseMatrix(data, k);

    QCOMPARE(out.rows(), nCh);
    QCOMPARE(out.cols(), nSamp);
}

//=============================================================================================================

void TestDspFirFilter::lowPass_frequencySelectivity()
{
    // Signal: 5 Hz (pass) + 200 Hz (stop) at 1000 Hz sampling
    const double sFreq   = 1000.0;
    const int    nSamp   = 4096;
    const double cutoff  = 40.0;

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
    const double sFreq   = 1000.0;
    const int    nSamp   = 4096;
    const double cutoff  = 40.0;

    RowVectorXd sig = makeSine(5.0, sFreq, nSamp) + makeSine(200.0, sFreq, nSamp);

    FilterKernel k = FirFilter::design(256, FirFilter::HighPass, cutoff, 0.0, sFreq, 5.0, FirFilter::Cosine);
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
    const double sFreq  = 1000.0;
    const int    nSamp  = 4096;
    const double fLow   = 20.0;
    const double fHigh  = 80.0;

    RowVectorXd sig = makeSine(5.0, sFreq, nSamp)
                    + makeSine(40.0, sFreq, nSamp)
                    + makeSine(200.0, sFreq, nSamp);

    FilterKernel k = FirFilter::design(256, FirFilter::BandPass, fLow, fHigh, sFreq, 5.0, FirFilter::Cosine);
    RowVectorXd filtered = FirFilter::applyZeroPhase(sig, k);

    double passband  = bandPower(filtered, sFreq, fLow + 5.0, fHigh - 5.0);
    double stopLow   = bandPower(filtered, sFreq, 0.0,         fLow - 10.0);
    double stopHigh  = bandPower(filtered, sFreq, fHigh + 30.0, sFreq / 2.0);

    QVERIFY2(passband > stopLow  * 10.0,
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
    const int    nCh   = 3;
    const int    nSamp = 2048;

    FilterKernel k = FirFilter::design(256, FirFilter::LowPass, 40.0, 0.0, sFreq, 5.0, FirFilter::Cosine);

    // Each row is the same stop-band signal (200 Hz)
    MatrixXd data(nCh, nSamp);
    for (int ch = 0; ch < nCh; ++ch)
        data.row(ch) = makeSine(200.0, sFreq, nSamp);

    MatrixXd out = FirFilter::applyZeroPhaseMatrix(data, k);

    for (int ch = 0; ch < nCh; ++ch) {
        double powerOut = out.row(ch).squaredNorm() / nSamp;
        double powerIn  = data.row(ch).squaredNorm() / nSamp;
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

QTEST_MAIN(TestDspFirFilter)
#include "test_dsp_firfilter.moc"
