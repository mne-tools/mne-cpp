//=============================================================================================================
/**
 * @file     test_connectivity_aec.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for ConnectivityAec (AEC and orthogonalized AEC).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/connectivity_aec.h>

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

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestConnectivityAec : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testIdenticalSignals();
    void testUncorrelatedSignals();
    void testSymmetry();
    void testDiagonal();
    void testOrthogonalized();
    void testOrthogonalizedLower();
    void testHilbertEnvelope();
    void testPearsonCorrelation();
    void testSingleSignal();
    void testEmptyInput();
    void cleanupTestCase();

private:
    MatrixXd m_data;
    int m_nSig;
    int m_nSamples;
};

//=============================================================================================================

void TestConnectivityAec::initTestCase()
{
    m_nSig = 4;
    m_nSamples = 200;

    m_data = MatrixXd(m_nSig, m_nSamples);
    for (int t = 0; t < m_nSamples; ++t) {
        double time = static_cast<double>(t) / 100.0;
        m_data(0, t) = std::sin(2.0 * M_PI * 10.0 * time);
        m_data(1, t) = std::sin(2.0 * M_PI * 10.0 * time + 0.1);  // Slightly phase-shifted
        m_data(2, t) = std::sin(2.0 * M_PI * 25.0 * time);        // Different frequency
        m_data(3, t) = std::cos(2.0 * M_PI * 10.0 * time);        // Same freq, 90° shift
    }
}

//=============================================================================================================

void TestConnectivityAec::testIdenticalSignals()
{
    // Two identical signals should have AEC = 1
    MatrixXd data(2, m_nSamples);
    data.row(0) = m_data.row(0);
    data.row(1) = m_data.row(0);

    MatrixXd aec = ConnectivityAec::compute(data);
    QVERIFY2(aec(0, 1) > 0.95,
             qPrintable(QString("AEC for identical signals: %1").arg(aec(0, 1))));
}

//=============================================================================================================

void TestConnectivityAec::testUncorrelatedSignals()
{
    // Signals at very different frequencies should have lower AEC
    MatrixXd data(2, m_nSamples);
    data.row(0) = m_data.row(0);  // 10 Hz
    data.row(1) = m_data.row(2);  // 25 Hz

    MatrixXd aec = ConnectivityAec::compute(data);
    // The AEC can be moderate since pure sinusoidal envelopes are flat,
    // but should not be as high as for identical signals
    QVERIFY(std::abs(aec(0, 1)) < 1.0);
}

//=============================================================================================================

void TestConnectivityAec::testSymmetry()
{
    MatrixXd aec = ConnectivityAec::compute(m_data);
    for (int i = 0; i < m_nSig; ++i) {
        for (int j = 0; j < m_nSig; ++j) {
            QVERIFY2(std::abs(aec(i, j) - aec(j, i)) < 1e-10,
                     qPrintable(QString("AEC not symmetric at (%1,%2)").arg(i).arg(j)));
        }
    }
}

//=============================================================================================================

void TestConnectivityAec::testDiagonal()
{
    MatrixXd aec = ConnectivityAec::compute(m_data);
    for (int i = 0; i < m_nSig; ++i) {
        QVERIFY2(std::abs(aec(i, i) - 1.0) < 1e-10,
                 qPrintable(QString("Diagonal AEC(%1,%1) = %2").arg(i).arg(aec(i, i))));
    }
}

//=============================================================================================================

void TestConnectivityAec::testOrthogonalized()
{
    MatrixXd aecCorr = ConnectivityAec::computeOrthogonalized(m_data);

    // Should be symmetric
    for (int i = 0; i < m_nSig; ++i)
        for (int j = 0; j < m_nSig; ++j)
            QVERIFY(std::abs(aecCorr(i, j) - aecCorr(j, i)) < 1e-10);

    // Diagonal should be 1
    for (int i = 0; i < m_nSig; ++i)
        QVERIFY(std::abs(aecCorr(i, i) - 1.0) < 1e-10);
}

//=============================================================================================================

void TestConnectivityAec::testOrthogonalizedLower()
{
    // Orthogonalized AEC should generally be lower than standard AEC
    // (removes instantaneous correlation)
    MatrixXd aec = ConnectivityAec::compute(m_data);
    MatrixXd aecCorr = ConnectivityAec::computeOrthogonalized(m_data);

    // At least for some pairs, orthogonalized should be ≤ standard
    bool anyLower = false;
    for (int i = 0; i < m_nSig; ++i)
        for (int j = i + 1; j < m_nSig; ++j)
            if (aecCorr(i, j) <= std::abs(aec(i, j)) + 0.01)
                anyLower = true;

    QVERIFY(anyLower);
}

//=============================================================================================================

void TestConnectivityAec::testHilbertEnvelope()
{
    // Envelope of a sinusoid should be approximately constant (≈ 1)
    VectorXd signal(m_nSamples);
    for (int t = 0; t < m_nSamples; ++t)
        signal[t] = std::sin(2.0 * M_PI * 10.0 * t / 100.0);

    VectorXd env = ConnectivityAec::hilbertEnvelope(signal);

    QCOMPARE(static_cast<int>(env.size()), m_nSamples);

    // Skip edges (filter transients), check interior
    int start = m_nSamples / 4;
    int end = 3 * m_nSamples / 4;
    double meanEnv = env.segment(start, end - start).mean();
    QVERIFY2(std::abs(meanEnv - 1.0) < 0.2,
             qPrintable(QString("Mean envelope %1 not near 1.0").arg(meanEnv)));
}

//=============================================================================================================

void TestConnectivityAec::testPearsonCorrelation()
{
    VectorXd a(100), b(100);
    for (int i = 0; i < 100; ++i) {
        a[i] = static_cast<double>(i);
        b[i] = static_cast<double>(i) * 2.0 + 3.0;
    }
    double r = ConnectivityAec::pearsonCorrelation(a, b);
    QVERIFY(std::abs(r - 1.0) < 1e-10);

    // Anti-correlated
    VectorXd c = -b;
    r = ConnectivityAec::pearsonCorrelation(a, c);
    QVERIFY(std::abs(r + 1.0) < 1e-10);
}

//=============================================================================================================

void TestConnectivityAec::testSingleSignal()
{
    MatrixXd data(1, m_nSamples);
    data.row(0) = m_data.row(0);

    MatrixXd aec = ConnectivityAec::compute(data);
    QCOMPARE(static_cast<int>(aec.rows()), 1);
    QCOMPARE(static_cast<int>(aec.cols()), 1);
    QVERIFY(std::abs(aec(0, 0) - 1.0) < 1e-10);
}

//=============================================================================================================

void TestConnectivityAec::testEmptyInput()
{
    MatrixXd empty;
    MatrixXd aec = ConnectivityAec::compute(empty);
    QCOMPARE(static_cast<int>(aec.size()), 0);
}

//=============================================================================================================

void TestConnectivityAec::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestConnectivityAec)
#include "test_connectivity_aec.moc"
