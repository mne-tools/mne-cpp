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
    MatrixXd generateBlobs(int pointsPerCluster, int k) {
        MatrixXd data(pointsPerCluster * k, 2);
        for (int c = 0; c < k; ++c) {
            double cx = c * 10.0;  // well-separated centers
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
    void initTestCase() {
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
        KMeans kmeans("sqeuclidean", "sample", 1, "error", true, 100);
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
        KMeans kmeans("cityblock", "sample", 1, "error", true, 50);
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
        KMeans kmeans("cosine", "sample", 1, "error", true, 50);
        // For cosine distance, use non-zero data
        MatrixXd X(30, 3);
        for (int i = 0; i < 10; ++i) {
            X.row(i) = RowVector3d(1.0 + 0.1*i, 0.0, 0.0);
            X.row(10+i) = RowVector3d(0.0, 1.0 + 0.1*i, 0.0);
            X.row(20+i) = RowVector3d(0.0, 0.0, 1.0 + 0.1*i);
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
        KMeans kmeans("correlation", "sample", 1, "error", true, 50);
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
        KMeans kmeans("sqeuclidean", "uniform", 1, "error", true, 50);
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
        KMeans kmeans("sqeuclidean", "sample", 1, "error", false, 50);
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
        KMeans kmeans("sqeuclidean", "sample", 3, "error", true, 50);
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
        KMeans kmeans("sqeuclidean", "sample", 3, "error", true, 100);
        MatrixXd X = generateBlobs(20, 3);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        kmeans.calculate(X, 3, idx, C, sumD, D);

        // Verify all points in the same original cluster share the same label
        int label0 = idx(0);
        for (int i = 1; i < 20; ++i) {
            QCOMPARE(idx(i), label0);
        }
        int label1 = idx(20);
        for (int i = 21; i < 40; ++i) {
            QCOMPARE(idx(i), label1);
        }
    }

    void testSumDNonNegative()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "error", true, 50);
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

    void testEmptyactDrop()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "drop", true, 50);
        MatrixXd X = generateBlobs(10, 2);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 2, idx, C, sumD, D);
        QVERIFY(ok);
    }

    void testEmptyactSingleton()
    {
        KMeans kmeans("sqeuclidean", "sample", 1, "singleton", true, 50);
        MatrixXd X = generateBlobs(10, 2);
        VectorXi idx;
        MatrixXd C, D;
        VectorXd sumD;

        bool ok = kmeans.calculate(X, 2, idx, C, sumD, D);
        QVERIFY(ok);
    }
};

QTEST_GUILESS_MAIN(TestUtilsKMeans)
#include "test_utils_kmeans.moc"
