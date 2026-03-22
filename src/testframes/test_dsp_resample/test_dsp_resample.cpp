//=============================================================================================================
/**
 * @file     test_dsp_resample.cpp
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
 * @brief    Unit tests for Resample (polyphase sinc resampler).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/resample.h>

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

RowVectorXd makeSine(double freqHz, double sFreq, int nSamples)
{
    RowVectorXd v(nSamples);
    for (int i = 0; i < nSamples; ++i)
        v(i) = std::sin(2.0 * M_PI * freqHz * i / sFreq);
    return v;
}

/** DFT magnitude at a single frequency bin (Hz). */
double dftMagnitude(const RowVectorXd& sig, double sFreq, double freqHz)
{
    const int N = sig.size();
    double re = 0.0, im = 0.0;
    for (int n = 0; n < N; ++n) {
        double angle = 2.0 * M_PI * freqHz * n / sFreq;
        re += sig(n) * std::cos(angle);
        im -= sig(n) * std::sin(angle);
    }
    return std::sqrt(re * re + im * im) / N;
}

} // anonymous namespace

//=============================================================================================================
// TEST CLASS
//=============================================================================================================

class TestDspResample : public QObject
{
    Q_OBJECT

private slots:
    /** Output length = ceil(nIn * newRate / oldRate). */
    void outputLength_decimation();

    /** Output length for upsampling. */
    void outputLength_interpolation();

    /** Same-rate resampling returns the input unchanged. */
    void sameRate_returnsIdentity();

    /** A 4:1 decimation of a DC signal preserves the DC value. */
    void decimation_DCpreserved();

    /** A low-frequency sine survives 4:1 decimation (passband signal). */
    void decimation_passband_preserved();

    /** A high-frequency sine is suppressed by 4:1 decimation (anti-aliasing). */
    void decimation_stopband_attenuated();

    /** A 2:1 upsampling of a sine preserves the sinusoid frequency. */
    void interpolation_sinusoidPreserved();

    /** Rational ratio (600→250 Hz, i.e. 5/12) produces correct length and preserves low-freq content. */
    void rationalRatio_correctLengthAndContent();

    /** Empty input returns empty output. */
    void emptyInput_returnsEmpty();

    /** resampleMatrix() produces correct dimensions. */
    void resampleMatrix_dimensions();

    /** resampleMatrix() with picks only resamples selected rows. */
    void resampleMatrix_picks();
};

//=============================================================================================================

void TestDspResample::outputLength_decimation()
{
    // 4:1 decimation: 1000 → 250 Hz, 4096 samples → ceil(4096 * 250/1000) = 1024
    const int nIn   = 4096;
    const int nExpected = 1024;
    RowVectorXd sig = RowVectorXd::Random(nIn);
    RowVectorXd out = Resample::resample(sig, 250.0, 1000.0);
    QCOMPARE(out.size(), nExpected);
}

//=============================================================================================================

void TestDspResample::outputLength_interpolation()
{
    // 4:1 upsampling: 250 → 1000 Hz, 256 samples → ceil(256 * 4) = 1024
    const int nIn   = 256;
    const int nExpected = 1024;
    RowVectorXd sig = RowVectorXd::Random(nIn);
    RowVectorXd out = Resample::resample(sig, 1000.0, 250.0);
    QCOMPARE(out.size(), nExpected);
}

//=============================================================================================================

void TestDspResample::sameRate_returnsIdentity()
{
    RowVectorXd sig = RowVectorXd::Random(512);
    RowVectorXd out = Resample::resample(sig, 1000.0, 1000.0);
    QCOMPARE(out.size(), sig.size());
    QVERIFY((out - sig).norm() < 1e-10);
}

//=============================================================================================================

void TestDspResample::decimation_DCpreserved()
{
    const int nIn = 4096;
    const double dcVal = 3.14;
    RowVectorXd sig = RowVectorXd::Constant(nIn, dcVal);
    RowVectorXd out = Resample::resample(sig, 250.0, 1000.0);

    // Interior samples should be very close to dcVal (edges may have transients)
    const int nOut = out.size();
    const int edge = 10;
    if (nOut > 2 * edge) {
        RowVectorXd interior = out.segment(edge, nOut - 2 * edge);
        QVERIFY2((interior.array() - dcVal).abs().maxCoeff() < 0.01,
                 "DC value not preserved after decimation");
    }
}

//=============================================================================================================

void TestDspResample::decimation_passband_preserved()
{
    // 10 Hz sine at 1000 Hz → decimate to 250 Hz
    // After decimation Nyquist is 125 Hz; 10 Hz is well within passband
    const double sFreqOld = 1000.0;
    const double sFreqNew = 250.0;
    const int nIn         = 8192;
    const double freqHz   = 10.0;

    RowVectorXd sig = makeSine(freqHz, sFreqOld, nIn);
    RowVectorXd out = Resample::resample(sig, sFreqNew, sFreqOld);

    // Magnitude at 10 Hz in decimated signal should be close to 0.5 (peak of sine / N ≈ 0.5)
    double mag = dftMagnitude(out, sFreqNew, freqHz);
    QVERIFY2(mag > 0.3, qPrintable(QString("Passband 10 Hz attenuated too much: mag=%1").arg(mag)));
}

