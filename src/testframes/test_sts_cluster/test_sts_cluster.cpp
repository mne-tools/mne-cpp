//=============================================================================================================
/**
 * @file     test_sts_cluster.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 *
 * @brief    Tests for STS library (cluster permutation, t-test, f-test, multiple comparisons).
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <sts/sts_cluster.h>
#include <sts/sts_ttest.h>
#include <sts/sts_ftest.h>
#include <sts/sts_correction.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Sparse>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QObject>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace STSLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestStsCluster
 *
 * @brief The TestStsCluster class provides tests for the STS statistics library.
 */
class TestStsCluster : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // T-test
    void testTtestOneSample();
    void testTtestOneSampleZeroMean();
    void testTtestPaired();
    void testTtestIndependent();
    void testTtestDegreesOfFreedom();

    // F-test
    void testFtestOneWay();
    void testFtestEqualGroups();
    void testFtestDegreesOfFreedom();

    // Multiple comparison correction
    void testBonferroni();
    void testHolmBonferroni();
    void testFdr();
    void testCorrectionBounds();

    // Cluster permutation test
    void testClusterPermutationBasic();
    void testClusterPermutationNullDistribution();
    void testClusterPermutationStrongEffect();

    void cleanupTestCase();

private:
    Eigen::SparseMatrix<int> createChainAdjacency(int n) const;
};

//=============================================================================================================

void TestStsCluster::initTestCase()
{
}

//=============================================================================================================

Eigen::SparseMatrix<int> TestStsCluster::createChainAdjacency(int n) const
{
    // Simple chain adjacency: each vertex connected to its neighbors
    SparseMatrix<int> adj(n, n);
    QVector<Triplet<int>> triplets;
    for (int i = 0; i < n - 1; ++i) {
        triplets.append(Triplet<int>(i, i + 1, 1));
        triplets.append(Triplet<int>(i + 1, i, 1));
    }
    adj.setFromTriplets(triplets.begin(), triplets.end());
    return adj;
}

//=============================================================================================================

void TestStsCluster::testTtestOneSample()
{
    // 20 observations, 5 spatial points — test against mu=0
    MatrixXd data = MatrixXd::Random(20, 5) + MatrixXd::Constant(20, 5, 2.0);

    StatsTtestResult result = StatsTtest::oneSample(data, 0.0);

    QCOMPARE(static_cast<int>(result.matTstat.rows()), 1);
    QCOMPARE(static_cast<int>(result.matTstat.cols()), 5);
    QCOMPARE(static_cast<int>(result.matPval.cols()), 5);

    // With mean shifted by 2, t should be positive and significant
    for (int i = 0; i < 5; ++i) {
        QVERIFY(result.matTstat(0, i) > 0);
        QVERIFY(result.matPval(0, i) < 0.05);
    }
}

//=============================================================================================================

void TestStsCluster::testTtestOneSampleZeroMean()
{
    // Large sample of zero-mean noise — should not reject H0
    MatrixXd data = MatrixXd::Random(100, 3);

    StatsTtestResult result = StatsTtest::oneSample(data, 0.0);

    // With zero-mean noise and large N, p-values should generally be > 0.01
    // (not always, so just check t-stats are small relative to offset tests)
    for (int i = 0; i < 3; ++i) {
        QVERIFY(qAbs(result.matTstat(0, i)) < 5.0);
    }
}

//=============================================================================================================

void TestStsCluster::testTtestPaired()
{
    MatrixXd dataA = MatrixXd::Random(15, 4);
    MatrixXd dataB = dataA + MatrixXd::Constant(15, 4, 3.0); // B = A + 3

    StatsTtestResult result = StatsTtest::paired(dataA, dataB);

    QCOMPARE(static_cast<int>(result.matTstat.cols()), 4);

    // Difference is constant 3.0, so t should be large and negative (A < B)
    for (int i = 0; i < 4; ++i) {
        QVERIFY(result.matTstat(0, i) < 0);
        QVERIFY(result.matPval(0, i) < 0.001);
    }
}

//=============================================================================================================

