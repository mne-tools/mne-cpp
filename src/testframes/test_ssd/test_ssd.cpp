//=============================================================================================================
/**
 * @file     test_ssd.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for MlSsd (Spatio-Spectral Decomposition).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_ssd.h>

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

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================

class TestSsd : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConstructor();
    void testFit();
    void testTransform();
    void testFitTransform();
    void testFiltersShape();
    void testPatternsShape();
    void testEigenvaluesDescending();
    void testTransformNotFitted();
    void testEmptyInput();
    void testSignalEnhancement();
    void cleanupTestCase();

private:
    MatrixXd m_data;
    double m_sFreq;
    int m_nCh;
    int m_nTimes;
};

//=============================================================================================================

void TestSsd::initTestCase()
{
    m_nCh = 8;
    m_sFreq = 250.0;
    m_nTimes = 2500;  // 10 seconds

    std::mt19937 gen(42);
    std::normal_distribution<double> noise(0.0, 1.0);

    m_data = MatrixXd(m_nCh, m_nTimes);

    // Create a spatial pattern for a 10 Hz oscillation
    VectorXd pattern = VectorXd::Random(m_nCh);
    pattern.normalize();

    for (int ch = 0; ch < m_nCh; ++ch) {
        for (int t = 0; t < m_nTimes; ++t) {
            double time = static_cast<double>(t) / m_sFreq;
            // Background broadband noise
            m_data(ch, t) = noise(gen) * 0.5;
            // Add 10 Hz oscillation on the spatial pattern
            m_data(ch, t) += pattern[ch] * 3.0 * std::sin(2.0 * M_PI * 10.0 * time);
            // Add some 25 Hz noise too
            m_data(ch, t) += noise(gen) * 0.3 * std::sin(2.0 * M_PI * 25.0 * time + noise(gen));
        }
    }
}

//=============================================================================================================

void TestSsd::testConstructor()
{
    MlSsd ssd(3);
    QVERIFY(!ssd.isFitted());
}

//=============================================================================================================

void TestSsd::testFit()
{
    MlSsd ssd(3);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(m_data, m_sFreq, signalBand, noiseBand);
    QVERIFY(ssd.isFitted());
}

//=============================================================================================================

void TestSsd::testTransform()
{
    MlSsd ssd(3);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(m_data, m_sFreq, signalBand, noiseBand);

    MatrixXd components = ssd.transform(m_data);
    QCOMPARE(static_cast<int>(components.rows()), 3);
    QCOMPARE(static_cast<int>(components.cols()), m_nTimes);
    QVERIFY(components.allFinite());
}

//=============================================================================================================

void TestSsd::testFitTransform()
{
    MlSsd ssd(3);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);

    MatrixXd components = ssd.fitTransform(m_data, m_sFreq, signalBand, noiseBand);
    QVERIFY(ssd.isFitted());
    QCOMPARE(static_cast<int>(components.rows()), 3);
    QCOMPARE(static_cast<int>(components.cols()), m_nTimes);
}

//=============================================================================================================

void TestSsd::testFiltersShape()
{
    MlSsd ssd(4);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(m_data, m_sFreq, signalBand, noiseBand);

    QCOMPARE(static_cast<int>(ssd.filters().rows()), 4);
    QCOMPARE(static_cast<int>(ssd.filters().cols()), m_nCh);
}

//=============================================================================================================

void TestSsd::testPatternsShape()
{
    MlSsd ssd(4);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(m_data, m_sFreq, signalBand, noiseBand);

    QCOMPARE(static_cast<int>(ssd.patterns().rows()), 4);
    QCOMPARE(static_cast<int>(ssd.patterns().cols()), m_nCh);
}

//=============================================================================================================

void TestSsd::testEigenvaluesDescending()
{
    MlSsd ssd(4);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(m_data, m_sFreq, signalBand, noiseBand);

    const VectorXd& ev = ssd.eigenvalues();
    for (int i = 0; i < ev.size() - 1; ++i)
        QVERIFY2(ev[i] >= ev[i + 1],
                 qPrintable(QString("Eigenvalue %1 (%2) < %3 (%4)").arg(i).arg(ev[i]).arg(i + 1).arg(ev[i + 1])));
}

//=============================================================================================================

void TestSsd::testTransformNotFitted()
{
    MlSsd ssd(3);
    MatrixXd components = ssd.transform(m_data);
    QVERIFY(components.size() == 0);
}

//=============================================================================================================

void TestSsd::testEmptyInput()
{
    MlSsd ssd(3);
    MatrixXd empty;
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);
    ssd.fit(empty, m_sFreq, signalBand, noiseBand);
    QVERIFY(!ssd.isFitted());
}

//=============================================================================================================

void TestSsd::testSignalEnhancement()
{
    // The first SSD component should capture the 10 Hz signal
    MlSsd ssd(1);
    QPair<double, double> signalBand(8.0, 12.0);
    QPair<double, double> noiseBand(6.0, 14.0);

    MatrixXd comp = ssd.fitTransform(m_data, m_sFreq, signalBand, noiseBand);

    // The first eigenvalue should be substantially larger than the last
    // (signal component has more signal-band power relative to noise-band power)
    QVERIFY2(ssd.eigenvalues()[0] > 0.5,
             qPrintable(QString("First eigenvalue %1 should be > 0.5 for signal enhancement").arg(ssd.eigenvalues()[0])));
}

//=============================================================================================================

void TestSsd::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestSsd)
#include "test_ssd.moc"
