//=============================================================================================================
/**
 * @file     test_picard_ica.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for PicardIca.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/picard_ica.h>
#include <dsp/ica.h>

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

class TestPicardIca : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testBasicRun();
    void testDimensions();
    void testConvergence();
    void testSourceSeparation();
    void testReducedComponents();
    void testMixingUnmixingInverse();
    void testMeanRestoration();
    void testExcludeViaICA();
    void testInsufficientData();
    void testSingleComponent();
    void cleanupTestCase();

private:
    MatrixXd m_mixedData;
    MatrixXd m_sources;
    int m_nCh;
    int m_nSamples;
};

//=============================================================================================================

void TestPicardIca::initTestCase()
{
    m_nCh = 5;
    m_nSamples = 1000;

    std::mt19937 gen(42);
    std::normal_distribution<double> noise(0.0, 0.01);

    // Create 3 independent source signals
    m_sources = MatrixXd(3, m_nSamples);
    for (int t = 0; t < m_nSamples; ++t) {
        double time = static_cast<double>(t) / 250.0;
        m_sources(0, t) = std::sin(2.0 * M_PI * 5.0 * time);                // 5 Hz sine
        m_sources(1, t) = (std::fmod(time * 3.0, 1.0) > 0.5) ? 1.0 : -1.0; // square wave
        m_sources(2, t) = std::fmod(time * 7.0, 1.0) - 0.5;                 // sawtooth
    }

    // Random mixing matrix (5 x 3)
    MatrixXd mixing(m_nCh, 3);
    for (int i = 0; i < m_nCh; ++i)
        for (int j = 0; j < 3; ++j)
            mixing(i, j) = std::normal_distribution<double>(0.0, 1.0)(gen);

    m_mixedData = mixing * m_sources;

    // Add small noise
    for (int i = 0; i < m_nCh; ++i)
        for (int t = 0; t < m_nSamples; ++t)
            m_mixedData(i, t) += noise(gen);
}

//=============================================================================================================

void TestPicardIca::testBasicRun()
{
    IcaResult result = PicardIca::run(m_mixedData, 3);
    QVERIFY(result.matSources.rows() == 3);
    QVERIFY(result.matSources.cols() == m_nSamples);
}

//=============================================================================================================

void TestPicardIca::testDimensions()
{
    IcaResult result = PicardIca::run(m_mixedData, 3);
    QCOMPARE(static_cast<int>(result.matUnmixing.rows()), 3);
    QCOMPARE(static_cast<int>(result.matUnmixing.cols()), m_nCh);
    QCOMPARE(static_cast<int>(result.matMixing.rows()), m_nCh);
    QCOMPARE(static_cast<int>(result.matMixing.cols()), 3);
    QCOMPARE(static_cast<int>(result.vecMean.size()), m_nCh);
}

//=============================================================================================================

void TestPicardIca::testConvergence()
{
    IcaResult result = PicardIca::run(m_mixedData, 3, 1000, 1e-4);
    QVERIFY(result.bConverged);
}

//=============================================================================================================

void TestPicardIca::testSourceSeparation()
{
    IcaResult result = PicardIca::run(m_mixedData, 3);

    // Each recovered source should correlate strongly with exactly one original source
    for (int k = 0; k < 3; ++k) {
        VectorXd recovered = result.matSources.row(k).transpose();
        double maxCorr = 0.0;
        for (int j = 0; j < 3; ++j) {
            VectorXd orig = m_sources.row(j).transpose();
            // Compute absolute correlation
            double r = std::abs(recovered.dot(orig)) /
                       (recovered.norm() * orig.norm());
            maxCorr = std::max(maxCorr, r);
        }
        QVERIFY2(maxCorr > 0.8,
                 qPrintable(QString("Component %1 max correlation %2 < 0.8").arg(k).arg(maxCorr)));
    }
}

//=============================================================================================================

void TestPicardIca::testReducedComponents()
{
    IcaResult result = PicardIca::run(m_mixedData, 2);
    QCOMPARE(static_cast<int>(result.matSources.rows()), 2);
    QCOMPARE(static_cast<int>(result.matUnmixing.rows()), 2);
    QCOMPARE(static_cast<int>(result.matMixing.cols()), 2);
}

//=============================================================================================================

void TestPicardIca::testMixingUnmixingInverse()
{
    IcaResult result = PicardIca::run(m_mixedData, 3);

    // W * A ≈ I (up to permutation and sign)
    MatrixXd WA = result.matUnmixing * result.matMixing;
    // Each row of WA should have exactly one dominant element
    for (int i = 0; i < 3; ++i) {
        double maxVal = WA.row(i).cwiseAbs().maxCoeff();
        QVERIFY2(maxVal > 0.5, qPrintable(QString("Row %1 max %2 < 0.5").arg(i).arg(maxVal)));
    }
}

//=============================================================================================================

void TestPicardIca::testMeanRestoration()
{
    // Add a known mean
    MatrixXd dataWithMean = m_mixedData;
    VectorXd addedMean = VectorXd::Ones(m_nCh) * 5.0;
    dataWithMean.colwise() += addedMean;

    IcaResult result = PicardIca::run(dataWithMean, 3);

    // The mean should be close to addedMean + original mean
    for (int i = 0; i < m_nCh; ++i) {
        double expected = m_mixedData.row(i).mean() + 5.0;
        QVERIFY2(std::abs(result.vecMean[i] - expected) < 0.1,
                 qPrintable(QString("Mean ch %1: %2 vs %3").arg(i).arg(result.vecMean[i]).arg(expected)));
    }
}

//=============================================================================================================

void TestPicardIca::testExcludeViaICA()
{
    IcaResult result = PicardIca::run(m_mixedData, 3);

    // Exclude component 0 using ICA's excludeComponents
    QVector<int> exclude = {0};
    MatrixXd cleaned = ICA::excludeComponents(m_mixedData, result, exclude);

    QCOMPARE(static_cast<int>(cleaned.rows()), m_nCh);
    QCOMPARE(static_cast<int>(cleaned.cols()), m_nSamples);

    // Cleaned data should differ from original
    double diff = (cleaned - m_mixedData).norm();
    QVERIFY(diff > 0.1);
}

//=============================================================================================================

void TestPicardIca::testInsufficientData()
{
    MatrixXd small(1, 5);
    small.setRandom();
    IcaResult result = PicardIca::run(small);
    QVERIFY(result.matSources.size() == 0);
}

//=============================================================================================================

void TestPicardIca::testSingleComponent()
{
    // Extract all channels as components (default nComponents = -1)
    IcaResult result = PicardIca::run(m_mixedData);
    QCOMPARE(static_cast<int>(result.matSources.rows()), m_nCh);
}

//=============================================================================================================

void TestPicardIca::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestPicardIca)
#include "test_picard_ica.moc"
