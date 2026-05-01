//=============================================================================================================
/**
 * @file     test_dsp_stim_artifact.cpp
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
 * @brief    Tests for fixStimArtifact.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/stim_artifact.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest/QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for fixStimArtifact function.
 */
class TestDspStimArtifact : public QObject
{
    Q_OBJECT

private slots:
    void testLinearInterpolation();
    void testZeroPadding();
    void testEventIdFilter();
    void testAllEvents();
    void testNegativeTmin();
    void testBoundaryClamp();
    void testMultipleEvents();
    void testEmptyEvents();
    void testSingleSampleWindow();
    void testMultiChannel();
};

//=============================================================================================================

void TestDspStimArtifact::testLinearInterpolation()
{
    // 1 channel, 20 samples, all ones
    MatrixXd data = MatrixXd::Ones(1, 20);

    // One event at sample 5, event id = 1
    MatrixXi events(1, 3);
    events << 5, 0, 1;

    // sfreq = 1000 Hz, tmin = 0.0, tmax = 0.005 => window [5, 10]
    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Linear);

    // With constant data, linear interpolation between 1.0 and 1.0 should give 1.0
    for (int s = 5; s <= 10; ++s) {
        QCOMPARE(data(0, s), 1.0);
    }

    // Now test with non-constant data: ramp 0..19
    for (int s = 0; s < 20; ++s) {
        data(0, s) = static_cast<double>(s);
    }
    // Set the window interior to a different value to verify interpolation replaces it
    data(0, 6) = 999.0;
    data(0, 7) = 999.0;
    data(0, 8) = 999.0;
    data(0, 9) = 999.0;

    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Linear);

    // Boundary values: data(0,5)=5.0, data(0,10)=10.0 (end boundary taken from original)
    // But wait: data(0,10) was not modified in our setup (still 10.0)
    // After interpolation: data(0, 5+s) = 5.0 + s * (10.0 - 5.0) / 5.0
    const double dStart = 5.0;
    const double dEnd = 10.0;
    const int iWindowLen = 6; // [5..10] inclusive
    for (int s = 0; s < iWindowLen; ++s) {
        const double dExpected = dStart + static_cast<double>(s) / static_cast<double>(iWindowLen - 1) * (dEnd - dStart);
        QVERIFY(std::abs(data(0, 5 + s) - dExpected) < 1e-12);
    }
}

//=============================================================================================================

void TestDspStimArtifact::testZeroPadding()
{
    MatrixXd data = MatrixXd::Ones(1, 20);

    MatrixXi events(1, 3);
    events << 5, 0, 1;

    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Window);

    // Samples [5..10] should be zero
    for (int s = 5; s <= 10; ++s) {
        QCOMPARE(data(0, s), 0.0);
    }
    // Samples outside window should be unchanged
    QCOMPARE(data(0, 4), 1.0);
    QCOMPARE(data(0, 11), 1.0);
}

//=============================================================================================================

void TestDspStimArtifact::testEventIdFilter()
{
    MatrixXd data = MatrixXd::Ones(1, 30);

    // Two events: id=1 at sample 5, id=2 at sample 20
    MatrixXi events(2, 3);
    events << 5, 0, 1,
              20, 0, 2;

    // Only process event id 2
    fixStimArtifact(data, events, 1000.0, 2, 0.0, 0.003, StimArtifactMode::Window);

    // Event at sample 5 (id=1) should be untouched
    QCOMPARE(data(0, 5), 1.0);
    QCOMPARE(data(0, 6), 1.0);
    QCOMPARE(data(0, 7), 1.0);
    QCOMPARE(data(0, 8), 1.0);

    // Event at sample 20 (id=2) should be zeroed: [20..23]
    QCOMPARE(data(0, 20), 0.0);
    QCOMPARE(data(0, 21), 0.0);
    QCOMPARE(data(0, 22), 0.0);
    QCOMPARE(data(0, 23), 0.0);
}

//=============================================================================================================

void TestDspStimArtifact::testAllEvents()
{
    MatrixXd data = MatrixXd::Ones(1, 30);

    MatrixXi events(2, 3);
    events << 5, 0, 1,
              20, 0, 2;

    // eventId = -1 processes all events
    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.002, StimArtifactMode::Window);

    // Both windows should be zeroed: [5..7] and [20..22]
    QCOMPARE(data(0, 5), 0.0);
    QCOMPARE(data(0, 6), 0.0);
    QCOMPARE(data(0, 7), 0.0);

    QCOMPARE(data(0, 20), 0.0);
    QCOMPARE(data(0, 21), 0.0);
    QCOMPARE(data(0, 22), 0.0);

    // Surrounding samples unchanged
    QCOMPARE(data(0, 4), 1.0);
    QCOMPARE(data(0, 8), 1.0);
    QCOMPARE(data(0, 19), 1.0);
    QCOMPARE(data(0, 23), 1.0);
}

//=============================================================================================================

void TestDspStimArtifact::testNegativeTmin()
{
    MatrixXd data = MatrixXd::Ones(1, 20);

    MatrixXi events(1, 3);
    events << 10, 0, 1;

    // tmin = -0.003, tmax = 0.002 => offsets: -3..+2 => window [7..12]
    fixStimArtifact(data, events, 1000.0, -1, -0.003, 0.002, StimArtifactMode::Window);

    QCOMPARE(data(0, 6), 1.0);   // before window
    for (int s = 7; s <= 12; ++s) {
        QCOMPARE(data(0, s), 0.0);
    }
    QCOMPARE(data(0, 13), 1.0);  // after window
}

