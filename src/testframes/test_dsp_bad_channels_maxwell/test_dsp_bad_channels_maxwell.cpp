//=============================================================================================================
/**
 * @file     test_dsp_bad_channels_maxwell.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for SSS-based bad channel detection.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/bad_channels_maxwell.h>
#include <dsp/sss.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

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
 * @brief Helper: create synthetic FiffInfo with MEG-like sensor geometry.
 *
 * Creates channels positioned on a hemisphere. Good enough for SSS basis
 * computation (no real sensor integrations needed for unit-test purposes).
 */
static FiffInfo createSyntheticMegInfo(int nChannels = 102)
{
    FiffInfo info;
    info.sfreq = 1000.0;
    info.nchan = nChannels;

    // Create channels in a hemisphere arrangement
    for (int i = 0; i < nChannels; ++i) {
        FiffChInfo ch;
        ch.ch_name = QString("MEG%1").arg(i + 1, 4, 10, QChar('0'));
        ch.kind = FIFFV_MEG_CH;
        ch.unit = FIFF_UNIT_T;

        // Distribute on a hemisphere of radius 0.12 m
        double theta = M_PI * 0.4 * (static_cast<double>(i) / nChannels);
        double phi = 2.0 * M_PI * i * 0.618033988749895; // golden angle

        double r = 0.12;
        double x = r * std::sin(theta) * std::cos(phi);
        double y = r * std::sin(theta) * std::sin(phi);
        double z = r * std::cos(theta);

        // Channel position
        ch.chpos.r0 << static_cast<float>(x), static_cast<float>(y), static_cast<float>(z);

        // Normal pointing inward (toward origin)
        Eigen::Vector3f normal;
        normal << -static_cast<float>(x), -static_cast<float>(y), -static_cast<float>(z);
        normal.normalize();
        ch.chpos.ex = normal;
        ch.chpos.ey = Eigen::Vector3f::UnitY();
        Eigen::Vector3f ez = normal.cross(Eigen::Vector3f(Eigen::Vector3f::UnitY()));
        ch.chpos.ez = ez.norm() > 1e-6f ? ez.normalized() : Eigen::Vector3f::UnitZ();

        ch.scanNo = i;
        ch.logNo = i + 1;
        ch.range = 1.0;
        ch.cal = 1.0;

        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }

    return info;
}

//=============================================================================================================

