//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_sts_cluster.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Tests for STS library (cluster permutation, t-test, f-test, multiple comparisons).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <sts/sts_adjacency.h>
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

#include <algorithm>
#include <cmath>

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
    void testClusterPvalueFloorOneSampleAndFtest();

    // Reference values were produced with scipy 1.16 / mne 1.11 (scipy.stats.ttest_*, f_oneway,
    // mne.stats.*_correction, spatio_temporal_tris_adjacency, permutation_cluster_*_test) on the same data.
    void testTtestsMatchScipy();
    void testFtestMatchesScipy();
    void testCorrectionsMatchMne();
    void testTrisAdjacencyMatchesMne();
    void testClusterSumsMatchMne();

    void cleanupTestCase();

private:
    Eigen::SparseMatrix<int> createChainAdjacency(int n) const;
    static Eigen::MatrixXd oracleObservations(int nObs, int offset, double shiftFirstThree, double shiftRest);
    static Eigen::MatrixXd oracleSubject(int s, int group);
    static void compareValues(const Eigen::MatrixXd& got, const QVector<double>& expected, double tol);
    static QStringList upperEdges(const Eigen::SparseMatrix<int>& adjacency);
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

    QCOMPARE(result.dfBetween, 2); // k - 1
    QCOMPARE(result.dfWithin, 27); // N - k
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
    // Two groups of 10 observations across 8 spatial points.
    // StatsCluster::permutationTest expects each observation as a
    // (nChannels x nTimes) matrix where nChannels matches the
    // adjacency dimension. We use nTimes=1 here.
    int nObs = 10;
    int nSpace = 8;

    QVector<MatrixXd> dataA, dataB;
    for (int i = 0; i < nObs; ++i) {
        dataA.append(MatrixXd::Random(nSpace, 1));
        dataB.append(MatrixXd::Random(nSpace, 1) + MatrixXd::Constant(nSpace, 1, 3.0));
    }

    SparseMatrix<int> adj = createChainAdjacency(nSpace);

    StatsClusterResult result = StatsCluster::permutationTest(
        dataA, dataB, adj, 100, 0.05, 0.05);

    // Result should have the right dimensions
    QCOMPARE(static_cast<int>(result.matTObs.rows()), nSpace);
    // Cluster threshold should be set
    QVERIFY(result.clusterThreshold > 0);
    // The shift of 3 is never exceeded by a permutation; p is still >= 1/(100+1) as in mne-python
    QVERIFY(!result.vecClusterPvals.isEmpty());
    for (double p : result.vecClusterPvals) {
        QVERIFY2(p >= 1.0 / 101.0 && p <= 1.0, qPrintable(QString::number(p)));
    }
}

//=============================================================================================================

