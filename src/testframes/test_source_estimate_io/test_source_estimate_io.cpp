//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_source_estimate_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for source estimate extended I/O (CSV, matrix export).
 */

#include <inv/inv_source_estimate_io.h>
#include <inv/inv_source_estimate.h>
#include <fs/fs_label.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <Eigen/Core>
#include <cmath>

using namespace INVLIB;
using namespace FSLIB;
using namespace Eigen;

class TestSourceEstimateIO : public QObject
{
    Q_OBJECT

private:
    InvSourceEstimate makeTestStc(int nVerts = 5, int nTimes = 10)
    {
        VectorXi verts(nVerts);
        for (int i = 0; i < nVerts; ++i)
            verts(i) = i * 10;
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

    void testMnePythonStcAndW()
    {
        // make_stc_fixtures.py: mne-python SourceEstimate.save, ftype "stc" and "w".
        MatrixXd ref(7, 4);
        ref << 1.764052391052246, 0.40015721321105957, 0.978738009929657, 2.2408931255340576,
            1.8675580024719238, -0.9772778749465942, 0.9500884413719177, -0.15135720372200012,
            -0.10321885347366333, 0.4105985164642334, 0.14404356479644775, 1.4542734622955322,
            0.7610377073287964, 0.12167501449584961, 0.44386324286460876, 0.3336743414402008,
            1.4940791130065918, -0.2051582634449005, 0.3130677044391632, -0.8540957570075989,
            -2.5529897212982178, 0.653618574142456, 0.8644362092018127, -0.7421650290489197,
            2.269754648208618, -1.4543657302856445, 0.04575851559638977, -0.18718385696411133;
        VectorXi verts(7);
        verts << 3, 17, 42, 1000, 5, 99, 70000;
        const QString dir = QStringLiteral(MNE_STC_DATA_DIR);

        InvSourceEstimate lh;
        InvSourceEstimate rh;
        QFile lhFile(dir + "/py-lh.stc");
        QFile rhFile(dir + "/py-rh.stc");
        QVERIFY(InvSourceEstimate::read(lhFile, lh));
        QVERIFY(InvSourceEstimate::read(rhFile, rh));
        QCOMPARE(lh.vertices, verts.head(4));
        QCOMPARE(rh.vertices, verts.tail(3));
        QVERIFY((lh.data - ref.topRows(4)).cwiseAbs().maxCoeff() < 1e-6);
        QVERIFY((rh.data - ref.bottomRows(3)).cwiseAbs().maxCoeff() < 1e-6);
        QVERIFY(std::abs(lh.tmin + 0.1f) < 1e-7f);
        QVERIFY(std::abs(lh.tstep - 0.004f) < 1e-7f);
        QCOMPARE(static_cast<int>(lh.times.size()), 4);
        QVERIFY(std::abs(lh.times[3] + 0.088f) < 1e-6f);

        for (const QString& hemi : {QStringLiteral("lh"), QStringLiteral("rh")}) {
            const InvSourceEstimate w = InvSourceEstimate::read_w(dir + "/pyw-" + hemi + ".w");
            const int offset = hemi == "lh" ? 0 : 4;
            QCOMPARE(w.vertices, verts.segment(offset, w.vertices.size()));
            QVERIFY((w.data.col(0) - ref.col(0).segment(offset, w.vertices.size())).cwiseAbs().maxCoeff() < 1e-6);
        }

        // Files written by MNE-CPP must be byte-identical to mne-python's.
        QTemporaryDir tmp;
        auto bytes = [](const QString& path) {
            QFile f(path);
            return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
        };
        InvSourceEstimate both(ref, verts, -0.1f, 0.004f);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("nVerticesLh is -1"));
        QVERIFY(!both.writeHemispherePair(tmp.filePath("cpp")));
        both.nVerticesLh = 4;
        QVERIFY(both.writeHemispherePair(tmp.filePath("cpp-rh.stc")));
        QCOMPARE(bytes(tmp.filePath("cpp-lh.stc")), bytes(dir + "/py-lh.stc"));
        QCOMPARE(bytes(tmp.filePath("cpp-rh.stc")), bytes(dir + "/py-rh.stc"));
        InvSourceEstimate(ref.col(0).tail(3), verts.tail(3), 0.0f, 0.0f).write_w(tmp.filePath("cpp-rh.w"));
        QCOMPARE(bytes(tmp.filePath("cpp-rh.w")), bytes(dir + "/pyw-rh.w"));

        InvSourceEstimate bad(ref, verts.head(4), 0.0f, 1.0f);
        bad.nVerticesLh = 2;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("data has 7 rows"));
        QVERIFY(!bad.writeHemispherePair(tmp.filePath("bad")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Cannot open file"));
        QVERIFY(InvSourceEstimate::read_w(tmp.filePath("missing.w")).isEmpty());
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Source estimate is empty"));
        InvSourceEstimate().write_w(tmp.filePath("empty.w"));
        QVERIFY(!QFile::exists(tmp.filePath("empty.w")));

        // Label lookup: by vertex and hemisphere (rh starts where vertex numbers drop), or by cluster id.
        FsLabel lhLabel;
        lhLabel.hemi = 0;
        lhLabel.vertices = VectorXi(3);
        lhLabel.vertices << 17, 42, 99;
        FsLabel rhLabel;
        rhLabel.hemi = 1;
        rhLabel.vertices = VectorXi(2);
        rhLabel.vertices << 70000, 3;
        VectorXi want(3);
        want << 1, 2, 6;
        QCOMPARE(both.getIndicesByLabel({lhLabel, rhLabel}, false), want);
        lhLabel.label_id = 42;
        want.resize(1);
        want << 2;
        QCOMPARE(both.getIndicesByLabel({lhLabel}, true), want);
        QVERIFY(both.getIndicesByLabel({}, false).size() == 0);
    }
};

QTEST_GUILESS_MAIN(TestSourceEstimateIO)
#include "test_source_estimate_io.moc"
