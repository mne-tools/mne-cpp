//=============================================================================================================
/**
 * @file     test_utils_mnemath.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Tests for MNEMath utility class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <math/mnemath.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QObject>
#include <QDebug>
#include <QtTest>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestMNEMath
 *
 * @brief The TestMNEMath class provides comprehensive tests for MNEMath functions.
 */
class TestMNEMath : public QObject
{
    Q_OBJECT

public:
    TestMNEMath();

private slots:
    void initTestCase();

    // GCD tests (inspired by mne-python test_numerics.py pattern of known analytical results)
    void testGcd();

    // combine_xyz: combine 3D vector components
    void testCombineXyz();

    // Condition number and slope
    void testConditionNumber();
    void testConditionSlope();

    // Whitener
    void testGetWhitener();

    // Intersect
    void testIntersect();

    // issparse
    void testIsSparse();

    // make_block_diag
    void testMakeBlockDiag();

    // nchoose2
    void testNchoose2();

    // rank
    void testRank();

    // rescale (baseline correction) - inspired by mne-python test_numerics rescale tests
    void testRescaleMean();
    void testRescaleRatio();
    void testRescaleZscore();
    void testRescalePercent();
    void testRescaleInvalidMode();

    // sort
    void testSortAscending();
    void testSortDescending();
    void testSortWithMatrix();

    // sortrows
    void testSortrows();

    // log2
    void testLog2();

    // nchoose2 edge cases
    void testNchoose2EdgeCases();

    // histcounts
    void testHistcounts();

    // pinv (pseudo-inverse) - inspired by mne-python test_linalg.py _reg_pinv tests
    void testPinv();
    void testPinvRankDeficient();

    // compareTransformation
    void testCompareTransformation();

    void cleanupTestCase();

private:
    double m_dEpsilon;
};

//=============================================================================================================

TestMNEMath::TestMNEMath()
: m_dEpsilon(1e-6)
{
}

//=============================================================================================================

void TestMNEMath::initTestCase()
{
    qInstallMessageHandler(UTILSLIB::ApplicationLogger::customLogWriter);
    qInfo() << "TestMNEMath: Starting MNEMath unit tests";
}

//=============================================================================================================

void TestMNEMath::testGcd()
{
    // Standard cases
    QCOMPARE(MNEMath::gcd(12, 8), 4);
    QCOMPARE(MNEMath::gcd(100, 75), 25);
    QCOMPARE(MNEMath::gcd(17, 13), 1);    // coprime
    QCOMPARE(MNEMath::gcd(0, 5), 5);      // zero case
    QCOMPARE(MNEMath::gcd(7, 7), 7);      // equal
    QCOMPARE(MNEMath::gcd(1024, 512), 512);
    QCOMPARE(MNEMath::gcd(48, 18), 6);
}

//=============================================================================================================

void TestMNEMath::testCombineXyz()
{
    // Input: [x1, y1, z1, x2, y2, z2] => output: [x1^2+y1^2+z1^2, x2^2+y2^2+z2^2]
    VectorXd vec(6);
    vec << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0;

    VectorXd* result = MNEMath::combine_xyz(vec);
    QVERIFY(result != NULL);
    QCOMPARE(result->size(), (Eigen::Index)2);

    // First entry: 1^2 + 2^2 + 3^2 = 14
    QVERIFY(std::abs((*result)(0) - 14.0) < m_dEpsilon);
    // Second entry: 4^2 + 5^2 + 6^2 = 77
    QVERIFY(std::abs((*result)(1) - 77.0) < m_dEpsilon);

    delete result;

    // Edge case: input size not multiple of 3 should return NULL
    VectorXd badVec(5);
    badVec << 1.0, 2.0, 3.0, 4.0, 5.0;
    QVERIFY(MNEMath::combine_xyz(badVec) == NULL);
}

//=============================================================================================================

void TestMNEMath::testConditionNumber()
{
    // Well-conditioned identity matrix => condition number = 1
    MatrixXd I = MatrixXd::Identity(3, 3);
    VectorXd s;
    double c = MNEMath::getConditionNumber(I, s);
    QVERIFY(std::abs(c - 1.0) < m_dEpsilon);
    QCOMPARE(s.size(), (Eigen::Index)3);

    // Ill-conditioned matrix
    MatrixXd A(2, 2);
    A << 1.0, 0.0,
         0.0, 1e-10;
    double c2 = MNEMath::getConditionNumber(A, s);
    QVERIFY(c2 > 1e9);
}