void TestStsCluster::testClusterPermutationNullDistribution()
{
    // Same distribution in both groups — should not find significant clusters
    int nObs = 15;
    int nSpace = 6;

    QVector<MatrixXd> dataA, dataB;
    for (int i = 0; i < nObs; ++i) {
        dataA.append(MatrixXd::Random(nSpace, 1));
        dataB.append(MatrixXd::Random(nSpace, 1));
    }

    SparseMatrix<int> adj = createChainAdjacency(nSpace);

    StatsClusterResult result = StatsCluster::permutationTest(
        dataA, dataB, adj, 50, 0.05, 0.05);

    // Under null, most cluster p-values should be > 0.05
    int nSignificant = 0;
    for (double p : result.vecClusterPvals) {
        if (p < 0.05)
            ++nSignificant;
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
        MatrixXd a = MatrixXd::Random(nSpace, 1) * 0.1;
        MatrixXd b = MatrixXd::Random(nSpace, 1) * 0.1;
        // Add strong effect at spatial positions 2-5
        for (int j = 2; j <= 5; ++j) {
            b(j, 0) += 10.0;
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

void TestStsCluster::testClusterPvalueFloorOneSampleAndFtest()
{
    // An effect no permutation reaches: p must be exactly 1/(nPerm+1), as in mne-python
    const int nSpace = 6;
    QVector<MatrixXd> shifted, zero;
    for (int s = 0; s < 10; ++s) {
        MatrixXd x(nSpace, 1);
        for (int j = 0; j < nSpace; ++j) {
            x(j, 0) = 0.1 * std::sin(1.7 * s + j) + (j >= 2 && j <= 3 ? 5.0 : 0.0);
        }
        shifted.append(x);
        zero.append(MatrixXd::Constant(nSpace, 1, 0.1 * std::cos(0.9 * s)));
    }
    const SparseMatrix<int> adj = createChainAdjacency(nSpace);

    const StatsClusterResult oneSample = StatsCluster::oneSamplePermutationTest(shifted, adj, 3.0, 99, StatsTailType::Both);
    const StatsClusterResult fTest = StatsCluster::fTestPermutationTest({shifted, zero}, adj, 10.0, 99);
    for (const StatsClusterResult* result : {&oneSample, &fTest}) {
        QVERIFY(!result->vecClusterPvals.isEmpty());
        const double pMin = *std::min_element(result->vecClusterPvals.begin(), result->vecClusterPvals.end());
        // A random permutation can reproduce the observed labelling (sign flips all +1), so allow a few hits
        QVERIFY2(pMin >= 1.0 / 100.0 && pMin <= 5.0 / 100.0, qPrintable(QString::number(pMin)));
    }
}

//=============================================================================================================

MatrixXd TestStsCluster::oracleObservations(int nObs, int offset, double shiftFirstThree, double shiftRest)
{
    MatrixXd x(nObs, 5);
    for (int s = 0; s < nObs; ++s) {
        for (int j = 0; j < 5; ++j) {
            const double t = s + offset;
            x(s, j) = std::sin(1.3 * t + 0.7 * j + 0.2 * t * j) + (j < 3 ? shiftFirstThree : shiftRest);
        }
    }
    return x;
}

//=============================================================================================================

MatrixXd TestStsCluster::oracleSubject(int s, int group)
{
    // 6 vertices x 4 times; group < 0 is the one-sample data set with a positive and a negative bump
    MatrixXd x(6, 4);
    const int t0 = group < 0 ? s : s + 30 * group;
    for (int v = 0; v < 6; ++v) {
        for (int t = 0; t < 4; ++t) {
            double shift = 0.0;
            if (group < 0) {
                shift = ((v <= 2 && (t == 1 || t == 2)) ? 1.5 : 0.0) - ((v >= 4 && t == 3) ? 1.4 : 0.0);
            } else if (v < 3 && t > 0) {
                shift = 0.9 * group;
            }
            x(v, t) = 0.8 * std::sin(0.9 * t0 + 1.1 * v + 0.37 * t + 0.05 * t0 * v) + shift;
        }
    }
    return x;
}

//=============================================================================================================

void TestStsCluster::compareValues(const MatrixXd& got, const QVector<double>& expected, double tol)
{
    QCOMPARE(static_cast<int>(got.size()), static_cast<int>(expected.size()));
    for (int i = 0; i < expected.size(); ++i) {
        const double g = got.data()[i];
        QVERIFY2(std::fabs(g - expected[i]) <= tol * std::max(1.0, std::fabs(expected[i])),
                 qPrintable(QString("index %1: %2 vs %3").arg(i).arg(g, 0, 'g', 12).arg(expected[i], 0, 'g', 12)));
    }
}

//=============================================================================================================

QStringList TestStsCluster::upperEdges(const SparseMatrix<int>& adjacency)
{
    QStringList edges;
    for (int k = 0; k < adjacency.outerSize(); ++k) {
        for (SparseMatrix<int>::InnerIterator it(adjacency, k); it; ++it) {
            if (it.row() < it.col() && it.value() != 0) {
                edges << QString("%1-%2").arg(it.row()).arg(it.col());
            }
        }
    }
    edges.sort();
    return edges;
}

//=============================================================================================================

void TestStsCluster::testTtestsMatchScipy()
{
    const MatrixXd a = oracleObservations(9, 0, 0.6, 0.0);
    const MatrixXd b = oracleObservations(12, 20, 0.0, 0.0);
    MatrixXd p = a;
    for (int s = 0; s < 9; ++s) {
        for (int j = 0; j < 5; ++j) {
            p(s, j) -= 0.3 * std::cos(s + 1.7 * j) + 0.15;
        }
    }

    // scipy.stats.ttest_1samp(A, 0.25) / ttest_rel(A, P) / ttest_ind(A, B), alternative two-sided/less/greater
    const QVector<double> tOne = {1.72357449612, 1.56814718017, 1.94780768253, -1.06363101376, -0.984430170265};
    const QVector<double> tInd = {1.65862122237, 1.82881258626, 2.41803907298, -0.00604345427716, 0.0115994204498};
    const QVector<std::pair<StatsTailType, QVector<QVector<double>>>> cases = {
        {StatsTailType::Both,
         {{0.123076383077, 0.155483847471, 0.0872873289603, 0.318544347626, 0.353736367667},
          {0.0287087666695, 0.237767250709, 0.138169331563, 0.0189591048384, 0.0556362253499},
          {0.113609656123, 0.0831697203862, 0.025816756409, 0.995241054318, 0.990866140445}}},
        {StatsTailType::Left,
         {{0.938461808462, 0.922258076265, 0.95635633552, 0.159272173813, 0.176868183834},
          {0.985645616665, 0.881116374645, 0.930915334219, 0.990520447581, 0.972181887325},
          {0.943195171939, 0.958415139807, 0.987091621795, 0.497620527159, 0.504566929778}}},
        {StatsTailType::Right,
         {{0.0615381915383, 0.0777419237353, 0.0436436644802, 0.840727826187, 0.823131816166},
          {0.0143543833348, 0.118883625355, 0.0690846657814, 0.00947955241922, 0.0278181126749},
          {0.0568048280615, 0.0415848601931, 0.0129083782045, 0.502379472841, 0.495433070222}}},
    };
    for (const auto& [tail, pvals] : cases) {
        const StatsTtestResult one = StatsTtest::oneSample(a, 0.25, tail);
        const StatsTtestResult rel = StatsTtest::paired(a, p, tail);
        const StatsTtestResult ind = StatsTtest::independent(a, b, tail);
        compareValues(one.matTstat, tOne, 1e-9);
        compareValues(ind.matTstat, tInd, 1e-9);
        compareValues(one.matPval, pvals[0], 1e-8);
        compareValues(rel.matPval, pvals[1], 1e-8);
        compareValues(ind.matPval, pvals[2], 1e-8);
        QCOMPARE(one.degreesOfFreedom, 8);
        QCOMPARE(ind.degreesOfFreedom, 19);
    }
}

//=============================================================================================================

void TestStsCluster::testFtestMatchesScipy()
{
    const StatsFtestResult f = StatsFtest::oneWay({oracleObservations(9, 0, 0.6, 0.0),
                                                   oracleObservations(12, 20, 0.0, 0.0),
                                                   oracleObservations(7, 40, -0.4, -0.4)});
    // scipy.stats.f_oneway(A, B, C) (mne.stats.f_oneway gives the same F)
    compareValues(f.matFstat, {3.26506049737, 4.98140232876, 4.81577700322, 0.448234705424, 1.31099139905}, 1e-9);
    compareValues(f.matPval, {0.0549768233267, 0.0151067767683, 0.0170157211473, 0.643788367021, 0.287451432677}, 1e-8);
    QCOMPARE(f.dfBetween, 2);
    QCOMPARE(f.dfWithin, 25);
}

//=============================================================================================================

void TestStsCluster::testCorrectionsMatchMne()
{
    MatrixXd pv(2, 4);
    // Column-major fill, so the flat order is the reference order
    const double flat[] = {0.01, 0.04, 0.03, 0.2, 0.005, 0.5, 0.04, 0.9};
    std::copy(std::begin(flat), std::end(flat), pv.data());
    // mne.stats.bonferroni_correction / fdr_correction; Holm = numpy step-down max-accumulate
    compareValues(StatsMcCorrection::bonferroni(pv), {0.08, 0.32, 0.24, 1, 0.04, 1, 0.32, 1}, 1e-12);
    compareValues(StatsMcCorrection::fdr(pv), {0.04, 0.064, 0.064, 0.266666666667, 0.04, 0.571428571429, 0.064, 0.9},
                  1e-11);
    compareValues(StatsMcCorrection::holmBonferroni(pv), {0.07, 0.2, 0.18, 0.6, 0.04, 1, 0.2, 1}, 1e-12);
}

//=============================================================================================================

void TestStsCluster::testTrisAdjacencyMatchesMne()
{
    MatrixX3i tris(4, 3);
    tris << 0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4;
    // mne.spatial_tris_adjacency(tris)
    QCOMPARE(upperEdges(StatsAdjacency::fromSourceSpace(tris, 6)).join(' '), QStringLiteral("0-1 0-2 1-2 1-3 2-3 2-4 3-4 3-5 4-5"));
    // mne.spatio_temporal_tris_adjacency(tris, 4), re-indexed from t*nV+v to MNE-CPP's v*nT+t
    QStringList expected = QStringLiteral(
                               "0-1 0-4 0-8 1-2 1-5 1-9 2-3 2-6 2-10 3-7 3-11 4-5 4-8 4-12 5-6 5-9 5-13 6-7 6-10 6-14 "
                               "7-11 7-15 8-9 8-12 8-16 9-10 9-13 9-17 10-11 10-14 10-18 11-15 11-19 12-13 12-16 12-20 "
                               "13-14 13-17 13-21 14-15 14-18 14-22 15-19 15-23 16-17 16-20 17-18 17-21 18-19 18-22 "
                               "19-23 20-21 21-22 22-23")
                               .split(' ');
    expected.sort();
    QCOMPARE(upperEdges(StatsAdjacency::fromSourceSpaceTemporal(tris, 6, 4)), expected);
}

//=============================================================================================================

void TestStsCluster::testClusterSumsMatchMne()
{
    MatrixX3i tris(4, 3);
    tris << 0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4;
    const SparseMatrix<int> adj = StatsAdjacency::fromSourceSpaceTemporal(tris, 6, 4);

    QVector<MatrixXd> x;
    for (int s = 0; s < 10; ++s) {
        x.append(oracleSubject(s, -1));
    }
    // mne.stats.permutation_cluster_1samp_test(X, threshold=+-2, adjacency, tail): T_obs and sorted cluster sums
    const QVector<double> tObs = {0.771372356921, 9.09236914862, 9.12936151371, 0.881500872446, 0.754460971088,
                                  8.51877013753, 8.08572445253, -0.185388348512, -0.338456122904, 7.50688531999,
                                  7.61210535246, -0.916448350028, -0.778289560636, -0.653600837936, -0.444949805677,
                                  -0.199903537471, -0.040595395669, 0.159587916852, 0.351235431264, -7.23003113546,
                                  0.395060347363, 0.420482170674, 0.377301852783, -7.17373102444};
    const QVector<std::pair<StatsTailType, QVector<double>>> cases = {
        {StatsTailType::Both, {-14.4037621599, 49.9452159248}},
        {StatsTailType::Right, {49.9452159248}},
        {StatsTailType::Left, {-14.4037621599}}};
    for (const auto& [tail, sums] : cases) {
        const StatsClusterResult r = StatsCluster::oneSamplePermutationTest(x, adj, 2.0, 16, tail);
        MatrixXd t = r.matTObs.transpose();
        compareValues(t, tObs, 1e-9);
        QVector<double> got = r.vecClusterStats;
        std::sort(got.begin(), got.end());
        compareValues(Map<const VectorXd>(got.constData(), got.size()), sums, 1e-9);
    }

    QVector<QVector<MatrixXd>> groups(3);
    for (int g = 0; g < 3; ++g) {
        for (int s = 0; s < 6 + g; ++s) {
            groups[g].append(oracleSubject(s, g));
        }
    }
    // mne.stats.permutation_cluster_test(G, threshold=3.0, adjacency): F_obs and cluster sums
    const QVector<double> fObs = {0.122091241775, 12.21778446, 12.0920791162, 12.7596845925, 0.171483156373,
                                  17.5415105767, 18.2858687168, 18.5837096342, 0.0566795808527, 16.7234322352,
                                  16.8590695231, 16.825753784, 0.0997493216428, 0.129763992525, 0.134809137764,
                                  0.114705164553, 0.258164415407, 0.13778022901, 0.080932742813, 0.115000719816,
                                  0.0986159899017, 0.0608399265322, 0.164234048009, 0.354486410202};
    const StatsClusterResult f = StatsCluster::fTestPermutationTest(groups, adj, 3.0, 16);
    MatrixXd ft = f.matTObs.transpose();
    compareValues(ft, fObs, 1e-9);
    QCOMPARE(f.vecClusterStats.size(), 1);
    QVERIFY(std::fabs(f.vecClusterStats[0] - 141.888892639) < 1e-7);
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
