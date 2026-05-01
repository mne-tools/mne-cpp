//=============================================================================================================
/**
 * @file     test_resolution_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for InvResolutionMatrix (PSF/CTF analysis).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <inv/inv_resolution_matrix.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/LU>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================

class TestResolutionMatrix : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testComputeDimensions();
    void testIdentityInverse();
    void testPsfDimensions();
    void testCtfDimensions();
    void testPsfPeakAtSource();
    void testCtfPeakAtSource();
    void testPsfsBatch();
    void testCtfsBatch();
    void testSpatialSpread();
    void testPeakLocalisationError();
    void testDimensionMismatch();
    void testOutOfRange();
    void cleanupTestCase();

private:
    MatrixXd m_kernel;      // n_sources × n_channels
    MatrixXd m_leadField;   // n_channels × n_sources
    MatrixXd m_resolution;
    int m_nSrc;
    int m_nCh;
};

//=============================================================================================================

void TestResolutionMatrix::initTestCase()
{
    m_nSrc = 20;
    m_nCh = 10;

    // Create a simple lead field and pseudo-inverse as kernel
    m_leadField = MatrixXd::Random(m_nCh, m_nSrc);

    // Kernel = pseudoinverse of lead field: (nSrc × nCh)
    m_kernel = m_leadField.bdcSvd<ComputeThinU | ComputeThinV>().solve(
        MatrixXd::Identity(m_nCh, m_nCh));

    m_resolution = InvResolutionMatrix::compute(m_kernel, m_leadField);
}

//=============================================================================================================

void TestResolutionMatrix::testComputeDimensions()
{
    QCOMPARE(static_cast<int>(m_resolution.rows()), m_nSrc);
    QCOMPARE(static_cast<int>(m_resolution.cols()), m_nSrc);
}

//=============================================================================================================

void TestResolutionMatrix::testIdentityInverse()
{
    // When the kernel is the exact inverse and nCh == nSrc,
    // the resolution matrix should be identity
    int n = 5;
    MatrixXd L = MatrixXd::Random(n, n);
    MatrixXd K = L.inverse();
    MatrixXd R = InvResolutionMatrix::compute(K, L);

    double diff = (R - MatrixXd::Identity(n, n)).norm();
    QVERIFY2(diff < 1e-8, qPrintable(QString("Exact inverse R != I, diff = %1").arg(diff)));
}

//=============================================================================================================

void TestResolutionMatrix::testPsfDimensions()
{
    VectorXd psf = InvResolutionMatrix::getPsf(m_resolution, 0);
    QCOMPARE(static_cast<int>(psf.size()), m_nSrc);
}

//=============================================================================================================

void TestResolutionMatrix::testCtfDimensions()
{
    VectorXd ctf = InvResolutionMatrix::getCtf(m_resolution, 0);
    QCOMPARE(static_cast<int>(ctf.size()), m_nSrc);
}

//=============================================================================================================

void TestResolutionMatrix::testPsfPeakAtSource()
{
    // For a reasonable inverse, the PSF should have its peak at or near the source
    // Use the identity case for a definitive test
    int n = 5;
    MatrixXd L = MatrixXd::Random(n, n);
    MatrixXd K = L.inverse();
    MatrixXd R = InvResolutionMatrix::compute(K, L);

    for (int s = 0; s < n; ++s) {
        VectorXd psf = InvResolutionMatrix::getPsf(R, s);
        Index peakIdx = 0;
        psf.cwiseAbs().maxCoeff(&peakIdx);
        QCOMPARE(static_cast<int>(peakIdx), s);
    }
}

//=============================================================================================================

void TestResolutionMatrix::testCtfPeakAtSource()
{
    int n = 5;
    MatrixXd L = MatrixXd::Random(n, n);
    MatrixXd K = L.inverse();
    MatrixXd R = InvResolutionMatrix::compute(K, L);

    for (int s = 0; s < n; ++s) {
        VectorXd ctf = InvResolutionMatrix::getCtf(R, s);
        Index peakIdx = 0;
        ctf.cwiseAbs().maxCoeff(&peakIdx);
        QCOMPARE(static_cast<int>(peakIdx), s);
    }
}

//=============================================================================================================

void TestResolutionMatrix::testPsfsBatch()
{
    VectorXi idx(3);
    idx << 0, 5, 10;

    MatrixXd psfs = InvResolutionMatrix::getPsfs(m_resolution, idx);
    QCOMPARE(static_cast<int>(psfs.rows()), m_nSrc);
    QCOMPARE(static_cast<int>(psfs.cols()), 3);

    // Each column should match individual getPsf
    for (int i = 0; i < 3; ++i) {
        VectorXd expected = InvResolutionMatrix::getPsf(m_resolution, idx[i]);
        double diff = (psfs.col(i) - expected).norm();
        QVERIFY2(diff < 1e-12, "Batch PSF doesn't match individual.");
    }
}

//=============================================================================================================

void TestResolutionMatrix::testCtfsBatch()
{
    VectorXi idx(2);
    idx << 3, 7;

    MatrixXd ctfs = InvResolutionMatrix::getCtfs(m_resolution, idx);
    QCOMPARE(static_cast<int>(ctfs.rows()), 2);
    QCOMPARE(static_cast<int>(ctfs.cols()), m_nSrc);
}

//=============================================================================================================

void TestResolutionMatrix::testSpatialSpread()
{
    // For identity resolution, spatial spread should be zero
    int n = 5;
    MatrixXd R = MatrixXd::Identity(n, n);
    MatrixX3d pos = MatrixX3d::Random(n, 3);

    VectorXd spread = InvResolutionMatrix::spatialSpread(R, pos);
    QCOMPARE(static_cast<int>(spread.size()), n);

    for (int i = 0; i < n; ++i)
        QVERIFY2(spread[i] < 1e-10, "Identity resolution should have zero spread.");
}

//=============================================================================================================

void TestResolutionMatrix::testPeakLocalisationError()
{
    // For identity resolution, peak localisation error should be zero
    int n = 5;
    MatrixXd R = MatrixXd::Identity(n, n);
    MatrixX3d pos = MatrixX3d::Random(n, 3);

    VectorXd ple = InvResolutionMatrix::peakLocalisationError(R, pos);
    QCOMPARE(static_cast<int>(ple.size()), n);

    for (int i = 0; i < n; ++i)
        QVERIFY2(ple[i] < 1e-10, "Identity resolution should have zero PLE.");
}

//=============================================================================================================

void TestResolutionMatrix::testDimensionMismatch()
{
    MatrixXd K(3, 5);
    MatrixXd L(4, 3);  // Mismatch: K.cols()=5 != L.rows()=4

    MatrixXd R = InvResolutionMatrix::compute(K, L);
    QVERIFY(R.size() == 0);
}

//=============================================================================================================

void TestResolutionMatrix::testOutOfRange()
{
    VectorXd psf = InvResolutionMatrix::getPsf(m_resolution, -1);
    QVERIFY(psf.size() == 0);

    VectorXd ctf = InvResolutionMatrix::getCtf(m_resolution, 9999);
    QVERIFY(ctf.size() == 0);
}

//=============================================================================================================

void TestResolutionMatrix::cleanupTestCase()
{
}

QTEST_GUILESS_MAIN(TestResolutionMatrix)
#include "test_resolution_matrix.moc"