//=============================================================================================================

void TestMNEMath::testConditionSlope()
{
    MatrixXd I = MatrixXd::Identity(3, 3);
    VectorXd s;
    double c = MNEMath::getConditionSlope(I, s);
    // For identity: max/mean = 1/1 = 1
    QVERIFY(std::abs(c - 1.0) < m_dEpsilon);
}

//=============================================================================================================

void TestMNEMath::testGetWhitener()
{
    // Create a simple 3x3 symmetric positive definite matrix (covariance)
    MatrixXd cov(3, 3);
    cov << 4.0, 2.0, 0.0,
           2.0, 3.0, 1.0,
           0.0, 1.0, 2.0;

    VectorXd eig;
    MatrixXd eigvec;

    // Test without PCA
    MatrixXd covCopy = cov;
    MNEMath::get_whitener(covCopy, false, QString("test"), eig, eigvec);
    QCOMPARE(eig.size(), (Eigen::Index)3);
    QCOMPARE(eigvec.rows(), (Eigen::Index)3);
    QCOMPARE(eigvec.cols(), (Eigen::Index)3);

    // Test with PCA
    covCopy = cov;
    MNEMath::get_whitener(covCopy, true, std::string("test"), eig, eigvec);
    QCOMPARE(eig.size(), (Eigen::Index)3);
    // PCA should keep rank(cov) eigenvectors
    QVERIFY(eigvec.rows() <= 3);
    QVERIFY(eigvec.rows() > 0);
}

//=============================================================================================================

void TestMNEMath::testIntersect()
{
    VectorXi v1(5), v2(4);
    v1 << 1, 3, 5, 7, 9;
    v2 << 3, 5, 8, 9;

    VectorXi idx_sel;
    VectorXi result = MNEMath::intersect(v1, v2, idx_sel);

    // Intersection should be {3, 5, 9}
    QCOMPARE(result.size(), (Eigen::Index)3);

    // Check all intersection values are present
    std::vector<int> res_vec(result.data(), result.data() + result.size());
    std::sort(res_vec.begin(), res_vec.end());
    QCOMPARE(res_vec[0], 3);
    QCOMPARE(res_vec[1], 5);
    QCOMPARE(res_vec[2], 9);

    // idx_sel should have same size
    QCOMPARE(idx_sel.size(), result.size());
}

//=============================================================================================================

void TestMNEMath::testIsSparse()
{
    // Sparse vector: more than half are zeros
    VectorXd sparse(10);
    sparse << 0, 0, 0, 0, 0, 0, 1.0, 0, 0, 0;
    QVERIFY(MNEMath::issparse(sparse));

    // Dense vector: fewer than half are zeros
    VectorXd dense(10);
    dense << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10;
    QVERIFY(!MNEMath::issparse(dense));

    // Exactly half zeros - should not be sparse (need > n/2)
    VectorXd half(4);
    half << 0, 0, 1, 1;
    QVERIFY(!MNEMath::issparse(half));
}

//=============================================================================================================

void TestMNEMath::testMakeBlockDiag()
{
    // Create a 2x4 matrix => 2 blocks of size 2x2
    MatrixXd A(2, 4);
    A << 1, 2, 3, 4,
         5, 6, 7, 8;

    SparseMatrix<double>* bd = MNEMath::make_block_diag(A, 2);
    QVERIFY(bd != NULL);

    // Should be 4x4 (2 blocks of 2x2 on the diagonal)
    QCOMPARE(bd->rows(), 4);
    QCOMPARE(bd->cols(), 4);

    // Check diagonal blocks
    QVERIFY(std::abs(bd->coeff(0, 0) - 1.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(0, 1) - 2.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(1, 0) - 5.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(1, 1) - 6.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(2, 2) - 3.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(2, 3) - 4.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(3, 2) - 7.0) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(3, 3) - 8.0) < m_dEpsilon);

    // Off-diagonal blocks should be zero
    QVERIFY(std::abs(bd->coeff(0, 2)) < m_dEpsilon);
    QVERIFY(std::abs(bd->coeff(2, 0)) < m_dEpsilon);

    delete bd;
}

//=============================================================================================================

void TestMNEMath::testNchoose2()
{
    // n choose 2 = n*(n-1)/2
    QCOMPARE(MNEMath::nchoose2(2), 1);
    QCOMPARE(MNEMath::nchoose2(4), 6);
    QCOMPARE(MNEMath::nchoose2(5), 10);
    QCOMPARE(MNEMath::nchoose2(10), 45);
    QCOMPARE(MNEMath::nchoose2(100), 4950);
}