void TestStsCluster::testTtestIndependent()
{
    MatrixXd dataA = MatrixXd::Random(20, 3);
    MatrixXd dataB = MatrixXd::Random(25, 3) + MatrixXd::Constant(25, 3, 5.0);

    StatsTtestResult result = StatsTtest::independent(dataA, dataB);

    QCOMPARE(static_cast<int>(result.matTstat.cols()), 3);

    // Groups differ by 5.0, should be significant
    for (int i = 0; i < 3; ++i) {
        QVERIFY(result.matPval(0, i) < 0.05);
    }
}

//=============================================================================================================

void TestStsCluster::testTtestDegreesOfFreedom()
{
    MatrixXd data = MatrixXd::Random(30, 2);
    StatsTtestResult result = StatsTtest::oneSample(data, 0.0);
    QCOMPARE(result.degreesOfFreedom, 29); // N - 1
}

//=============================================================================================================

void TestStsCluster::testFtestOneWay()
{
    // Three groups with distinct means
    QVector<MatrixXd> groups;
    groups.append(MatrixXd::Random(20, 3));
    groups.append(MatrixXd::Random(20, 3) + MatrixXd::Constant(20, 3, 3.0));
    groups.append(MatrixXd::Random(20, 3) + MatrixXd::Constant(20, 3, 6.0));

    StatsFtestResult result = StatsFtest::oneWay(groups);

    QCOMPARE(static_cast<int>(result.matFstat.cols()), 3);
    QCOMPARE(static_cast<int>(result.matPval.cols()), 3);

    // F-stat should be large and p-values small
    for (int i = 0; i < 3; ++i) {
        QVERIFY(result.matFstat(0, i) > 1.0);
        QVERIFY(result.matPval(0, i) < 0.05);
    }
}

//=============================================================================================================

void TestStsCluster::testFtestEqualGroups()
{
    // Three groups from same distribution — should not reject H0
    QVector<MatrixXd> groups;
    groups.append(MatrixXd::Random(30, 2));
    groups.append(MatrixXd::Random(30, 2));
    groups.append(MatrixXd::Random(30, 2));

    StatsFtestResult result = StatsFtest::oneWay(groups);

    // F-stat should be small (groups come from same distribution)
    for (int i = 0; i < 2; ++i) {
        QVERIFY(result.matFstat(0, i) < 20.0); // Generous bound
    }
}

//=============================================================================================================

void TestStsCluster::testFtestDegreesOfFreedom()
{
    QVector<MatrixXd> groups;
    groups.append(MatrixXd::Random(10, 2));
    groups.append(MatrixXd::Random(10, 2));
    groups.append(MatrixXd::Random(10, 2));

    StatsFtestResult result = StatsFtest::oneWay(groups);

    QCOMPARE(result.dfBetween, 2);    // k - 1
    QCOMPARE(result.dfWithin, 27);    // N - k
}

//=============================================================================================================

void TestStsCluster::testBonferroni()
{
    MatrixXd pVals(1, 5);
    pVals << 0.01, 0.04, 0.005, 0.10, 0.03;

    MatrixXd corrected = StatsMcCorrection::bonferroni(pVals);

    QCOMPARE(corrected.cols(), pVals.cols());

    // Bonferroni: p_corrected = p * n_tests, capped at 1.0
    QVERIFY(qAbs(corrected(0, 0) - 0.05) < 1e-10);
    QVERIFY(qAbs(corrected(0, 1) - 0.20) < 1e-10);
    QVERIFY(qAbs(corrected(0, 2) - 0.025) < 1e-10);
    QVERIFY(qAbs(corrected(0, 3) - 0.50) < 1e-10);
    QVERIFY(qAbs(corrected(0, 4) - 0.15) < 1e-10);
}

//=============================================================================================================

void TestStsCluster::testHolmBonferroni()
{
    MatrixXd pVals(1, 4);
    pVals << 0.01, 0.04, 0.005, 0.10;

    MatrixXd corrected = StatsMcCorrection::holmBonferroni(pVals);

    QCOMPARE(corrected.cols(), pVals.cols());

    // All corrected p-values should be >= original
    for (int i = 0; i < 4; ++i) {
        QVERIFY(corrected(0, i) >= pVals(0, i) - 1e-10);
        QVERIFY(corrected(0, i) <= 1.0 + 1e-10);
    }
}