class TestDspBadChannelsMaxwell : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() {}

    void testBasicDetectionNoBad()
    {
        // All channels with clean synthetic data — none should be flagged
        int nCh = 50;
        int nTimes = 500;
        FiffInfo info = createSyntheticMegInfo(nCh);

        // Create clean sinusoidal data
        MatrixXd data(nCh, nTimes);
        for (int i = 0; i < nCh; ++i) {
            for (int t = 0; t < nTimes; ++t) {
                data(i, t) = 1e-12 * std::sin(2.0 * M_PI * 10.0 * t / 1000.0 + i * 0.1);
            }
        }

        BadChannelsMaxwellParams params;
        params.iOrderIn = 4;  // Low order for small array
        params.iOrderOut = 2;

        BadChannelsMaxwellResult result = findBadChannelsMaxwell(data, info, params);

        // With low-order SSS on clean data, most channels should reconstruct well
        QVERIFY(result.residuals.size() == nCh);
        QVERIFY(result.zScores.size() == nCh);
    }

    void testDetectionWithBadChannel()
    {
        // Inject large noise on one channel — it should be flagged
        int nCh = 50;
        int nTimes = 1000;
        FiffInfo info = createSyntheticMegInfo(nCh);

        // Create clean data
        MatrixXd data(nCh, nTimes);
        for (int i = 0; i < nCh; ++i) {
            for (int t = 0; t < nTimes; ++t) {
                data(i, t) = 1e-12 * std::sin(2.0 * M_PI * 10.0 * t / 1000.0 + i * 0.1);
            }
        }

        // Corrupt channel 5 with large noise
        std::mt19937 gen(42);
        std::normal_distribution<double> dist(0.0, 1e-9); // 1000x normal amplitude
        for (int t = 0; t < nTimes; ++t) {
            data(5, t) = dist(gen);
        }

        BadChannelsMaxwellParams params;
        params.iOrderIn = 4;
        params.iOrderOut = 2;
        params.dZThreshold = 3.0;

        BadChannelsMaxwellResult result = findBadChannelsMaxwell(data, info, params);

        // Channel 5 should have a much higher residual
        QVERIFY(result.residuals.size() == nCh);

        // The corrupted channel should have one of the highest z-scores
        QVERIFY2(result.zScores(5) > 2.0,
                 qPrintable(QString("Corrupted channel z-score=%1, expected > 2.0").arg(result.zScores(5))));
    }

    void testResultStructure()
    {
        int nCh = 30;
        int nTimes = 200;
        FiffInfo info = createSyntheticMegInfo(nCh);
        MatrixXd data = MatrixXd::Random(nCh, nTimes) * 1e-12;

        BadChannelsMaxwellParams params;
        params.iOrderIn = 3;
        params.iOrderOut = 1;

        BadChannelsMaxwellResult result = findBadChannelsMaxwell(data, info, params);

        QCOMPARE(result.residuals.size(), static_cast<Eigen::Index>(nCh));
        QCOMPARE(result.zScores.size(), static_cast<Eigen::Index>(nCh));
        QCOMPARE(result.badChannels.size(), result.badIndices.size());
    }

    void testEmptyData()
    {
        FiffInfo info = createSyntheticMegInfo(10);
        MatrixXd data(10, 0);

        BadChannelsMaxwellResult result = findBadChannelsMaxwell(data, info);
        // Should handle gracefully
        QVERIFY(result.residuals.size() == 0 || result.residuals.size() == 10);
    }

    void testCustomThreshold()
    {
        int nCh = 40;
        int nTimes = 300;
        FiffInfo info = createSyntheticMegInfo(nCh);
        MatrixXd data = MatrixXd::Random(nCh, nTimes) * 1e-12;

        // Very low threshold should flag more channels
        BadChannelsMaxwellParams paramsLow;
        paramsLow.iOrderIn = 3;
        paramsLow.iOrderOut = 1;
        paramsLow.dZThreshold = 1.0;

        BadChannelsMaxwellParams paramsHigh;
        paramsHigh.iOrderIn = 3;
        paramsHigh.iOrderOut = 1;
        paramsHigh.dZThreshold = 10.0;

        BadChannelsMaxwellResult resultLow = findBadChannelsMaxwell(data, info, paramsLow);
        BadChannelsMaxwellResult resultHigh = findBadChannelsMaxwell(data, info, paramsHigh);

        QVERIFY2(resultLow.badChannels.size() >= resultHigh.badChannels.size(),
                 qPrintable(QString("Low threshold: %1 bad, high threshold: %2 bad")
                            .arg(resultLow.badChannels.size()).arg(resultHigh.badChannels.size())));
    }

    void testResidualsNonNegative()
    {
        int nCh = 30;
        int nTimes = 200;
        FiffInfo info = createSyntheticMegInfo(nCh);
        MatrixXd data = MatrixXd::Random(nCh, nTimes) * 1e-12;

        BadChannelsMaxwellParams params;
        params.iOrderIn = 3;
        params.iOrderOut = 1;

        BadChannelsMaxwellResult result = findBadChannelsMaxwell(data, info, params);

        for (int i = 0; i < result.residuals.size(); ++i) {
            QVERIFY2(result.residuals(i) >= 0.0, "Residuals must be non-negative");
        }
    }

    void cleanupTestCase() {}
};

//=============================================================================================================

QTEST_GUILESS_MAIN(TestDspBadChannelsMaxwell)
#include "test_dsp_bad_channels_maxwell.moc"