//=============================================================================================================

void TestMNEMath::testRank()
{
    // Full rank 3x3
    MatrixXd A(3, 3);
    A << 1, 0, 0,
         0, 2, 0,
         0, 0, 3;
    QCOMPARE(MNEMath::rank(A), (qint32)3);

    // Rank deficient: last row is sum of first two
    MatrixXd B(3, 3);
    B << 1, 0, 0,
         0, 1, 0,
         1, 1, 0;
    QCOMPARE(MNEMath::rank(B), (qint32)2);

    // Zero matrix
    MatrixXd Z = MatrixXd::Zero(3, 3);
    QCOMPARE(MNEMath::rank(Z), (qint32)0);

    // Identity
    MatrixXd I = MatrixXd::Identity(5, 5);
    QCOMPARE(MNEMath::rank(I), (qint32)5);
}

//=============================================================================================================

void TestMNEMath::testRescaleMean()
{
    // Simple 2x10 data, baseline in first 3 columns
    MatrixXd data(2, 10);
    data.setOnes();
    data.row(0) *= 2.0;
    data.row(1) *= 4.0;

    RowVectorXf times(10);
    for (int i = 0; i < 10; ++i)
        times(i) = -0.3f + 0.1f * i;  // -0.3 to 0.6

    QPair<float, float> baseline(-0.3f, 0.0f);

    MatrixXd result = MNEMath::rescale(data, times, baseline, QString("mean"));

    // Mean baseline subtraction: data - mean(baseline) should give zeros for constant data
    for (int i = 0; i < result.cols(); ++i) {
        QVERIFY(std::abs(result(0, i)) < m_dEpsilon);
        QVERIFY(std::abs(result(1, i)) < m_dEpsilon);
    }
}

//=============================================================================================================

void TestMNEMath::testRescaleRatio()
{
    MatrixXd data(1, 10);
    data.setOnes();
    data *= 2.0;

    RowVectorXf times(10);
    for (int i = 0; i < 10; ++i)
        times(i) = -0.3f + 0.1f * i;

    std::pair<float, float> baseline(-0.3f, 0.0f);

    MatrixXd result = MNEMath::rescale(data, times, baseline, std::string("ratio"));

    // Constant data / mean(baseline) = 1.0 everywhere
    for (int i = 0; i < result.cols(); ++i) {
        QVERIFY(std::abs(result(0, i) - 1.0) < m_dEpsilon);
    }
}

//=============================================================================================================

void TestMNEMath::testRescaleZscore()
{
    // Create data with known baseline stats
    MatrixXd data(1, 10);
    for (int i = 0; i < 10; ++i)
        data(0, i) = 1.0 + 0.1 * i;

    RowVectorXf times(10);
    for (int i = 0; i < 10; ++i)
        times(i) = -0.3f + 0.1f * i;

    QPair<float, float> baseline(-0.3f, 0.0f);

    MatrixXd result = MNEMath::rescale(data, times, baseline, QString("zscore"));

    // Result should be z-scored; verify it's finite
    for (int i = 0; i < result.cols(); ++i) {
        QVERIFY(std::isfinite(result(0, i)));
    }
}

//=============================================================================================================

void TestMNEMath::testRescalePercent()
{
    MatrixXd data(1, 10);
    data.setOnes();
    data *= 4.0;
    // Make some columns different to see percent change
    data(0, 5) = 8.0;  // 100% increase from baseline mean of 4

    RowVectorXf times(10);
    for (int i = 0; i < 10; ++i)
        times(i) = -0.3f + 0.1f * i;

    QPair<float, float> baseline(-0.3f, 0.0f);

    MatrixXd result = MNEMath::rescale(data, times, baseline, QString("percent"));

    // For column 5: (8-4)/4 = 1.0
    QVERIFY(std::abs(result(0, 5) - 1.0) < m_dEpsilon);

    // For baseline columns: (4-4)/4 = 0.0
    QVERIFY(std::abs(result(0, 0)) < m_dEpsilon);
}

//=============================================================================================================

void TestMNEMath::testRescaleInvalidMode()
{
    MatrixXd data(1, 5);
    data.setOnes();
    RowVectorXf times(5);
    for (int i = 0; i < 5; ++i)
        times(i) = (float)i;
    QPair<float, float> baseline(0.0f, 2.0f);

    // Invalid mode should return input data unchanged
    MatrixXd result = MNEMath::rescale(data, times, baseline, QString("invalid_mode"));
    QVERIFY(result.isApprox(data));
}

