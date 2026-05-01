//=============================================================================================================
/**
 * @file     test_dsp_bad_channels_lof.cpp
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
 * @brief    Tests for LOF-based bad channel detection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/bad_channels_lof.h>
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

#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * @brief Tests for LOF bad channel detection.
 */
class TestDspBadChannelsLof : public QObject
{
    Q_OBJECT

private:
    static FiffInfo makeTestInfo(int nCh, int kind = FIFFV_EEG_CH)
    {
        FiffInfo info;
        info.nchan = nCh;
        info.sfreq = 1000.0;

        QString prefix = (kind == FIFFV_MEG_CH) ? "MEG" : "EEG";
        for (int i = 0; i < nCh; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 1;
            ch.logNo = i + 1;
            ch.kind = kind;
            ch.ch_name = QString("%1%2").arg(prefix).arg(i + 1);
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

private slots:
    void initTestCase() {}

    void testLofScoresNormalData()
    {
        // All points similar → LOF ≈ 1
        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        MatrixXd features(20, 3);
        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 3; ++j) {
                features(i, j) = dist(gen);
            }
        }

        VectorXd lof = computeLofScores(features, 5);

        QCOMPARE(lof.size(), static_cast<Eigen::Index>(20));

        // All scores should be close to 1 for uniform data
        for (int i = 0; i < 20; ++i) {
            QVERIFY2(lof(i) > 0.5 && lof(i) < 3.0,
                     qPrintable(QString("LOF(%1)=%2, expected near 1").arg(i).arg(lof(i))));
        }
    }

    void testLofScoresDetectsOutlier()
    {
        // Create cluster of normal points + one outlier
        MatrixXd features(21, 2);
        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 0.5);

        for (int i = 0; i < 20; ++i) {
            features(i, 0) = dist(gen);
            features(i, 1) = dist(gen);
        }
        // Outlier far from cluster
        features(20, 0) = 20.0;
        features(20, 1) = 20.0;

        VectorXd lof = computeLofScores(features, 5);

        // Outlier should have highest LOF score
        int maxIdx = 0;
        for (int i = 1; i < 21; ++i) {
            if (lof(i) > lof(maxIdx)) maxIdx = i;
        }
        QCOMPARE(maxIdx, 20);
        QVERIFY2(lof(20) > 2.0,
                 qPrintable(QString("Outlier LOF=%1, expected > 2").arg(lof(20))));
    }

    void testFindBadChannelsWithOutlier()
    {
        int nCh = 20;
        FiffInfo info = makeTestInfo(nCh);

        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        MatrixXd data(nCh, 1000);
        for (int i = 0; i < nCh; ++i) {
            for (int j = 0; j < 1000; ++j) {
                data(i, j) = dist(gen);
            }
        }

        // Make channel 5 an outlier: huge amplitude
        data.row(5) *= 100.0;

        LofBadChannelParams params;
        params.iNNeighbors = 5;
        params.dThreshold = 2.0;

        QStringList bads = findBadChannelsLof(data, info, params);
        QVERIFY2(bads.contains("EEG6"),
                 qPrintable(QString("Expected EEG6 in bads, got: %1").arg(bads.join(", "))));
    }

    void testCleanDataNoBads()
    {
        int nCh = 15;
        FiffInfo info = makeTestInfo(nCh);

        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        MatrixXd data(nCh, 1000);
        for (int i = 0; i < nCh; ++i) {
            for (int j = 0; j < 1000; ++j) {
                data(i, j) = dist(gen);
            }
        }

        LofBadChannelParams params;
        params.iNNeighbors = 5;
        params.dThreshold = 3.0;  // High threshold

        QStringList bads = findBadChannelsLof(data, info, params);
        // With uniform data and high threshold, should find few/no bads
        QVERIFY2(bads.size() <= 2,
                 qPrintable(QString("Too many false positives: %1").arg(bads.size())));
    }

    void testMegOnlyFilter()
    {
        // Mix of MEG and EEG channels
        FiffInfo info;
        info.nchan = 6;
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
        for (int i = 0; i < 3; ++i) {
            FiffChInfo ch;
            ch.scanNo = i + 4;
            ch.logNo = i + 4;
            ch.kind = FIFFV_EEG_CH;
            ch.ch_name = QString("EEG%1").arg(i + 1);
            ch.range = 1.0f;
            ch.cal = 1.0f;
            info.chs.append(ch);
            info.ch_names.append(ch.ch_name);
        }

        MatrixXd data = MatrixXd::Random(6, 500);

        LofBadChannelParams params;
        params.bMegOnly = true;

        // Should only check MEG channels, but 3 channels is minimum
        QStringList bads = findBadChannelsLof(data, info, params);
        for (const QString& name : bads) {
            QVERIFY2(name.startsWith("MEG"),
                     qPrintable(QString("MEG-only mode returned non-MEG channel: %1").arg(name)));
        }
    }

    void testTooFewChannels()
    {
        FiffInfo info = makeTestInfo(2);
        MatrixXd data = MatrixXd::Random(2, 100);
        QStringList bads = findBadChannelsLof(data, info);
        QVERIFY(bads.isEmpty());
    }

    void testFlatChannelDetected()
    {
        int nCh = 15;
        FiffInfo info = makeTestInfo(nCh);

        std::mt19937 gen(42);
        std::normal_distribution<> dist(0.0, 1.0);

        MatrixXd data(nCh, 1000);
        for (int i = 0; i < nCh; ++i) {
            for (int j = 0; j < 1000; ++j) {
                data(i, j) = dist(gen);
            }
        }

        // Make channel 3 flat
        data.row(3).setZero();

        LofBadChannelParams params;
        params.iNNeighbors = 5;
        params.dThreshold = 2.0;

        QStringList bads = findBadChannelsLof(data, info, params);
        QVERIFY2(bads.contains("EEG4"),
                 qPrintable(QString("Expected flat EEG4 in bads, got: %1").arg(bads.join(", "))));
    }

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspBadChannelsLof)
#include "test_dsp_bad_channels_lof.moc"
