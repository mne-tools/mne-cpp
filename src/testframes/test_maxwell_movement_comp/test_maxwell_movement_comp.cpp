//=============================================================================================================
/**
 * @file     test_maxwell_movement_comp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for MaxwellMovementComp class.
 */

#include <dsp/maxwell_movement_comp.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>

using namespace UTILSLIB;
using namespace Eigen;

class TestMaxwellMovementComp : public QObject
{
    Q_OBJECT

private slots:
    void testHeadPosWriteAndRead()
    {
        QList<HeadPosEntry> positions;
        for (int i = 0; i < 5; ++i) {
            HeadPosEntry entry;
            entry.dTime = 0.1 * i;
            entry.translation = Vector3d(0.001 * i, 0.002, 0.003);
            entry.rotation = Quaterniond(AngleAxisd(0.01 * i, Vector3d::UnitZ()));
            entry.dGof = 0.95 + 0.01 * i;
            positions.append(entry);
        }

        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString path = tmpFile.fileName();
        tmpFile.close();

        QVERIFY(MaxwellMovementComp::writeHeadPos(path, positions));

        QList<HeadPosEntry> loaded = MaxwellMovementComp::readHeadPos(path);
        QCOMPARE(loaded.size(), 5);

        // Check first entry
        QVERIFY(std::abs(loaded[0].dTime - 0.0) < 1e-5);
        QVERIFY(std::abs(loaded[0].translation.y() - 0.002) < 1e-5);
    }

    void testReadNonexistent()
    {
        QList<HeadPosEntry> pos = MaxwellMovementComp::readHeadPos("/nonexistent/path.pos");
        QVERIFY(pos.isEmpty());
    }

    void testHeadPosRoundTrip()
    {
        HeadPosEntry entry;
        entry.dTime = 1.5;
        entry.translation = Vector3d(0.01, -0.02, 0.03);
        entry.rotation = Quaterniond(AngleAxisd(0.1, Vector3d::UnitX()));
        entry.dGof = 0.99;

        QList<HeadPosEntry> positions;
        positions.append(entry);

        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString path = tmpFile.fileName();
        tmpFile.close();

        MaxwellMovementComp::writeHeadPos(path, positions);
        QList<HeadPosEntry> loaded = MaxwellMovementComp::readHeadPos(path);

        QCOMPARE(loaded.size(), 1);
        QVERIFY(std::abs(loaded[0].dTime - 1.5) < 1e-5);
        QVERIFY(std::abs(loaded[0].translation.x() - 0.01) < 1e-5);
        QVERIFY(std::abs(loaded[0].dGof - 0.99) < 1e-5);
    }

    void testEmptyHeadPos()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        tmpFile.write("# empty\n");
        tmpFile.close();

        QList<HeadPosEntry> pos = MaxwellMovementComp::readHeadPos(tmpFile.fileName());
        QVERIFY(pos.isEmpty());
    }

    void testWriteFailsOnBadPath()
    {
        QList<HeadPosEntry> pos;
        QVERIFY(!MaxwellMovementComp::writeHeadPos("/nonexistent/dir/file.pos", pos));
    }

    void testHeadPosEntryDefaults()
    {
        HeadPosEntry entry;
        QVERIFY(std::abs(entry.dTime) < 1e-10);
        QVERIFY(entry.translation.norm() < 1e-10);
        QVERIFY(std::abs(entry.rotation.w() - 1.0) < 1e-10);
    }

    void testMaxwellMoveCompParamsDefaults()
    {
        MaxwellMoveCompParams params;
        QCOMPARE(params.iOrderIn, 8);
        QCOMPARE(params.iOrderOut, 3);
        QCOMPARE(params.iRefIdx, 0);
        QVERIFY(std::abs(params.origin.z() - 0.04) < 1e-10);
    }
};

QTEST_GUILESS_MAIN(TestMaxwellMovementComp)
#include "test_maxwell_movement_comp.moc"