//=============================================================================================================

void TestDspResample::decimation_stopband_attenuated()
{
    // 200 Hz sine at 1000 Hz → decimate to 250 Hz
    // 200 Hz is above new Nyquist (125 Hz) → should be strongly attenuated
    const double sFreqOld = 1000.0;
    const double sFreqNew = 250.0;
    const int nIn         = 8192;
    const double freqHz   = 200.0;

    RowVectorXd sig = makeSine(freqHz, sFreqOld, nIn);
    RowVectorXd out = Resample::resample(sig, sFreqNew, sFreqOld);

    // The 200 Hz component should alias somewhere, but the anti-aliasing filter should suppress it.
    // Check RMS of output is much less than input RMS (input RMS ≈ 0.707)
    double rmsOut = std::sqrt(out.squaredNorm() / out.size());
    double rmsIn  = std::sqrt(sig.squaredNorm() / sig.size());
    QVERIFY2(rmsOut < rmsIn * 0.05,
             qPrintable(QString("Stopband not suppressed: rmsOut=%1, rmsIn=%2").arg(rmsOut).arg(rmsIn)));
}

//=============================================================================================================

void TestDspResample::interpolation_sinusoidPreserved()
{
    // 5 Hz sine at 250 Hz, upsample to 1000 Hz
    const double sFreqOld = 250.0;
    const double sFreqNew = 1000.0;
    const int nIn         = 1024;
    const double freqHz   = 5.0;

    RowVectorXd sig = makeSine(freqHz, sFreqOld, nIn);
    RowVectorXd out = Resample::resample(sig, sFreqNew, sFreqOld);

    double mag = dftMagnitude(out, sFreqNew, freqHz);
    QVERIFY2(mag > 0.3, qPrintable(QString("5 Hz not preserved after upsampling: mag=%1").arg(mag)));
}

//=============================================================================================================

void TestDspResample::rationalRatio_correctLengthAndContent()
{
    // 600 Hz → 250 Hz: ratio = 5/12 after GCD(250*1000, 600*1000) reduction
    const double sFreqOld = 600.0;
    const double sFreqNew = 250.0;
    const int nIn         = 6000;  // 10 s at 600 Hz
    // Expected: ceil(6000 * 250 / 600) = ceil(2500) = 2500
    const int nExpected = 2500;

    RowVectorXd sig = makeSine(20.0, sFreqOld, nIn);
    RowVectorXd out = Resample::resample(sig, sFreqNew, sFreqOld);

    QCOMPARE(out.size(), nExpected);

    // 20 Hz is below new Nyquist (125 Hz) → should survive
    double mag = dftMagnitude(out, sFreqNew, 20.0);
    QVERIFY2(mag > 0.3, qPrintable(QString("20 Hz attenuated at 250 Hz: mag=%1").arg(mag)));
}

//=============================================================================================================

void TestDspResample::emptyInput_returnsEmpty()
{
    RowVectorXd empty;
    RowVectorXd out = Resample::resample(empty, 250.0, 1000.0);
    QCOMPARE(out.size(), 0);
}

//=============================================================================================================

void TestDspResample::resampleMatrix_dimensions()
{
    const int nCh = 6, nIn = 4096;
    MatrixXd data = MatrixXd::Random(nCh, nIn);
    MatrixXd out  = Resample::resampleMatrix(data, 250.0, 1000.0);

    QCOMPARE(out.rows(), nCh);
    QCOMPARE(out.cols(), 1024);  // ceil(4096 * 250/1000)
}

//=============================================================================================================

void TestDspResample::resampleMatrix_picks()
{
    const int nCh = 4, nIn = 2048;
    // All channels are a 5 Hz sine
    MatrixXd data(nCh, nIn);
    for (int ch = 0; ch < nCh; ++ch)
        data.row(ch) = makeSine(5.0, 1000.0, nIn);

    RowVectorXi picks(2);
    picks << 0, 2;  // Only rows 0 and 2

    MatrixXd out = Resample::resampleMatrix(data, 250.0, 1000.0, picks);

    // Picked rows should have signal content
    QVERIFY(out.row(0).norm() > 1.0);
    QVERIFY(out.row(2).norm() > 1.0);
    // Non-picked rows should be zero (as documented)
    QVERIFY(out.row(1).norm() < 1e-10);
    QVERIFY(out.row(3).norm() < 1e-10);
}

//=============================================================================================================

QTEST_MAIN(TestDspResample)
#include "test_dsp_resample.moc"
