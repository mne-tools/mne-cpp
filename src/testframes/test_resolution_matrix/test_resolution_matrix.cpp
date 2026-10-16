//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_resolution_matrix.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
    void testMetricsMatchMnePython();
    void testDimensionMismatch();
    void testOutOfRange();
    void cleanupTestCase();

private:
    MatrixXd m_kernel;    // n_sources × n_channels
    MatrixXd m_leadField; // n_channels × n_sources
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

void TestResolutionMatrix::testMetricsMatchMnePython()
{
    // Reference values produced by mne.minimum_norm.resolution_metrics (mne 1.11), via its _localisation_error and
    // _spatial_extent with the source locations below in metres (mne reports cm):
    //   R[i, j] = sin(1.3 i + 0.7 j) + 2 (i == j); R[4, 1] = 3.5
    //   pos[k] = 0.05 * (cos(0.9 k), sin(0.9 k), 0.1 k)
    // CTF metrics are the PSF metrics of R^T.
    const int n = 6;
    MatrixXd R(n, n);
    MatrixX3d posCm(n, 3);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j)
            R(i, j) = std::sin(1.3 * i + 0.7 * j) + (i == j ? 2.0 : 0.0);
        posCm.row(i) << 5.0 * std::cos(0.9 * i), 5.0 * std::sin(0.9 * i), 0.5 * i;
    }
    R(4, 1) = 3.5;

    const VectorXd psfPeak = (VectorXd(n) << 0, 9.87185935378, 0, 0, 0, 0).finished();
    const VectorXd psfSd = (VectorXd(n) << 4.97453964319, 7.63115913768, 6.34498537097, 4.8744455778, 3.33926452767, 6.10541101403).finished();
    const VectorXd ctfPeak = (VectorXd(n) << 0, 0, 0, 0, 9.87185935378, 0).finished();
    const VectorXd ctfSd = (VectorXd(n) << 5.02001029439, 4.08609119997, 4.70357224416, 5.35444309551, 7.47929357294, 6.7224570419).finished();

    QVERIFY((InvResolutionMatrix::peakLocalisationError(R, posCm) - psfPeak).cwiseAbs().maxCoeff() < 1e-9);
    QVERIFY((InvResolutionMatrix::peakLocalisationError(R.transpose(), posCm) - ctfPeak).cwiseAbs().maxCoeff() < 1e-9);
    const VectorXd sd = InvResolutionMatrix::spatialSpread(R, posCm);
    QVERIFY2((sd - psfSd).cwiseAbs().maxCoeff() < 1e-9, qPrintable(QString::number(sd[0], 'g', 12)));
    QVERIFY((InvResolutionMatrix::spatialSpread(R.transpose(), posCm) - ctfSd).cwiseAbs().maxCoeff() < 1e-9);
}

//=============================================================================================================

void TestResolutionMatrix::testDimensionMismatch()
{
    MatrixXd K(3, 5);
    MatrixXd L(4, 3); // Mismatch: K.cols()=5 != L.rows()=4

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
