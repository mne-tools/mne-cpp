//=============================================================================================================
/**
 * @file     test_source_estimate_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for source estimate extended I/O (CSV, matrix export).
 */

#include <inv/inv_source_estimate_io.h>
#include <inv/inv_source_estimate.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <Eigen/Core>
#include <cmath>

using namespace INVLIB;
using namespace Eigen;

class TestSourceEstimateIO : public QObject
{
    Q_OBJECT

private:
    InvSourceEstimate makeTestStc(int nVerts = 5, int nTimes = 10)
    {
        VectorXi verts(nVerts);
        for (int i = 0; i < nVerts; ++i) verts(i) = i * 10;
        MatrixXd data = MatrixXd::Random(nVerts, nTimes);
        return InvSourceEstimate(data, verts, 0.1f, 0.001f);
    }

private slots:
    void testWriteCsv()
    {
        auto stc = makeTestStc();

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(InvSourceEstimateIO::writeCsv(stc, path));

        // File should exist and have content
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray content = file.readAll();
        QVERIFY(content.size() > 0);
        QVERIFY(content.contains("time"));
    }

    void testCsvRoundTrip()
    {
        auto stc = makeTestStc(3, 8);

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(InvSourceEstimateIO::writeCsv(stc, path));

        auto loaded = InvSourceEstimateIO::readCsv(path);
        QVERIFY(!loaded.isEmpty());
        QCOMPARE(loaded.data.rows(), stc.data.rows());
        QCOMPARE(loaded.data.cols(), stc.data.cols());

        // Vertex indices should match
        for (int i = 0; i < stc.vertices.size(); ++i) {
            QCOMPARE(loaded.vertices(i), stc.vertices(i));
        }

        // Data should be close (limited by text precision)
        QVERIFY((loaded.data - stc.data).norm() < 1e-5);
    }

    void testCsvWithCustomDelimiter()
    {
        auto stc = makeTestStc(2, 4);

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(InvSourceEstimateIO::writeCsv(stc, path, '\t'));

        auto loaded = InvSourceEstimateIO::readCsv(path, '\t');
        QVERIFY(!loaded.isEmpty());
        QCOMPARE(loaded.data.rows(), static_cast<Index>(2));
        QCOMPARE(loaded.data.cols(), static_cast<Index>(4));
    }

    void testWriteMatrix()
    {
        auto stc = makeTestStc(3, 5);

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(InvSourceEstimateIO::writeMatrix(stc, path));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray content = file.readAll();
        QVERIFY(content.size() > 0);
        // Should not contain "time" header
        QVERIFY(!content.contains("time"));
    }

    void testWriteEmptyStc()
    {
        InvSourceEstimate empty;
        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        QVERIFY(!InvSourceEstimateIO::writeCsv(empty, path));
        QVERIFY(!InvSourceEstimateIO::writeMatrix(empty, path));
    }

    void testReadNonexistent()
    {
        auto stc = InvSourceEstimateIO::readCsv("/nonexistent/path.csv");
        QVERIFY(stc.isEmpty());
    }

    void testCsvPreservesTimingInfo()
    {
        VectorXi verts(2);
        verts << 5, 15;
        MatrixXd data = MatrixXd::Ones(2, 3);
        InvSourceEstimate stc(data, verts, 0.05f, 0.002f);

        QTemporaryFile tmp;
        tmp.setAutoRemove(true);
        QVERIFY(tmp.open());
        QString path = tmp.fileName();
        tmp.close();

        InvSourceEstimateIO::writeCsv(stc, path);
        auto loaded = InvSourceEstimateIO::readCsv(path);

        QVERIFY(std::abs(loaded.tmin - 0.05f) < 1e-4f);
        QVERIFY(std::abs(loaded.tstep - 0.002f) < 1e-4f);
    }
};

QTEST_GUILESS_MAIN(TestSourceEstimateIO)
#include "test_source_estimate_io.moc"
