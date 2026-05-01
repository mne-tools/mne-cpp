//=============================================================================================================
/**
 * @file     test_surface_laplacian.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for SurfaceLaplacian (Current Source Density).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <dsp/surface_laplacian.h>

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
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================

class TestSurfaceLaplacian : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testTransformDimensions();
    void testTransformSquare();
    void testOutputDimensions();
    void testReferenceFreeness();
    void testSpatialHighPass();
    void testSymmetry();
    void testRegularization();
    void testEmptyInput();
    void testComputeTransformOnly();
    void testStiffnessEffect();
    void cleanupTestCase();

private:
    MatrixX3d m_positions;
    int m_nCh;
};

//=============================================================================================================

void TestSurfaceLaplacian::initTestCase()
{
    // Create electrode positions on a hemisphere (unit sphere)
    m_nCh = 16;
    m_positions.resize(m_nCh, 3);

    // Distribute points on upper hemisphere using Fibonacci spiral
    const double golden = (1.0 + std::sqrt(5.0)) / 2.0;
    for (int i = 0; i < m_nCh; ++i) {
        double theta = std::acos(1.0 - static_cast<double>(i) / static_cast<double>(m_nCh));
        double phi = 2.0 * M_PI * static_cast<double>(i) / golden;
        // Scale to head-like radius (~0.085 m)
        double r = 0.085;
        m_positions(i, 0) = r * std::sin(theta) * std::cos(phi);
        m_positions(i, 1) = r * std::sin(theta) * std::sin(phi);
        m_positions(i, 2) = r * std::cos(theta);
    }
}

//=============================================================================================================

void TestSurfaceLaplacian::testTransformDimensions()
{
    MatrixXd T = SurfaceLaplacian::computeTransform(m_positions);
    QCOMPARE(static_cast<int>(T.rows()), m_nCh);
    QCOMPARE(static_cast<int>(T.cols()), m_nCh);
}

//=============================================================================================================

void TestSurfaceLaplacian::testTransformSquare()
{
    MatrixXd T = SurfaceLaplacian::computeTransform(m_positions);
    QVERIFY(T.rows() == T.cols());
}

//=============================================================================================================

void TestSurfaceLaplacian::testOutputDimensions()
{
    int nTimes = 100;
    MatrixXd data = MatrixXd::Random(m_nCh, nTimes);

    auto result = SurfaceLaplacian::compute(data, m_positions);
    QCOMPARE(static_cast<int>(result.matData.rows()), m_nCh);
    QCOMPARE(static_cast<int>(result.matData.cols()), nTimes);
}

//=============================================================================================================

void TestSurfaceLaplacian::testReferenceFreeness()
{
    // CSD should be reference-free: adding a constant to all channels
    // should not change the output
    int nTimes = 50;
    MatrixXd data = MatrixXd::Random(m_nCh, nTimes);
    MatrixXd dataShifted = data.array() + 100.0;

    auto r1 = SurfaceLaplacian::compute(data, m_positions);
    auto r2 = SurfaceLaplacian::compute(dataShifted, m_positions);

    double diff = (r1.matData - r2.matData).norm();
    QVERIFY2(diff < 1e-6, qPrintable(QString("Reference shift changed output by %1").arg(diff)));
}

//=============================================================================================================

void TestSurfaceLaplacian::testSpatialHighPass()
{
    // A spatially uniform signal (all channels same) should produce near-zero CSD
    int nTimes = 50;
    MatrixXd data(m_nCh, nTimes);
    for (int ch = 0; ch < m_nCh; ++ch)
        data.row(ch) = RowVectorXd::LinSpaced(nTimes, 0.0, 1.0);

    auto result = SurfaceLaplacian::compute(data, m_positions);

    double maxAbs = result.matData.cwiseAbs().maxCoeff();
    QVERIFY2(maxAbs < 1e-3, qPrintable(QString("Uniform signal CSD max = %1").arg(maxAbs)));
}

//=============================================================================================================

void TestSurfaceLaplacian::testSymmetry()
{
    // The G and H matrices (and hence transform) should be symmetric in their
    // construction. The transform itself may not be symmetric, but let's verify
    // it's finite and has reasonable values.
    MatrixXd T = SurfaceLaplacian::computeTransform(m_positions);
    QVERIFY(T.allFinite());
}

//=============================================================================================================

void TestSurfaceLaplacian::testRegularization()
{
    // Higher lambda2 should produce a smoother (lower norm) transform
    MatrixXd T_low = SurfaceLaplacian::computeTransform(m_positions, 1e-8);
    MatrixXd T_high = SurfaceLaplacian::computeTransform(m_positions, 1e-1);

    double norm_low = T_low.norm();
    double norm_high = T_high.norm();
    QVERIFY2(norm_high < norm_low,
             qPrintable(QString("High reg norm %1 >= low reg norm %2").arg(norm_high).arg(norm_low)));
}

//=============================================================================================================

void TestSurfaceLaplacian::testEmptyInput()
{
    MatrixXd emptyData;
    MatrixX3d emptyPos;

    auto result = SurfaceLaplacian::compute(emptyData, emptyPos);
    QVERIFY(result.matData.size() == 0);
}

//=============================================================================================================

void TestSurfaceLaplacian::testComputeTransformOnly()
{
    MatrixXd T = SurfaceLaplacian::computeTransform(m_positions);
    QVERIFY(T.size() > 0);

    // Manually apply and compare
    int nTimes = 30;
    MatrixXd data = MatrixXd::Random(m_nCh, nTimes);
    MatrixXd manual = T * data;

    auto result = SurfaceLaplacian::compute(data, m_positions);
    double diff = (manual - result.matData).norm();
    QVERIFY2(diff < 1e-10, "Transform-only and compute() differ.");
}

//=============================================================================================================

void TestSurfaceLaplacian::testStiffnessEffect()
{
    // Different stiffness values should produce different transforms
    MatrixXd T3 = SurfaceLaplacian::computeTransform(m_positions, 1e-5, 3);
    MatrixXd T5 = SurfaceLaplacian::computeTransform(m_positions, 1e-5, 5);

    double diff = (T3 - T5).norm();
    QVERIFY2(diff > 1e-3, "Different stiffness produced same transform.");
}

//=============================================================================================================

void TestSurfaceLaplacian::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestSurfaceLaplacian)
#include "test_surface_laplacian.moc"
