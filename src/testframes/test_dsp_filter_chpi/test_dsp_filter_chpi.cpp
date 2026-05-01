//=============================================================================================================
/**
 * @file     test_dsp_filter_chpi.cpp
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
 * @brief    Tests for filterChpi — cHPI signal removal by notch filtering.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/filter_chpi.h>

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
 * @brief Build a minimal FiffInfo with the given number of MEG and EEG channels.
 */
FiffInfo makeFiffInfo(int nMeg, int nEeg)
{
    FiffInfo info;
    for (int i = 0; i < nMeg; ++i) {
        FiffChInfo ch;
        ch.kind = FIFFV_MEG_CH;
        ch.ch_name = QString("MEG%1").arg(i + 1, 4, 10, QChar('0'));
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    for (int i = 0; i < nEeg; ++i) {
        FiffChInfo ch;
        ch.kind = FIFFV_EEG_CH;
        ch.ch_name = QString("EEG%1").arg(i + 1, 4, 10, QChar('0'));
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    info.nchan = info.chs.size();
    return info;
}

/**
 * @brief Compute power at a specific frequency for a single row using the Goertzel-like DFT bin.
 *
 * Returns sum of (data * cos(2*pi*f*t))^2 + (data * sin(2*pi*f*t))^2 normalised by N^2.
 */
double powerAtFreq(const RowVectorXd& row, double freq, double sfreq)
{
    const int N = static_cast<int>(row.size());
    double cosSum = 0.0;
    double sinSum = 0.0;
    for (int n = 0; n < N; ++n) {
        const double t = static_cast<double>(n) / sfreq;
        cosSum += row(n) * std::cos(2.0 * M_PI * freq * t);
        sinSum += row(n) * std::sin(2.0 * M_PI * freq * t);
    }
    return (cosSum * cosSum + sinSum * sinSum) / (static_cast<double>(N) * static_cast<double>(N));
}

/**
 * @brief Generate a pure sine tone for a given number of samples and frequency.
 */
RowVectorXd sineTone(double freq, double sfreq, int nSamples, double amplitude = 1.0)
{
    RowVectorXd row(nSamples);
    for (int n = 0; n < nSamples; ++n) {
        const double t = static_cast<double>(n) / sfreq;
        row(n) = amplitude * std::sin(2.0 * M_PI * freq * t);
    }
    return row;
}

} // anonymous namespace

//=============================================================================================================
/**
 * @brief Tests for filterChpi function.
 */
class TestDspFilterChpi : public QObject
{
    Q_OBJECT

private slots:
    void testNotchRemovesFrequency();
    void testMultipleFrequencies();
    void testLowFrequencyPreserved();
    void testMegOnlyFiltered();
    void testAllChannelsFiltered();
    void testEmptyFrequencyList();
    void testInvalidFrequency();
    void testAutoDetectFallback();
    void testNotchWidth();
};

//=============================================================================================================

void TestDspFilterChpi::testNotchRemovesFrequency()
{
    // Inject a 143 Hz tone on 3 MEG channels, verify it's attenuated by >40 dB
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(3, 1);

    MatrixXd data(4, nSamples);
    for (int ch = 0; ch < 3; ++ch) {
        data.row(ch) = sineTone(143.0, sfreq, nSamples);
    }
    data.row(3) = sineTone(143.0, sfreq, nSamples); // EEG channel

    const double powerBefore = powerAtFreq(data.row(0), 143.0, sfreq);

    QVector<double> hpiFreqs = {143.0};
    filterChpi(data, info, sfreq, hpiFreqs);

    const double powerAfter = powerAtFreq(data.row(0), 143.0, sfreq);

    // Attenuation should be > 40 dB
    const double attenuationDb = 10.0 * std::log10(powerBefore / powerAfter);
    QVERIFY2(attenuationDb > 40.0,
             qPrintable(QString("Expected >40 dB attenuation, got %1 dB").arg(attenuationDb)));
}

//=============================================================================================================

void TestDspFilterChpi::testMultipleFrequencies()
{
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(3, 0);

    MatrixXd data(3, nSamples);
    for (int ch = 0; ch < 3; ++ch) {
        data.row(ch) = sineTone(83.0, sfreq, nSamples)
                      + sineTone(143.0, sfreq, nSamples)
                      + sineTone(203.0, sfreq, nSamples);
    }

    const double pow83Before = powerAtFreq(data.row(0), 83.0, sfreq);
    const double pow143Before = powerAtFreq(data.row(0), 143.0, sfreq);
    const double pow203Before = powerAtFreq(data.row(0), 203.0, sfreq);

    QVector<double> hpiFreqs = {83.0, 143.0, 203.0};
    filterChpi(data, info, sfreq, hpiFreqs);

    const double pow83After = powerAtFreq(data.row(0), 83.0, sfreq);
    const double pow143After = powerAtFreq(data.row(0), 143.0, sfreq);
    const double pow203After = powerAtFreq(data.row(0), 203.0, sfreq);

    // All three should be attenuated by > 40 dB
    QVERIFY(10.0 * std::log10(pow83Before / pow83After) > 40.0);
    QVERIFY(10.0 * std::log10(pow143Before / pow143After) > 40.0);
    QVERIFY(10.0 * std::log10(pow203Before / pow203After) > 40.0);
}

//=============================================================================================================

void TestDspFilterChpi::testLowFrequencyPreserved()
{
    // A 10 Hz signal should be preserved (within 1%) after notching at 143 Hz
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(1, 0);

    MatrixXd data(1, nSamples);
    data.row(0) = sineTone(10.0, sfreq, nSamples);

    const double powerBefore = powerAtFreq(data.row(0), 10.0, sfreq);

    QVector<double> hpiFreqs = {143.0};
    filterChpi(data, info, sfreq, hpiFreqs);

    const double powerAfter = powerAtFreq(data.row(0), 10.0, sfreq);

    // Should be within 1%
    const double ratio = powerAfter / powerBefore;
    QVERIFY2(std::abs(ratio - 1.0) < 0.01,
             qPrintable(QString("10 Hz power ratio = %1, expected ~1.0").arg(ratio)));
}

//=============================================================================================================

void TestDspFilterChpi::testMegOnlyFiltered()
{
    // Default bMegOnly=true: MEG channels filtered, EEG unchanged
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(3, 1);

    MatrixXd data(4, nSamples);
    for (int ch = 0; ch < 4; ++ch) {
        data.row(ch) = sineTone(143.0, sfreq, nSamples);
    }

    MatrixXd dataCopy = data;

    QVector<double> hpiFreqs = {143.0};
    filterChpi(data, info, sfreq, hpiFreqs);

    // MEG channels should be filtered (different from original)
    for (int ch = 0; ch < 3; ++ch) {
        QVERIFY(data.row(ch) != dataCopy.row(ch));
    }

    // EEG channel (index 3) should be unchanged
    QVERIFY(data.row(3).isApprox(dataCopy.row(3), 1e-12));
}

//=============================================================================================================

void TestDspFilterChpi::testAllChannelsFiltered()
{
    // bMegOnly=false: all channels filtered
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(3, 1);

    MatrixXd data(4, nSamples);
    for (int ch = 0; ch < 4; ++ch) {
        data.row(ch) = sineTone(143.0, sfreq, nSamples);
    }

    MatrixXd dataCopy = data;

    FilterChpiParams params;
    params.bMegOnly = false;

    QVector<double> hpiFreqs = {143.0};
    filterChpi(data, info, sfreq, hpiFreqs, params);

    // All channels should be filtered
    for (int ch = 0; ch < 4; ++ch) {
        QVERIFY(data.row(ch) != dataCopy.row(ch));
    }
}

//=============================================================================================================

void TestDspFilterChpi::testEmptyFrequencyList()
{
    // Empty frequency list → data unchanged
    const double sfreq = 1000.0;
    const int nSamples = 1000;
    FiffInfo info = makeFiffInfo(2, 0);

    MatrixXd data(2, nSamples);
    data.row(0) = sineTone(143.0, sfreq, nSamples);
    data.row(1) = sineTone(83.0, sfreq, nSamples);

    MatrixXd dataCopy = data;

    QVector<double> hpiFreqs;
    filterChpi(data, info, sfreq, hpiFreqs);

    QVERIFY(data.isApprox(dataCopy, 1e-15));
}

//=============================================================================================================

void TestDspFilterChpi::testInvalidFrequency()
{
    // Frequency > Nyquist → skipped with warning, data unchanged for that frequency
    const double sfreq = 1000.0;
    const int nSamples = 5000;
    FiffInfo info = makeFiffInfo(1, 0);

    MatrixXd data(1, nSamples);
    data.row(0) = sineTone(10.0, sfreq, nSamples);

    MatrixXd dataCopy = data;

    // 600 Hz > Nyquist (500 Hz) → should be skipped
    QVector<double> hpiFreqs = {600.0};
    filterChpi(data, info, sfreq, hpiFreqs);

    QVERIFY(data.isApprox(dataCopy, 1e-15));
}

//=============================================================================================================

void TestDspFilterChpi::testAutoDetectFallback()
{
    // Auto-detect overload with no HPI info → warning, no change
    const double sfreq = 1000.0;
    const int nSamples = 1000;
    FiffInfo info = makeFiffInfo(2, 0);

    MatrixXd data(2, nSamples);
    data.row(0) = sineTone(143.0, sfreq, nSamples);
    data.row(1) = sineTone(83.0, sfreq, nSamples);

    MatrixXd dataCopy = data;

    filterChpi(data, info, sfreq);

    // Should be unchanged (auto-detect not implemented)
    QVERIFY(data.isApprox(dataCopy, 1e-15));
}

//=============================================================================================================

void TestDspFilterChpi::testNotchWidth()
{
    // Wider notch removes more bandwidth: compare power at freq +/- 3 Hz offset
    const double sfreq = 1000.0;
    const int nSamples = 10000;
    FiffInfo info = makeFiffInfo(1, 0);

    // Signal at 143 Hz + sidebands at 140 Hz and 146 Hz
    MatrixXd dataNarrow(1, nSamples);
    dataNarrow.row(0) = sineTone(143.0, sfreq, nSamples)
                       + sineTone(140.0, sfreq, nSamples)
                       + sineTone(146.0, sfreq, nSamples);

    MatrixXd dataWide = dataNarrow;

    // Narrow notch: width=2 Hz (default)
    FilterChpiParams paramsNarrow;
    paramsNarrow.dNotchWidth = 2.0;
    QVector<double> hpiFreqs = {143.0};
    filterChpi(dataNarrow, info, sfreq, hpiFreqs, paramsNarrow);

    // Wide notch: width=5 Hz
    FilterChpiParams paramsWide;
    paramsWide.dNotchWidth = 5.0;
    filterChpi(dataWide, info, sfreq, hpiFreqs, paramsWide);

    // With narrow notch, sideband at 140 Hz should be less attenuated than with wide notch
    const double pow140Narrow = powerAtFreq(dataNarrow.row(0), 140.0, sfreq);
    const double pow140Wide = powerAtFreq(dataWide.row(0), 140.0, sfreq);

    // Wide notch should remove more of the 140 Hz sideband
    QVERIFY2(pow140Wide < pow140Narrow,
             qPrintable(QString("Wide notch power at 140 Hz (%1) should be less than narrow (%2)")
                        .arg(pow140Wide).arg(pow140Narrow)));
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspFilterChpi)

#include "test_dsp_filter_chpi.moc"