//=============================================================================================================

void TestMNEMath::testSortAscending()
{
    VectorXd v(5);
    v << 5.0, 3.0, 1.0, 4.0, 2.0;

    VectorXi idx = MNEMath::sort<double>(v, false);  // ascending

    // After sort, v should be: 1, 2, 3, 4, 5
    QVERIFY(std::abs(v(0) - 1.0) < m_dEpsilon);
    QVERIFY(std::abs(v(1) - 2.0) < m_dEpsilon);
    QVERIFY(std::abs(v(2) - 3.0) < m_dEpsilon);
    QVERIFY(std::abs(v(3) - 4.0) < m_dEpsilon);
    QVERIFY(std::abs(v(4) - 5.0) < m_dEpsilon);

    // Original index of 1.0 was position 2
    QCOMPARE(idx(0), 2);
}

//=============================================================================================================

void TestMNEMath::testSortDescending()
{
    VectorXd v(5);
    v << 5.0, 3.0, 1.0, 4.0, 2.0;

    VectorXi idx = MNEMath::sort<double>(v, true);  // descending

    QVERIFY(std::abs(v(0) - 5.0) < m_dEpsilon);
    QVERIFY(std::abs(v(1) - 4.0) < m_dEpsilon);
    QVERIFY(std::abs(v(2) - 3.0) < m_dEpsilon);
    QVERIFY(std::abs(v(3) - 2.0) < m_dEpsilon);
    QVERIFY(std::abs(v(4) - 1.0) < m_dEpsilon);

    // Original index of 5.0 was position 0
    QCOMPARE(idx(0), 0);
}

//=============================================================================================================

void TestMNEMath::testSortWithMatrix()
{
    VectorXd v(3);
    v << 3.0, 1.0, 2.0;

    MatrixXd mat(2, 3);
    mat << 10, 20, 30,
           40, 50, 60;

    VectorXi idx = MNEMath::sort<double>(v, mat, false);  // ascending

    // After sort: v = [1, 2, 3], mat columns reordered accordingly
    QVERIFY(std::abs(v(0) - 1.0) < m_dEpsilon);
    QVERIFY(std::abs(v(1) - 2.0) < m_dEpsilon);
    QVERIFY(std::abs(v(2) - 3.0) < m_dEpsilon);

    // mat column for v=1.0 (originally col 1) should now be first
    QVERIFY(std::abs(mat(0, 0) - 20.0) < m_dEpsilon);
    QVERIFY(std::abs(mat(1, 0) - 50.0) < m_dEpsilon);
}

//=============================================================================================================

void TestMNEMath::testSortrows()
{
    std::vector<Triplet<double>> triplets;
    triplets.push_back(Triplet<double>(3, 1, 10.0));
    triplets.push_back(Triplet<double>(1, 3, 20.0));
    triplets.push_back(Triplet<double>(2, 2, 30.0));

    auto sorted = MNEMath::sortrows<double>(triplets, 0);  // sort by row

    QCOMPARE((int)sorted.size(), 3);
    QCOMPARE(sorted[0].row(), 1);
    QCOMPARE(sorted[1].row(), 2);
    QCOMPARE(sorted[2].row(), 3);

    // Sort by column
    auto sorted2 = MNEMath::sortrows<double>(triplets, 1);
    QCOMPARE(sorted2[0].col(), 1);
    QCOMPARE(sorted2[1].col(), 2);
    QCOMPARE(sorted2[2].col(), 3);
}

//=============================================================================================================

void TestMNEMath::testLog2()
{
    QVERIFY(std::abs(MNEMath::log2(1) - 0.0) < m_dEpsilon);
    QVERIFY(std::abs(MNEMath::log2(2) - 1.0) < m_dEpsilon);
    QVERIFY(std::abs(MNEMath::log2(4) - 2.0) < m_dEpsilon);
    QVERIFY(std::abs(MNEMath::log2(8) - 3.0) < m_dEpsilon);
    QVERIFY(std::abs(MNEMath::log2(1024) - 10.0) < m_dEpsilon);
    QVERIFY(std::abs(MNEMath::log2(0.5) - (-1.0)) < m_dEpsilon);
}

//=============================================================================================================

void TestMNEMath::testNchoose2EdgeCases()
{
    QCOMPARE(MNEMath::nchoose2(0), 0);
    QCOMPARE(MNEMath::nchoose2(1), 0);
    QCOMPARE(MNEMath::nchoose2(3), 3);
}

