//=============================================================================================================
/**
 * @file     test_spoc.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for MlSpoc (Source Power Comodulation).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <ml/ml_spoc.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>
#include <QList>

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

class TestSpoc : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testConstructor();
    void testDefaultComponents();
    void testFit();
    void testTransform();
    void testFitTransform();
    void testCorrelationWithTarget();
    void testTransformNotFitted();
    void testEmptyInput();
    void testFiltersShape();
    void testPatternsShape();
    void cleanupTestCase();

private:
    QList<MatrixXd> m_epochs;
    VectorXd m_target;
    int m_nCh;
    int m_nTimes;
    int m_nEpochs;
};

//=============================================================================================================

void TestSpoc::initTestCase()
{
    m_nCh = 8;
    m_nTimes = 100;
    m_nEpochs = 40;

    std::mt19937 gen(42);
    std::normal_distribution<double> noise(0.0, 1.0);

    // Create a target variable
    m_target.resize(m_nEpochs);
    for (int e = 0; e < m_nEpochs; ++e)
        m_target[e] = noise(gen);

    // Create epochs where one spatial pattern's power covaries with target
    VectorXd pattern = VectorXd::Random(m_nCh);
    pattern.normalize();

    for (int e = 0; e < m_nEpochs; ++e) {
        MatrixXd epoch = MatrixXd::Zero(m_nCh, m_nTimes);

        // Add noise
        for (int ch = 0; ch < m_nCh; ++ch)
            for (int t = 0; t < m_nTimes; ++t)
                epoch(ch, t) = noise(gen);

        // Add a source whose amplitude covaries with target
        double amplitude = 3.0 + 2.0 * m_target[e];  // strong covariation
        for (int t = 0; t < m_nTimes; ++t) {
            double source = amplitude * std::sin(2.0 * M_PI * 10.0 * t / 100.0);
            epoch.col(t) += pattern * source;
        }

        m_epochs.append(epoch);
    }
}

//=============================================================================================================

void TestSpoc::testConstructor()
{
    MlSpoc spoc(3);
    QVERIFY(!spoc.isFitted());
}

//=============================================================================================================

void TestSpoc::testDefaultComponents()
{
    MlSpoc spoc;  // default = 4 components
    spoc.fit(m_epochs, m_target);
    QCOMPARE(static_cast<int>(spoc.filters().rows()), 4);
}

//=============================================================================================================

void TestSpoc::testFit()
{
    MlSpoc spoc(3);
    spoc.fit(m_epochs, m_target);
    QVERIFY(spoc.isFitted());
    QCOMPARE(static_cast<int>(spoc.eigenvalues().size()), 3);
}

//=============================================================================================================

void TestSpoc::testTransform()
{
    MlSpoc spoc(3);
    spoc.fit(m_epochs, m_target);

    MatrixXd features = spoc.transform(m_epochs);
    QCOMPARE(static_cast<int>(features.rows()), m_nEpochs);
    QCOMPARE(static_cast<int>(features.cols()), 3);
    QVERIFY(features.allFinite());
}

//=============================================================================================================

void TestSpoc::testFitTransform()
{
    MlSpoc spoc(3);
    MatrixXd features = spoc.fitTransform(m_epochs, m_target);
    QVERIFY(spoc.isFitted());
    QCOMPARE(static_cast<int>(features.rows()), m_nEpochs);
    QCOMPARE(static_cast<int>(features.cols()), 3);
}

//=============================================================================================================

void TestSpoc::testCorrelationWithTarget()
{
    MlSpoc spoc(1);
    MatrixXd features = spoc.fitTransform(m_epochs, m_target);

    // The first SPoC component should correlate with the target
    VectorXd feat = features.col(0);
    double fMean = feat.mean();
    double tMean = m_target.mean();

    double cov = 0.0, varF = 0.0, varT = 0.0;
    for (int i = 0; i < m_nEpochs; ++i) {
        double df = feat[i] - fMean;
        double dt = m_target[i] - tMean;
        cov += df * dt;
        varF += df * df;
        varT += dt * dt;
    }

    double corr = cov / (std::sqrt(varF) * std::sqrt(varT));
    QVERIFY2(std::abs(corr) > 0.3,
             qPrintable(QString("SPoC correlation with target = %1, expected > 0.3").arg(corr)));
}

//=============================================================================================================

void TestSpoc::testTransformNotFitted()
{
    MlSpoc spoc(3);
    MatrixXd features = spoc.transform(m_epochs);
    QVERIFY(features.size() == 0);
}

//=============================================================================================================

void TestSpoc::testEmptyInput()
{
    MlSpoc spoc(3);
    QList<MatrixXd> empty;
    VectorXd emptyTarget;
    spoc.fit(empty, emptyTarget);
    QVERIFY(!spoc.isFitted());
}

//=============================================================================================================

void TestSpoc::testFiltersShape()
{
    MlSpoc spoc(4);
    spoc.fit(m_epochs, m_target);

    QCOMPARE(static_cast<int>(spoc.filters().rows()), 4);
    QCOMPARE(static_cast<int>(spoc.filters().cols()), m_nCh);
}

//=============================================================================================================

void TestSpoc::testPatternsShape()
{
    MlSpoc spoc(4);
    spoc.fit(m_epochs, m_target);

    QCOMPARE(static_cast<int>(spoc.patterns().rows()), 4);
    QCOMPARE(static_cast<int>(spoc.patterns().cols()), m_nCh);
}

//=============================================================================================================

void TestSpoc::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestSpoc)
#include "test_spoc.moc"
