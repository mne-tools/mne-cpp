//=============================================================================================================
/**
 * @file     test_dsp_bridged_electrodes.cpp
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
 * @brief    Tests for bridged electrode detection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/bridged_electrodes.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>

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
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for bridged electrode detection.
 */
class TestDspBridgedElectrodes : public QObject
{
    Q_OBJECT

private:
    static FiffInfo makeTestInfo(int nEeg)
    {
        FiffInfo info;
        info.nchan = nEeg;
        info.sfreq = 1000.0;

        for (int i = 0; i < nEeg; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = FIFFV_EEG_CH;
            ch.ch_name = QString("EEG%1").arg(i + 1);
            ch.range = 1.0f;
            ch.cal = 1.0f;
            ch.unit = 107;
            ch.unit_mul = 0;
            ch.coord_frame = 0;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        return info;
    }

    static MatrixXd makeTestData(int nEeg, int nTimes, int bridgeCh1 = -1, int bridgeCh2 = -1)
    {
        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        MatrixXd data(nEeg, nTimes);
        for (int i = 0; i < nEeg; ++i) {
            for (int j = 0; j < nTimes; ++j) {
                data(i, j) = dist(gen);
            }
        }

        // Create a bridge: make ch2 nearly identical to ch1
        if (bridgeCh1 >= 0 && bridgeCh2 >= 0) {
            data.row(bridgeCh2) = data.row(bridgeCh1) + 0.001 * data.row(bridgeCh2);
        }

        return data;
    }

private slots:
    void initTestCase() {}

    void testDetectsBridgedPair()
    {
        FiffInfo info = makeTestInfo(8);
        MatrixXd data = makeTestData(8, 1000, 2, 5);  // EEG3 and EEG6 bridged

        auto bridged = computeBridgedElectrodes(data, info);

        // Should detect at least one bridge
        QVERIFY2(!bridged.isEmpty(), "Should detect bridged electrodes");

        // Check that the bridged pair includes our known bridge
        bool found = false;
        for (const auto& pair : bridged) {
            if ((pair.first == 2 && pair.second == 5) ||
                (pair.first == 5 && pair.second == 2)) {
                found = true;
                break;
            }
        }
        QVERIFY2(found, "Should detect the specific bridged pair (2, 5)");
    }

    void testNoBridgeCleanData()
    {
        FiffInfo info = makeTestInfo(8);
        MatrixXd data = makeTestData(8, 1000);  // No bridge

        BridgedElectrodeParams params;
        params.dElectricalDistanceThreshold = 0.1;  // Strict threshold

        auto bridged = computeBridgedElectrodes(data, info, params);
        QVERIFY2(bridged.isEmpty(), "Clean data should have no bridges");
    }

    void testElectricalDistanceMatrix()
    {
        FiffInfo info = makeTestInfo(4);
        MatrixXd data = makeTestData(4, 1000, 0, 1);  // Bridge channels 0 and 1

        MatrixXd edist = computeElectricalDistance(data, info);

        QCOMPARE(edist.rows(), static_cast<Eigen::Index>(4));
        QCOMPARE(edist.cols(), static_cast<Eigen::Index>(4));

        // Diagonal should be zero
        for (int i = 0; i < 4; ++i) {
            QCOMPARE(edist(i, i), 0.0);
        }

        // Bridged pair should have very low distance
        QVERIFY2(edist(0, 1) < 0.1,
                 qPrintable(QString("Bridged distance=%1, expected < 0.1").arg(edist(0, 1))));

        // Non-bridged pairs should have higher distance
        QVERIFY2(edist(0, 2) > 0.5,
                 qPrintable(QString("Non-bridged distance=%1, expected > 0.5").arg(edist(0, 2))));
    }

    void testSymmetricDistance()
    {
        FiffInfo info = makeTestInfo(5);
        MatrixXd data = makeTestData(5, 500);

        MatrixXd edist = computeElectricalDistance(data, info);

        double asymmetry = (edist - edist.transpose()).norm();
        QVERIFY2(asymmetry < 1e-12,
                 qPrintable(QString("Distance matrix asymmetry=%1").arg(asymmetry)));
    }

    void testThresholdAffectsResults()
    {
        FiffInfo info = makeTestInfo(6);
        MatrixXd data = makeTestData(6, 1000, 1, 3);

        BridgedElectrodeParams strict;
        strict.dElectricalDistanceThreshold = 0.05;

        BridgedElectrodeParams relaxed;
        relaxed.dElectricalDistanceThreshold = 0.5;

        auto bridgedStrict = computeBridgedElectrodes(data, info, strict);
        auto bridgedRelaxed = computeBridgedElectrodes(data, info, relaxed);

        QVERIFY2(bridgedRelaxed.size() >= bridgedStrict.size(),
                 "Relaxed threshold should find >= bridges than strict");
    }

    void testNoEegChannels()
    {
        // Info with only MEG channels
        FiffInfo info;
        info.nchan = 3;
        info.sfreq = 1000.0;
        for (int i = 0; i < 3; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = FIFFV_MEG_CH;
            ch.ch_name = QString("MEG%1").arg(i + 1);
            ch.range = 1.0f;
            ch.cal = 1.0f;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        MatrixXd data = MatrixXd::Random(3, 100);
        auto bridged = computeBridgedElectrodes(data, info);
        QVERIFY(bridged.isEmpty());
    }

    void testSingleEegChannel()
    {
        FiffInfo info = makeTestInfo(1);
        MatrixXd data = MatrixXd::Random(1, 100);
        auto bridged = computeBridgedElectrodes(data, info);
        QVERIFY(bridged.isEmpty());
    }

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspBridgedElectrodes)
#include "test_dsp_bridged_electrodes.moc"