//=============================================================================================================

void TestDspStimArtifact::testBoundaryClamp()
{
    MatrixXd data = MatrixXd::Ones(1, 10);

    // Event at sample 2, tmin = -0.005, tmax = 0.003 => offsets: -5..+3 => raw window [-3..5]
    // Clamped: [0..5]
    MatrixXi events(1, 3);
    events << 2, 0, 1;

    fixStimArtifact(data, events, 1000.0, -1, -0.005, 0.003, StimArtifactMode::Window);

    for (int s = 0; s <= 5; ++s) {
        QCOMPARE(data(0, s), 0.0);
    }
    QCOMPARE(data(0, 6), 1.0);

    // Event near end: event at sample 8, tmax = 0.005 => raw end = 13, clamped to 9
    data = MatrixXd::Ones(1, 10);
    MatrixXi events2(1, 3);
    events2 << 8, 0, 1;

    fixStimArtifact(data, events2, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Window);

    QCOMPARE(data(0, 7), 1.0);
    QCOMPARE(data(0, 8), 0.0);
    QCOMPARE(data(0, 9), 0.0);
}

//=============================================================================================================

void TestDspStimArtifact::testMultipleEvents()
{
    MatrixXd data = MatrixXd::Ones(1, 50);

    // Three events at samples 5, 20, 35
    MatrixXi events(3, 3);
    events << 5, 0, 1,
              20, 0, 1,
              35, 0, 1;

    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.003, StimArtifactMode::Window);

    // Windows: [5..8], [20..23], [35..38]
    auto verifyWindow = [&](int start, int end) {
        for (int s = start; s <= end; ++s) {
            QCOMPARE(data(0, s), 0.0);
        }
    };
    verifyWindow(5, 8);
    verifyWindow(20, 23);
    verifyWindow(35, 38);

    // Check some non-window samples
    QCOMPARE(data(0, 4), 1.0);
    QCOMPARE(data(0, 9), 1.0);
    QCOMPARE(data(0, 19), 1.0);
    QCOMPARE(data(0, 24), 1.0);
}

//=============================================================================================================

void TestDspStimArtifact::testEmptyEvents()
{
    MatrixXd data = MatrixXd::Ones(1, 20);
    MatrixXd dataCopy = data;

    MatrixXi events(0, 3);

    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Linear);

    QVERIFY(data.isApprox(dataCopy));
}

//=============================================================================================================

void TestDspStimArtifact::testSingleSampleWindow()
{
    MatrixXd data = MatrixXd::Ones(1, 20);

    MatrixXi events(1, 3);
    events << 10, 0, 1;

    // tmin == tmax => iMinOffset == iMaxOffset => iEnd == iStart => window skipped (iEnd <= iStart)
    MatrixXd dataCopy = data;
    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.0, StimArtifactMode::Window);

    // Data should be unchanged since window has zero width
    QVERIFY(data.isApprox(dataCopy));
}

//=============================================================================================================

void TestDspStimArtifact::testMultiChannel()
{
    // 3 channels, 30 samples
    MatrixXd data(3, 30);
    data.row(0).setConstant(1.0);
    data.row(1).setConstant(2.0);
    data.row(2).setConstant(3.0);

    MatrixXi events(1, 3);
    events << 10, 0, 1;

    // Window mode: [10..15]
    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Window);

    // All channels should be zeroed in the window
    for (int ch = 0; ch < 3; ++ch) {
        for (int s = 10; s <= 15; ++s) {
            QCOMPARE(data(ch, s), 0.0);
        }
    }

    // Outside window, original values should remain
    QCOMPARE(data(0, 9), 1.0);
    QCOMPARE(data(1, 9), 2.0);
    QCOMPARE(data(2, 9), 3.0);
    QCOMPARE(data(0, 16), 1.0);
    QCOMPARE(data(1, 16), 2.0);
    QCOMPARE(data(2, 16), 3.0);

    // Test linear mode with multiple channels using a ramp
    for (int ch = 0; ch < 3; ++ch) {
        for (int s = 0; s < 30; ++s) {
            data(ch, s) = static_cast<double>((ch + 1) * s);
        }
    }
    // Corrupt interior of the window
    for (int ch = 0; ch < 3; ++ch) {
        for (int s = 11; s <= 14; ++s) {
            data(ch, s) = 999.0;
        }
    }

    fixStimArtifact(data, events, 1000.0, -1, 0.0, 0.005, StimArtifactMode::Linear);

    // Verify linear interpolation for each channel
    for (int ch = 0; ch < 3; ++ch) {
        const double dStart = static_cast<double>((ch + 1) * 10);
        const double dEnd   = static_cast<double>((ch + 1) * 15);
        const int iWindowLen = 6; // [10..15]
        for (int s = 0; s < iWindowLen; ++s) {
            const double dExpected = dStart + static_cast<double>(s) / static_cast<double>(iWindowLen - 1) * (dEnd - dStart);
            QVERIFY2(std::abs(data(ch, 10 + s) - dExpected) < 1e-12,
                      qPrintable(QString("Channel %1, sample %2: expected %3, got %4")
                                 .arg(ch).arg(10 + s).arg(dExpected).arg(data(ch, 10 + s))));
        }
    }
}

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspStimArtifact)

#include "test_dsp_stim_artifact.moc"
