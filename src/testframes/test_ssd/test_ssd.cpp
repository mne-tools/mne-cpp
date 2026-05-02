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
 * @brief    Tests for Spatio-Spectral Decomposition (SSD) via Skigen.
 */

#include <Skigen/Decomposition>

#include <Eigen/Core>
#include <QtTest>
#include <QObject>
#include <cmath>
#include <random>

using namespace Eigen;

class TestSsd : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFit();
    void testTransform();
    void testFitTransform();
    void testFiltersShape();
    void testPatternsShape();
    void testEigenvaluesDescending();
    void testNotFitted();
    void testEmptyInput();
    void testSignalEnhancement();

private:
    MatrixXd m_data;
    double m_sFreq;
    int m_nCh;
    int m_nTimes;
};

void TestSsd::initTestCase()
{
    m_nCh = 8;
    m_sFreq = 250.0;
    m_nTimes = 2500;

    std::mt19937 gen(42);
    std::normal_distribution<double> noise(0.0, 1.0);

    m_data = MatrixXd(m_nCh, m_nTimes);
    VectorXd pattern = VectorXd::Random(m_nCh);
    pattern.normalize();

    for (int ch = 0; ch < m_nCh; ++ch) {
        for (int t = 0; t < m_nTimes; ++t) {
            double time = static_cast<double>(t) / m_sFreq;
            m_data(ch, t) = noise(gen) * 0.5;
            m_data(ch, t) += pattern[ch] * 3.0 * std::sin(2.0 * M_PI * 10.0 * time);
            m_data(ch, t) += noise(gen) * 0.3 * std::sin(2.0 * M_PI * 25.0 * time + noise(gen));
        }
    }
}

void TestSsd::testFit()
{
    Skigen::SSD<double> ssd(3);
    ssd.fit(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);
    QVERIFY(true);  // No exception = success
}

void TestSsd::testTransform()
{
    Skigen::SSD<double> ssd(3);
    ssd.fit(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);

    MatrixXd components = ssd.transform(m_data);
    QCOMPARE(static_cast<int>(components.rows()), 3);
    QCOMPARE(static_cast<int>(components.cols()), m_nTimes);
    QVERIFY(components.allFinite());
}

void TestSsd::testFitTransform()
{
    Skigen::SSD<double> ssd(3);
    MatrixXd components = ssd.fit_transform(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);
    QCOMPARE(static_cast<int>(components.rows()), 3);
    QCOMPARE(static_cast<int>(components.cols()), m_nTimes);
}

void TestSsd::testFiltersShape()
{
    Skigen::SSD<double> ssd(4);
    ssd.fit(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);
    QCOMPARE(static_cast<int>(ssd.filters().rows()), 4);
    QCOMPARE(static_cast<int>(ssd.filters().cols()), m_nCh);
}

void TestSsd::testPatternsShape()
{
    Skigen::SSD<double> ssd(4);
    ssd.fit(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);
    QCOMPARE(static_cast<int>(ssd.patterns().rows()), 4);
    QCOMPARE(static_cast<int>(ssd.patterns().cols()), m_nCh);
}

void TestSsd::testEigenvaluesDescending()
{
    Skigen::SSD<double> ssd(4);
    ssd.fit(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0);

    const VectorXd& ev = ssd.eigenvalues();
    for (int i = 0; i < ev.size() - 1; ++i)
        QVERIFY2(ev[i] >= ev[i + 1],
                 qPrintable(QString("Eigenvalue %1 (%2) < %3 (%4)").arg(i).arg(ev[i]).arg(i + 1).arg(ev[i + 1])));
}

void TestSsd::testNotFitted()
{
    Skigen::SSD<double> ssd(3);
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, static_cast<void>(ssd.transform(m_data)));
}

void TestSsd::testEmptyInput()
{
    Skigen::SSD<double> ssd(3);
    MatrixXd empty;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
        static_cast<void>(ssd.fit(empty, m_sFreq, 8.0, 12.0, 6.0, 14.0)));
}

void TestSsd::testSignalEnhancement()
{
    Skigen::SSD<double> ssd(1);
    static_cast<void>(ssd.fit_transform(m_data, m_sFreq, 8.0, 12.0, 6.0, 14.0));

    QVERIFY2(ssd.eigenvalues()[0] > 0.5,
             qPrintable(QString("First eigenvalue %1 should be > 0.5").arg(ssd.eigenvalues()[0])));
}

QTEST_GUILESS_MAIN(TestSsd)
#include "test_ssd.moc"