//=============================================================================================================

void TestStsCluster::testFdr()
{
    MatrixXd pVals(1, 5);
    pVals << 0.001, 0.01, 0.05, 0.10, 0.50;

    MatrixXd corrected = StatsMcCorrection::fdr(pVals, 0.05);

    QCOMPARE(corrected.cols(), pVals.cols());

    // All corrected values should be in [0, 1]
    for (int i = 0; i < 5; ++i) {
        QVERIFY(corrected(0, i) >= 0.0);
        QVERIFY(corrected(0, i) <= 1.0 + 1e-10);
    }
}

//=============================================================================================================

void TestStsCluster::testCorrectionBounds()
{
    // Corrected p-values should never exceed 1.0
    MatrixXd pVals(1, 3);
    pVals << 0.5, 0.8, 0.99;

    MatrixXd bonf = StatsMcCorrection::bonferroni(pVals);
    for (int i = 0; i < 3; ++i) {
        QVERIFY(bonf(0, i) <= 1.0 + 1e-10);
    }
}

//=============================================================================================================

void TestStsCluster::testClusterPermutationBasic()
{
    // Two groups of 10 observations across 8 spatial points
    int nObs = 10;
    int nSpace = 8;

    QVector<MatrixXd> dataA, dataB;
    for (int i = 0; i < nObs; ++i) {
        dataA.append(MatrixXd::Random(1, nSpace));
        dataB.append(MatrixXd::Random(1, nSpace) + MatrixXd::Constant(1, nSpace, 3.0));
    }

    SparseMatrix<int> adj = createChainAdjacency(nSpace);

    StatsClusterResult result = StatsCluster::permutationTest(
        dataA, dataB, adj, 100, 0.05, 0.05);

    // Result should have the right dimensions
    QCOMPARE(static_cast<int>(result.matTObs.cols()), nSpace);
    // Cluster threshold should be set
    QVERIFY(result.clusterThreshold > 0);
}

//=============================================================================================================

void TestStsCluster::testClusterPermutationNullDistribution()
{
    // Same distribution in both groups — should not find significant clusters
    int nObs = 15;
    int nSpace = 6;

    QVector<MatrixXd> dataA, dataB;
    for (int i = 0; i < nObs; ++i) {
        dataA.append(MatrixXd::Random(1, nSpace));
        dataB.append(MatrixXd::Random(1, nSpace));
    }

    SparseMatrix<int> adj = createChainAdjacency(nSpace);

    StatsClusterResult result = StatsCluster::permutationTest(
        dataA, dataB, adj, 50, 0.05, 0.05);

    // Under null, most cluster p-values should be > 0.05
    int nSignificant = 0;
    for (double p : result.vecClusterPvals) {
        if (p < 0.05) ++nSignificant;
    }
    // At most 1 significant cluster by chance (generous)
    QVERIFY(nSignificant <= 2);
}

//=============================================================================================================

void TestStsCluster::testClusterPermutationStrongEffect()
{
    // Strong signal at spatial points 2-5 — should detect a cluster there
    int nObs = 20;
    int nSpace = 8;

    QVector<MatrixXd> dataA, dataB;
    for (int i = 0; i < nObs; ++i) {
        MatrixXd a = MatrixXd::Random(1, nSpace) * 0.1;
        MatrixXd b = MatrixXd::Random(1, nSpace) * 0.1;
        // Add strong effect at positions 2-5
        for (int j = 2; j <= 5; ++j) {
            b(0, j) += 10.0;
        }
        dataA.append(a);
        dataB.append(b);
    }

    SparseMatrix<int> adj = createChainAdjacency(nSpace);

    StatsClusterResult result = StatsCluster::permutationTest(
        dataA, dataB, adj, 200, 0.05, 0.05);

    // Should detect at least one significant cluster
    bool foundSignificant = false;
    for (double p : result.vecClusterPvals) {
        if (p < 0.05) {
            foundSignificant = true;
            break;
        }
    }
    QVERIFY(foundSignificant);
}

//=============================================================================================================

void TestStsCluster::cleanupTestCase()
{
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestStsCluster)
#include "test_sts_cluster.moc"
