//=============================================================================================================
/**
 * @file     test_dsp_peak_finder.cpp
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
 * @brief    Tests for peak finder.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/peak_finder.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

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
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for peak finder.
 */
class TestDspPeakFinder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void testSineWavePeaks()
    {
        // Sine wave with 3 complete cycles: should have 3 peaks
        const int n = 300;
        VectorXd data(n);
        for (int i = 0; i < n; ++i) {
            data(i) = std::sin(2.0 * M_PI * 3.0 * static_cast<double>(i) / static_cast<double>(n));
        }

        auto peaks = peakFinder(data);
        QCOMPARE(peaks.size(), 3);

        // Peaks should be near +1.0
        for (const auto& peak : peaks) {
            QVERIFY2(std::abs(peak.second - 1.0) < 0.05,
                     qPrintable(QString("Peak value=%1, expected ~1.0").arg(peak.second)));
        }
    }

    void testMinHeightFilter()
    {
        VectorXd data(11);
        data << 0, 1, 0, 0.3, 0, 2, 0, 0.5, 0, 3, 0;

        PeakFinderParams params;
        params.dMinHeight = 1.5;

        auto peaks = peakFinder(data, params);
        QCOMPARE(peaks.size(), 2);  // Only peaks at 2.0 and 3.0
        QCOMPARE(peaks[0].first, 5);
        QCOMPARE(peaks[1].first, 9);
    }

    void testMinDistanceFilter()
    {
        // Many closely spaced peaks
        VectorXd data(20);
        for (int i = 0; i < 20; ++i) {
            data(i) = (i % 2 == 0) ? 0.0 : 1.0;
        }
        // Make some peaks taller
        data(5) = 3.0;
        data(15) = 2.0;

        PeakFinderParams params;
        params.iMinDistance = 5;

        auto peaks = peakFinder(data, params);

        // Check minimum distance between peaks
        for (int i = 1; i < peaks.size(); ++i) {
            int dist = peaks[i].first - peaks[i - 1].first;
            QVERIFY2(dist >= 5,
                     qPrintable(QString("Distance=%1 < minDistance=5").arg(dist)));
        }
    }

    void testProminenceFilter()
    {
        // Signal with peaks of varying prominence
        VectorXd data(30);
        data.setZero();
        data(5) = 1.0;    // Low prominence (between zeros)
        data(15) = 5.0;   // High prominence
        data(14) = 4.5;   // Shoulder — makes prominence of peak at 15 = 5-0=5
        data(25) = 0.5;   // Very low prominence

        PeakFinderParams params;
        params.dProminence = 2.0;

        auto peaks = peakFinder(data, params);

        // Only the high-prominence peak should remain
        bool foundBigPeak = false;
        for (const auto& peak : peaks) {
            if (peak.first == 15) foundBigPeak = true;
        }
        QVERIFY2(foundBigPeak, "Should find the prominent peak at index 15");
    }

    void testEmptySignal()
    {
        VectorXd data(2);
        data << 1, 0;
        auto peaks = peakFinder(data);
        QVERIFY(peaks.isEmpty());
    }

    void testFlatSignal()
    {
        VectorXd data = VectorXd::Constant(100, 5.0);
        auto peaks = peakFinder(data);
        QVERIFY(peaks.isEmpty());
    }

    void testSinglePeak()
    {
        VectorXd data(5);
        data << 0, 1, 3, 1, 0;
        auto peaks = peakFinder(data);
        QCOMPARE(peaks.size(), 1);
        QCOMPARE(peaks[0].first, 2);
        QCOMPARE(peaks[0].second, 3.0);
    }

    void testProminenceComputation()
    {
        VectorXd data(9);
        data << 0, 3, 0, 1, 0, 5, 0, 2, 0;

        QList<int> peakIdx = {1, 3, 5, 7};
        VectorXd proms = peakProminences(data, peakIdx);

        QCOMPARE(proms.size(), static_cast<Eigen::Index>(4));
        // Peak at 5 (value=5.0) has prominence 5.0 (goes down to 0)
        QVERIFY(std::abs(proms(2) - 5.0) < 0.01);
        // Peak at 3 (value=1.0) has prominence 1.0 (between higher peaks)
        QVERIFY(std::abs(proms(1) - 1.0) < 0.01);
    }

    void testPeaksSortedByIndex()
    {
        VectorXd data(100);
        for (int i = 0; i < 100; ++i) {
            data(i) = std::sin(2.0 * M_PI * 5.0 * static_cast<double>(i) / 100.0);
        }

        auto peaks = peakFinder(data);
        for (int i = 1; i < peaks.size(); ++i) {
            QVERIFY(peaks[i].first > peaks[i - 1].first);
        }
    }

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspPeakFinder)
#include "test_dsp_peak_finder.moc"