//=============================================================================================================

void TestMNEMath::testHistcounts()
{
    // Create simple data matrix
    MatrixXd data(1, 10);
    data << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0;

    VectorXd classLimits;
    VectorXi frequency;

    MNEMath::histcounts<double>(data, false, 5, classLimits, frequency);

    // Should have 5 classes + 1 limit
    QCOMPARE(classLimits.size(), (Eigen::Index)6);
    QCOMPARE(frequency.size(), (Eigen::Index)5);

    // Total frequency should equal number of data points
    int total = 0;
    for (int i = 0; i < frequency.size(); ++i)
        total += frequency(i);
    QCOMPARE(total, 10);

    // Test vector overload
    VectorXd colData(10);
    colData << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0;
    VectorXd classLimits2;
    VectorXi frequency2;
    MNEMath::histcounts<double>(colData, false, 5, classLimits2, frequency2);
    QCOMPARE(frequency2.size(), (Eigen::Index)5);

    // Test row vector overload
    RowVectorXd rowData(10);
    rowData << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0;
    VectorXd classLimits3;
    VectorXi frequency3;
    MNEMath::histcounts<double>(rowData, false, 5, classLimits3, frequency3);
    QCOMPARE(frequency3.size(), (Eigen::Index)5);

    // Test symmetric histogram
    MatrixXd symData(1, 6);
    symData << -5.0, -3.0, -1.0, 1.0, 3.0, 5.0;
    VectorXd symClassLimits;
    VectorXi symFrequency;
    MNEMath::histcounts<double>(symData, true, 4, symClassLimits, symFrequency);
    QCOMPARE(symClassLimits.size(), (Eigen::Index)5);
    // Symmetric: limits should be mirrored
    QVERIFY(std::abs(symClassLimits(0) + symClassLimits(4)) < m_dEpsilon);
}

//=============================================================================================================

void TestMNEMath::testPinv()
{
    // Test pinv of identity => identity
    MatrixXd I = MatrixXd::Identity(3, 3);
    MatrixXd I_pinv = MNEMath::pinv<double>(I);
    QVERIFY(I_pinv.isApprox(I, 1e-10));

    // Rectangular matrix: A * pinv(A) * A ≈ A (Moore-Penrose condition)
    MatrixXd A(3, 2);
    A << 1, 2,
         3, 4,
         5, 6;
    MatrixXd A_pinv = MNEMath::pinv<double>(A);
    QCOMPARE(A_pinv.rows(), (Eigen::Index)2);
    QCOMPARE(A_pinv.cols(), (Eigen::Index)3);

    MatrixXd reconstructed = A * A_pinv * A;
    QVERIFY(reconstructed.isApprox(A, 1e-10));
}

//=============================================================================================================

void TestMNEMath::testPinvRankDeficient()
{
    // Rank-1 matrix
    MatrixXd A(3, 3);
    A << 1, 2, 3,
         2, 4, 6,
         3, 6, 9;

    MatrixXd A_pinv = MNEMath::pinv<double>(A);

    // Moore-Penrose: A * A+ * A ≈ A
    MatrixXd reconstructed = A * A_pinv * A;
    QVERIFY(reconstructed.isApprox(A, 1e-8));
}

//=============================================================================================================

void TestMNEMath::testCompareTransformation()
{
    // Identity transformations => no movement
    MatrixX4f T1 = MatrixX4f::Identity(4, 4);
    MatrixX4f T2 = MatrixX4f::Identity(4, 4);

    QVERIFY(!MNEMath::compareTransformation(T1, T2, 5.0f, 0.005f));

    // Large translation
    MatrixX4f T3 = MatrixX4f::Identity(4, 4);
    T3(0, 3) = 0.01f;  // 10mm translation
    QVERIFY(MNEMath::compareTransformation(T1, T3, 5.0f, 0.005f));

    // Large rotation (around z by 10 degrees)
    float angle = 10.0f * M_PI / 180.0f;
    MatrixX4f T4 = MatrixX4f::Identity(4, 4);
    T4(0, 0) = cos(angle);
    T4(0, 1) = -sin(angle);
    T4(1, 0) = sin(angle);
    T4(1, 1) = cos(angle);
    QVERIFY(MNEMath::compareTransformation(T1, T4, 5.0f, 0.05f));
}

//=============================================================================================================

void TestMNEMath::cleanupTestCase()
{
    qInfo() << "TestMNEMath: All tests completed";
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMNEMath)
#include "test_utils_mnemath.moc"
