//=============================================================================================================
/**
 * @file     test_trap_music.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for InvTrapMusic (TRAP-MUSIC source localization).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/rap_music/inv_trap_music.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>
#include <random>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================

class TestTrapMusic : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConstructor();
    void testSingleSource();
    void testTwoSources();
    void testCorrelationAboveThreshold();
    void testScanCorrelations();
    void testFixedOrient();
    void testThresholdStop();
    void testEmptyInput();
    void testDimensionMismatch();
    void testSourcePosition();
    void cleanupTestCase();

private:
    MatrixXd m_leadField;    // nCh × nSrc*3
    MatrixXd m_sourcePos;    // nSrc × 3
    int m_nCh;
    int m_nSrc;
};

//=============================================================================================================

void TestTrapMusic::initTestCase()
{
    m_nCh = 20;
    m_nSrc = 50;

    std::mt19937 gen(42);
    std::normal_distribution<double> dist(0.0, 1.0);

    // Random lead field (nCh × nSrc*3)
    m_leadField = MatrixXd::Zero(m_nCh, m_nSrc * 3);
    for (int i = 0; i < m_nCh; ++i)
        for (int j = 0; j < m_nSrc * 3; ++j)
            m_leadField(i, j) = dist(gen);

    // Random source positions
    m_sourcePos = MatrixXd::Zero(m_nSrc, 3);
    for (int i = 0; i < m_nSrc; ++i) {
        // Place on a sphere
        double theta = M_PI * i / m_nSrc;
        double phi = 2.0 * M_PI * (i * 1.618);
        m_sourcePos(i, 0) = 0.07 * std::sin(theta) * std::cos(phi);
        m_sourcePos(i, 1) = 0.07 * std::sin(theta) * std::sin(phi);
        m_sourcePos(i, 2) = 0.07 * std::cos(theta);
    }
}

//=============================================================================================================

void TestTrapMusic::testConstructor()
{
    InvTrapMusic trap(3, 0.9);
    // No crash = pass
    QVERIFY(true);
}

//=============================================================================================================

void TestTrapMusic::testSingleSource()
{
    // Generate data from a single source
    int srcIdx = 10;
    int nTimes = 50;

    // Source activation: one dipole with fixed orientation
    Vector3d orient(1.0, 0.0, 0.0);
    orient.normalize();
    VectorXd timeSeries = VectorXd::LinSpaced(nTimes, 0.0, 1.0).array().sin();

    MatrixXd lfSrc = m_leadField.block(0, srcIdx * 3, m_nCh, 3);
    VectorXd forwardSignal = lfSrc * orient;
    MatrixXd data = forwardSignal * timeSeries.transpose();

    InvTrapMusic trap(1, 0.5);
    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, data, m_sourcePos, 3);

    QVERIFY(!dipoles.isEmpty());
    QCOMPARE(dipoles[0].sourceIdx, srcIdx);
}

//=============================================================================================================

void TestTrapMusic::testTwoSources()
{
    // Generate data from two uncorrelated sources
    int srcIdx1 = 5;
    int srcIdx2 = 30;
    int nTimes = 100;

    std::mt19937 gen(123);
    std::normal_distribution<double> dist(0.0, 1.0);

    Vector3d orient1(1.0, 0.0, 0.0);
    Vector3d orient2(0.0, 1.0, 0.0);

    VectorXd ts1(nTimes), ts2(nTimes);
    for (int t = 0; t < nTimes; ++t) {
        ts1[t] = std::sin(2.0 * M_PI * 10.0 * t / 100.0);
        ts2[t] = std::cos(2.0 * M_PI * 7.0 * t / 100.0);
    }

    MatrixXd lfSrc1 = m_leadField.block(0, srcIdx1 * 3, m_nCh, 3);
    MatrixXd lfSrc2 = m_leadField.block(0, srcIdx2 * 3, m_nCh, 3);

    MatrixXd data = lfSrc1 * orient1 * ts1.transpose() + lfSrc2 * orient2 * ts2.transpose();

    // Add small noise
    for (int i = 0; i < m_nCh; ++i)
        for (int t = 0; t < nTimes; ++t)
            data(i, t) += dist(gen) * 0.01;

    InvTrapMusic trap(2, 0.1);
    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, data, m_sourcePos, 3);

    QVERIFY2(dipoles.size() >= 2,
             qPrintable(QString("Found %1 sources, expected >= 2").arg(dipoles.size())));

    // Check that the two found sources are distinct
    QVERIFY2(dipoles[0].sourceIdx != dipoles[1].sourceIdx,
             "Found two sources should have different indices");

    // Both should have positive correlation
    QVERIFY(dipoles[0].correlation > 0.0);
    QVERIFY(dipoles[1].correlation > 0.0);
}

//=============================================================================================================

void TestTrapMusic::testCorrelationAboveThreshold()
{
    int srcIdx = 15;
    int nTimes = 50;
    Vector3d orient(0.0, 0.0, 1.0);

    VectorXd ts = VectorXd::LinSpaced(nTimes, 0.0, 2.0 * M_PI).array().sin();
    MatrixXd lfSrc = m_leadField.block(0, srcIdx * 3, m_nCh, 3);
    MatrixXd data = lfSrc * orient * ts.transpose();

    InvTrapMusic trap(1, 0.5);
    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, data, m_sourcePos, 3);

    QVERIFY(!dipoles.isEmpty());
    QVERIFY2(dipoles[0].correlation >= 0.5,
             qPrintable(QString("Correlation %1 below threshold").arg(dipoles[0].correlation)));
}

//=============================================================================================================

void TestTrapMusic::testScanCorrelations()
{
    // Build a simple signal subspace from one source
    int srcIdx = 20;
    Vector3d orient(1.0, 0.0, 0.0);
    MatrixXd lfSrc = m_leadField.block(0, srcIdx * 3, m_nCh, 3);
    VectorXd signal = lfSrc * orient;
    signal.normalize();

    MatrixXd signalSubspace = signal;  // nCh × 1

    VectorXd corrs = InvTrapMusic::scanCorrelations(m_leadField, signalSubspace, 3);

    QCOMPARE(static_cast<int>(corrs.size()), m_nSrc);

    // The true source should have the highest correlation
    Index bestIdx = 0;
    corrs.maxCoeff(&bestIdx);
    QCOMPARE(static_cast<int>(bestIdx), srcIdx);
}

//=============================================================================================================

void TestTrapMusic::testFixedOrient()
{
    // Test with fixed orientation (1 orientation per source)
    MatrixXd lfFixed = MatrixXd::Zero(m_nCh, m_nSrc);
    for (int i = 0; i < m_nCh; ++i)
        for (int j = 0; j < m_nSrc; ++j)
            lfFixed(i, j) = m_leadField(i, j * 3);  // Take first orientation

    int srcIdx = 8;
    int nTimes = 50;

    VectorXd ts = VectorXd::LinSpaced(nTimes, 0.0, 2.0 * M_PI).array().sin();
    MatrixXd data = lfFixed.col(srcIdx) * ts.transpose();

    InvTrapMusic trap(1, 0.5);
    QList<TrapMusicDipole> dipoles = trap.compute(lfFixed, data, m_sourcePos, 1);

    QVERIFY(!dipoles.isEmpty());
    QCOMPARE(dipoles[0].sourceIdx, srcIdx);
}

//=============================================================================================================

void TestTrapMusic::testThresholdStop()
{
    // With a very high threshold, should find no sources from pure noise
    std::mt19937 gen(99);
    std::normal_distribution<double> dist(0.0, 0.001);

    int nTimes = 50;
    MatrixXd noiseData(m_nCh, nTimes);
    for (int i = 0; i < m_nCh; ++i)
        for (int t = 0; t < nTimes; ++t)
            noiseData(i, t) = dist(gen);

    InvTrapMusic trap(3, 0.99);
    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, noiseData, m_sourcePos, 3);

    // With a 0.99 threshold and pure noise, should find very few or no sources
    QVERIFY2(dipoles.size() <= 2,
             qPrintable(QString("Found %1 sources from pure noise, expected <= 2").arg(dipoles.size())));
}

//=============================================================================================================

void TestTrapMusic::testEmptyInput()
{
    InvTrapMusic trap;
    MatrixXd emptyLF, emptyData, emptyPos;

    QList<TrapMusicDipole> dipoles = trap.compute(emptyLF, emptyData, emptyPos, 3);
    QVERIFY(dipoles.isEmpty());
}

//=============================================================================================================

void TestTrapMusic::testDimensionMismatch()
{
    MatrixXd badData(5, 10);  // Wrong number of channels
    InvTrapMusic trap;

    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, badData, m_sourcePos, 3);
    QVERIFY(dipoles.isEmpty());
}

//=============================================================================================================

void TestTrapMusic::testSourcePosition()
{
    int srcIdx = 25;
    int nTimes = 50;
    Vector3d orient(1.0, 0.0, 0.0);

    VectorXd ts = VectorXd::LinSpaced(nTimes, 0.0, 2.0 * M_PI).array().sin();
    MatrixXd lfSrc = m_leadField.block(0, srcIdx * 3, m_nCh, 3);
    MatrixXd data = lfSrc * orient * ts.transpose();

    InvTrapMusic trap(1, 0.5);
    QList<TrapMusicDipole> dipoles = trap.compute(m_leadField, data, m_sourcePos, 3);

    QVERIFY(!dipoles.isEmpty());

    // Position should match the source position in the grid
    Vector3d expectedPos = m_sourcePos.row(srcIdx).transpose();
    double dist = (dipoles[0].position - expectedPos).norm();
    QVERIFY2(dist < 1e-10, qPrintable(QString("Position mismatch: %1").arg(dist)));
}

//=============================================================================================================

void TestTrapMusic::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestTrapMusic)
#include "test_trap_music.moc"
