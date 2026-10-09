// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>

#include <QtTest/QtTest>
#include <Eigen/Dense>
#include <math/kmeans.h>

using namespace UTILSLIB;
using namespace Eigen;

class TestUtilsKMeans : public QObject
{
    Q_OBJECT

private:
    // Generate 2D blobs with known cluster centers
    MatrixXd generateBlobs(int pointsPerCluster, int k)
    {
        MatrixXd data(pointsPerCluster * k, 2);
        for (int c = 0; c < k; ++c) {
            double cx = c * 10.0; // well-separated centers
            double cy = c * 10.0;
            for (int i = 0; i < pointsPerCluster; ++i) {
                int row = c * pointsPerCluster + i;
                data(row, 0) = cx + 0.5 * (double(rand()) / RAND_MAX - 0.5);
                data(row, 1) = cy + 0.5 * (double(rand()) / RAND_MAX - 0.5);
            }
        }
        return data;
    }

private slots:
    void initTestCase()
    {
        srand(42);
    }

    void testDefaultCtor()
    {
        KMeans kmeans;
        Q_UNUSED(kmeans);
        QVERIFY(true);
    }

    void testCtorSqEuclidean()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "error", true, 100);
        Q_UNUSED(kmeans);
        QVERIFY(true);
    }

    void testCtorCityblock()
    {
        KMeans kmeans("cityblock", "sample", 1, "error", true, 50);
        Q_UNUSED(kmeans);
        QVERIFY(true);
    }

    void testCtorCosine()
    {
        KMeans kmeans("cosine", "sample", 1, "error", true, 50);
        Q_UNUSED(kmeans);
        QVERIFY(true);
    }

    void testCtorCorrelation()
    {
        KMeans kmeans("correlation", "sample", 1, "error", true, 50);
        Q_UNUSED(kmeans);
        QVERIFY(true);
    }

    void testCalculateSqEuclidean()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "singleton", true, 100);
        MatrixXd X = generateBlobs(20, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 3, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 60);
        QCOMPARE(C.rows(), 3);
        QCOMPARE(C.cols(), 2);
        QCOMPARE(sumD.size(), 3);
    }

    void testCalculateCityblock()
    {
        KMeans kmeans("cityblock", "sample", 1, "singleton", true, 50);
        MatrixXd X = generateBlobs(15, 2);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 2, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 30);
        QCOMPARE(C.rows(), 2);
    }

    void testCalculateCosine()
    {
        KMeans kmeans("cosine", "sample", 1, "singleton", true, 50);
        // For cosine distance, use non-zero data
        // Assign coefficient wise rather than from a fixed size RowVector3d.
        // Assigning a Matrix<double,1,3> into a dynamically sized row block
        // makes GCC unable to prove the destination has three columns and it
        // reports a false -Warray-bounds inside Eigen.
        MatrixXd X = MatrixXd::Zero(30, 3);
        for (int i = 0; i < 10; ++i) {
            X(i, 0) = 1.0 + 0.1 * i;
            X(10 + i, 1) = 1.0 + 0.1 * i;
            X(20 + i, 2) = 1.0 + 0.1 * i;
        }

        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 3, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 30);
    }

    void testCalculateCorrelation()
    {
        KMeans kmeans("correlation", "sample", 1, "singleton", true, 50);
        // Need enough dimensions for correlation (>1)
        MatrixXd X = MatrixXd::Random(20, 5);
        // Make first 10 rows different from last 10
        X.topRows(10) += MatrixXd::Constant(10, 5, 5.0);

        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 2, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 20);
    }

    void testCalculateUniformStart()
    {
        KMeans kmeans("sqeuclidean", "uniform", 1, "singleton", true, 50);
        MatrixXd X = generateBlobs(15, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 3, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 45);
    }

    void testCalculateOnlineFalse()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "singleton", false, 50);
        MatrixXd X = generateBlobs(15, 2);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 2, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 30);
    }

    void testCalculateMultipleReplicates()
    {
        KMeans kmeans("sqeuclidean", "sample", 3, "singleton", true, 50);
        MatrixXd X = generateBlobs(15, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 3, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(idx.size(), 45);
    }

    void testClusterAssignmentCorrectness()
    {
        // 3 well-separated clusters
        KMeans kmeans("sqeuclidean", "sample", 100, "singleton", true, 100);
        MatrixXd X = generateBlobs(20, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        QVERIFY(kmeans.calculate(X, 3, idx, C, sumD, D));

        // Verify all points in the same original cluster share the same label
        int label0 = idx(0);
        for (int i = 1; i < 20; ++i) {
            QCOMPARE(idx(i), label0);
        }
        int label1 = idx(20);
        for (int i = 21; i < 40; ++i) {
            QCOMPARE(idx(i), label1);
        }
        int label2 = idx(40);
        for (int i = 41; i < 60; ++i) {
            QCOMPARE(idx(i), label2);
        }
        QVERIFY(label0 != label1);
        QVERIFY(label0 != label2);
        QVERIFY(label1 != label2);
    }

    void testSumDNonNegative()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "singleton", true, 50);
        MatrixXd X = generateBlobs(15, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        kmeans.calculate(X, 3, idx, C, sumD, D);

        for (int i = 0; i < sumD.size(); ++i) {
            QVERIFY(sumD(i) >= 0.0);
        }
    }

    void testDMatrixDimensions()
    {
        KMeans kmeans;
        MatrixXd X = generateBlobs(10, 2);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        kmeans.calculate(X, 2, idx, C, sumD, D);

        QCOMPARE(D.rows(), 20); // total points
        QCOMPARE(D.cols(), 2);  // k clusters
    }

    void testSingleCluster()
    {
        KMeans kmeans;
        MatrixXd X = MatrixXd::Random(20, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 1, idx, C, sumD, D);
        QVERIFY(ok);
        // All points should be in cluster 0
        for (int i = 0; i < idx.size(); ++i) {
            QCOMPARE(idx(i), 0);
        }
    }

    void testHighDimensional()
    {
        KMeans kmeans;
        MatrixXd X = MatrixXd::Random(50, 10);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 5, idx, C, sumD, D);
        QVERIFY(ok);
        QCOMPARE(C.rows(), 5);
        QCOMPARE(C.cols(), 10);
    }

    void testEmptyactions_data()
    {
        QTest::addColumn<QString>("emptyact");
        QTest::addColumn<bool>("online");
        for (const QString& action : {QStringLiteral("error"), QStringLiteral("drop"), QStringLiteral("singleton")}) {
            QTest::newRow(qPrintable(action + "-batch")) << action << false;
            QTest::newRow(qPrintable(action + "-online")) << action << true;
        }
    }

    void testEmptyactions()
    {
        // Six copies of each of two points; seeds 0, 0 and 10 leave cluster 1 empty after the first assignment
        QFETCH(QString, emptyact);
        QFETCH(bool, online);
        MatrixXd X = MatrixXd::Zero(12, 2);
        X.bottomRows(6).setConstant(10.0);
        MatrixXd start(3, 2);
        start << 0.0, 0.0, 0.0, 0.0, 10.0, 10.0;
        KMeans kmeans("sqeuclidean", "sample", 1, emptyact, online, 50);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        const bool ok = kmeans.calculate(X, start, idx, C, sumD, D);

        // Only two distinct points can never fill three clusters, whatever the random seeds
        VectorXi idxRandom;
        MatrixXd CRandom, DRandom;
        VectorXd sumDRandom;
        KMeans replicated("sqeuclidean", "sample", 3, emptyact, online, 50);
        const bool okRandom = replicated.calculate(X, 3, idxRandom, CRandom, sumDRandom, DRandom);

        if (emptyact == QStringLiteral("error")) {
            QVERIFY(!ok);
            QVERIFY(!okRandom);
            return;
        }
        QVERIFY(ok);
        QVERIFY(okRandom);
        QVERIFY(sumDRandom.allFinite());
        QCOMPARE(idx.size(), 12);
        QCOMPARE(D.rows(), 12);
        QCOMPARE(D.cols(), 3);
        VectorXi expectedIdx(12);
        if (emptyact == QStringLiteral("drop")) {
            // The empty cluster is dropped: no centroid, no distances (MATLAB's NaN)
            expectedIdx << 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2;
            QVERIFY(!C.row(1).allFinite());
            QVERIFY(!D.col(1).allFinite());
        } else {
            // The point farthest from its centroid (the first, all are on theirs) becomes a singleton cluster
            expectedIdx << 1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2;
            QVERIFY(C.allFinite());
            QVERIFY(D.allFinite());
        }
        QCOMPARE(idx, expectedIdx);
        for (int i = 0; i < 12; ++i) {
            QCOMPARE((C.row(idx(i)) - X.row(i)).norm(), 0.0);
            QCOMPARE(D(i, idx(i)), 0.0);
        }
        QCOMPARE(sumD, VectorXd::Zero(3).eval());
    }

    void testExplicitStartMatchesLloyd()
    {
        // Three overlapping 2-D groups: the batch (Lloyd) phase from a fixed start must end where scikit-learn's
        // KMeans(init=start, n_init=1, algorithm="lloyd") ends; reference values computed with scikit-learn 1.9.1
        MatrixXd X(12, 2);
        X << 0.0, 0.0, 1.0, 0.2, 0.4, 1.1, 2.1, 1.9, 2.6, 2.4, 3.0, 3.3, 3.4, 2.2, 4.1, 3.9, 4.6, 4.2, 5.2, 4.9, 1.5, 1.4, 3.6, 3.1;
        MatrixXd start(3, 2);
        start << 0.0, 0.0, 1.0, 0.2, 0.4, 1.1;
        KMeans kmeans("sqeuclidean", "sample", 1, "error", false, 100);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        QVERIFY(kmeans.calculate(X, start, idx, C, sumD, D));
        VectorXi expectedIdx(12);
        expectedIdx << 0, 0, 0, 1, 1, 2, 1, 2, 2, 2, 1, 2;
        MatrixXd expectedC(3, 2);
        expectedC << 0.4666666666666668, 0.43333333333333357, 2.4, 1.975, 4.1, 3.8800000000000003;
        QCOMPARE(idx, expectedIdx);
        QVERIFY((C - expectedC).cwiseAbs().maxCoeff() < 1e-12);
        QVERIFY(std::abs(sumD.sum() - 8.708833333333335) < 1e-12);

        MatrixXd badStart(3, 3);
        QVERIFY(!kmeans.calculate(X, badStart, idx, C, sumD, D));
        QVERIFY(!kmeans.calculate(X.topRows(2), start, idx, C, sumD, D));
    }

    void testEmptiedDuringIterations_data()
    {
        QTest::addColumn<QList<double>>("points");
        QTest::addColumn<QList<double>>("start");
        QTest::addColumn<QString>("emptyact");
        QTest::addColumn<QList<int>>("expectedIdx");
        QTest::addColumn<QList<double>>("expectedC");
        QTest::addColumn<double>("expectedSumD");
        // Reference: MATLAB kmeans batchUpdate semantics re-implemented in NumPy.
        // Last cluster loses its only point in the second step, when only clusters 0, 2 and 3 changed
        const QList<double> points{16.0, 10.0, 15.0, 10.0, 10.0, 6.0, 1.0, 11.0, 10.0};
        const QList<double> start{16.0, 1.0, 6.0, 15.0};
        QTest::newRow("error") << points << start << QStringLiteral("error") << QList<int>() << QList<double>() << 0.0;
        QTest::newRow("drop") << points << start << QStringLiteral("drop") << QList<int>{0, 2, 0, 2, 2, 2, 1, 2, 2}
                              << QList<double>{15.5, 1.0, 9.5, qQNaN()} << 16.0;
        QTest::newRow("singleton") << points << start << QStringLiteral("singleton") << QList<int>{0, 2, 0, 2, 2, 3, 1, 2, 2}
                                   << QList<double>{15.5, 1.0, 10.2, 6.0} << 1.3;
        // First cluster empties: its NaN distances must never win the reassignment
        const QList<double> points2{9.0, 8.0, 2.0, 0.0, 3.0, 8.0, 4.0};
        const QList<double> start2{8.0, 0.0, 9.0};
        QTest::newRow("drop-first") << points2 << start2 << QStringLiteral("drop") << QList<int>{2, 2, 1, 1, 1, 2, 1}
                                    << QList<double>{qQNaN(), 2.25, 25.0 / 3.0} << 9.25 + 1.0 / 6.0;
        QTest::newRow("singleton-first") << points2 << start2 << QStringLiteral("singleton") << QList<int>{2, 2, 1, 0, 1, 2, 1}
                                         << QList<double>{0.0, 3.0, 25.0 / 3.0} << 2.0 + 2.0 / 3.0;
    }

    void testEmptiedDuringIterations()
    {
        QFETCH(QList<double>, points);
        QFETCH(QList<double>, start);
        QFETCH(QString, emptyact);
        QFETCH(QList<int>, expectedIdx);
        QFETCH(QList<double>, expectedC);
        QFETCH(double, expectedSumD);
        const MatrixXd X = Map<const VectorXd>(points.constData(), points.size());
        const MatrixXd startC = Map<const VectorXd>(start.constData(), start.size());
        KMeans kmeans("sqeuclidean", "sample", 1, emptyact, false, 100);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        const bool ok = kmeans.calculate(X, startC, idx, C, sumD, D);
        QCOMPARE(ok, !expectedIdx.isEmpty());
        if (!ok) {
            return;
        }
        for (int i = 0; i < points.size(); ++i) {
            QCOMPARE(idx(i), expectedIdx[i]);
        }
        for (int c = 0; c < start.size(); ++c) {
            if (std::isnan(expectedC[c])) {
                QVERIFY(std::isnan(C(c, 0)));
            } else {
                QVERIFY(std::abs(C(c, 0) - expectedC[c]) < 1e-12);
            }
        }
        QVERIFY(std::abs(sumD.sum() - expectedSumD) < 1e-12);
    }

    void testOnlinePhaseAfterDrop()
    {
        // The batch phase drops cluster 0 and stops where moving 15 to the 17s still lowers the total; the online
        // phase must find such moves even though the dropped cluster's costs are NaN
        MatrixXd X(8, 1);
        X << 9.0, 9.0, 5.0, 10.0, 18.0, 17.0, 17.0, 15.0;
        MatrixXd start(4, 1);
        start << 15.0, 18.0, 5.0, 17.0;
        KMeans kmeans("sqeuclidean", "sample", 1, "drop", true, 100);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        QVERIFY(kmeans.calculate(X, start, idx, C, sumD, D));
        QVERIFY(std::isnan(C(0, 0)));
        const auto total = [&X](const VectorXi& labels) {
            double sum = 0.0;
            for (int c = 0; c < 4; ++c) {
                const Array<bool, Dynamic, 1> in = labels.array() == c;
                const int count = static_cast<int>(in.count());
                if (count > 0) {
                    const double mean = in.select(X.col(0).array(), 0.0).sum() / count;
                    sum += in.select((X.col(0).array() - mean).square(), 0.0).sum();
                }
            }
            return sum;
        };
        QVERIFY(std::abs(sumD.sum() - total(idx)) < 1e-12);
        for (int i = 0; i < 8; ++i) {
            for (int c = 1; c < 4; ++c) {
                if (c == idx(i) || (idx.array() == idx(i)).count() < 2) {
                    continue;
                }
                VectorXi moved = idx;
                moved(i) = c;
                QVERIFY2(total(moved) >= total(idx) - 1e-12, qPrintable(QStringLiteral("point %1 to %2").arg(i).arg(c)));
            }
        }
    }

    void testHamming_data()
    {
        QTest::addColumn<bool>("online");
        QTest::newRow("batch") << false;
        QTest::newRow("online") << true;
    }

    void testHamming()
    {
        // Two groups of binary codes; centroids are the component-wise medians (MATLAB), D the fraction of differing bits
        QFETCH(bool, online);
        MatrixXd X(8, 6);
        X << 1, 1, 1, 0, 0, 0,
            1, 1, 1, 0, 0, 1,
            1, 1, 0, 0, 0, 0,
            1, 1, 1, 0, 0, 0,
            0, 0, 0, 1, 1, 1,
            0, 0, 1, 1, 1, 1,
            0, 0, 0, 1, 1, 0,
            0, 0, 0, 1, 1, 1;
        MatrixXd start(2, 6);
        start << X.row(2), X.row(6);
        KMeans kmeans("hamming", "sample", 1, "error", online, 100);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        QVERIFY(kmeans.calculate(X, start, idx, C, sumD, D));
        for (int i = 0; i < 8; ++i) {
            QCOMPARE(idx(i), i < 4 ? 0 : 1);
        }
        MatrixXd expectedC(2, 6);
        expectedC << 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1;
        QCOMPARE(C, expectedC);
        QCOMPARE(D(1, 0), 1.0 / 6.0);
        QCOMPARE(D(0, 1), 1.0);
        QCOMPARE(sumD(0), 2.0 / 6.0);
        QCOMPARE(sumD(1), 2.0 / 6.0);

        // Random sample starts reach the same partition with replicates
        KMeans replicated("hamming", "sample", 10, "drop", online, 100);
        QVERIFY(replicated.calculate(X, 2, idx, C, sumD, D));
        QCOMPARE(sumD.sum(), 4.0 / 6.0);
    }

    void testClusterStart()
    {
        // "cluster" seeds from a preliminary clustering of a 10 % subsample, then clusters all points
        KMeans kmeans("sqeuclidean", "cluster", 5, "drop", true, 100);
        MatrixXd X = generateBlobs(40, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;
        QVERIFY(kmeans.calculate(X, 3, idx, C, sumD, D));
        QVERIFY(C.allFinite());
        for (int c = 0; c < 3; ++c) {
            for (int i = 1; i < 40; ++i) {
                QCOMPARE(idx(c * 40 + i), idx(c * 40));
            }
        }
        QVERIFY(idx(0) != idx(40) && idx(0) != idx(80) && idx(40) != idx(80));
    }
};

QTEST_GUILESS_MAIN(TestUtilsKMeans)
#include "test_utils_kmeans.moc"
