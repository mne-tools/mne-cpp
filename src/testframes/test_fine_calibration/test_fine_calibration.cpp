//=============================================================================================================
/**
 * @file     test_fine_calibration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * @brief    Tests for FineCalibration class.
 */

#include <dsp/fine_calibration.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryFile>
#include <Eigen/Core>
#include <cmath>

using namespace UTILSLIB;
using namespace Eigen;

class TestFineCalibration : public QObject
{
    Q_OBJECT

private slots:
    void testAddAndSize()
    {
        FineCalibration cal;
        QVERIFY(cal.isEmpty());
        QCOMPARE(cal.size(), 0);

        FineCalEntry entry;
        entry.chNumber = 113;
        entry.dGain = 1.001;
        entry.imbalance = Vector3d(0.001, -0.002, 0.003);
        cal.addEntry(entry);

        QCOMPARE(cal.size(), 1);
        QVERIFY(!cal.isEmpty());
    }

    void testFindEntry()
    {
        FineCalibration cal;
        FineCalEntry e1;
        e1.chNumber = 113;
        e1.dGain = 1.05;
        e1.imbalance = Vector3d(0.01, 0.02, 0.03);
        cal.addEntry(e1);

        FineCalEntry e2;
        e2.chNumber = 211;
        e2.dGain = 0.99;
        e2.imbalance = Vector3d(-0.01, 0.0, 0.01);
        cal.addEntry(e2);

        FineCalEntry found;
        QVERIFY(cal.findEntry(211, found));
        QCOMPARE(found.chNumber, 211);
        QVERIFY(std::abs(found.dGain - 0.99) < 1e-10);

        QVERIFY(!cal.findEntry(999, found));
    }

    void testGainVector()
    {
        FineCalibration cal;
        for (int i = 0; i < 5; ++i) {
            FineCalEntry e;
            e.chNumber = i;
            e.dGain = 1.0 + 0.01 * i;
            cal.addEntry(e);
        }

        VectorXd gains = cal.gainVector();
        QCOMPARE(gains.size(), 5);
        QVERIFY(std::abs(gains(0) - 1.00) < 1e-10);
        QVERIFY(std::abs(gains(4) - 1.04) < 1e-10);
    }

    void testImbalanceMatrix()
    {
        FineCalibration cal;
        FineCalEntry e;
        e.chNumber = 1;
        e.dGain = 1.0;
        e.imbalance = Vector3d(0.1, 0.2, 0.3);
        cal.addEntry(e);

        MatrixXd imb = cal.imbalanceMatrix();
        QCOMPARE(imb.rows(), static_cast<Index>(1));
        QCOMPARE(imb.cols(), static_cast<Index>(3));
        QVERIFY(std::abs(imb(0, 0) - 0.1) < 1e-10);
        QVERIFY(std::abs(imb(0, 2) - 0.3) < 1e-10);
    }

    void testWriteAndRead()
    {
        FineCalibration cal;
        for (int i = 0; i < 3; ++i) {
            FineCalEntry e;
            e.chNumber = 100 + i;
            e.dGain = 1.0 + 0.001 * i;
            e.imbalance = Vector3d(0.001 * i, -0.002 * i, 0.003 * i);
            cal.addEntry(e);
        }

        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString path = tmpFile.fileName();
        tmpFile.close();

        QVERIFY(cal.write(path));

        FineCalibration loaded = FineCalibration::read(path);
        QCOMPARE(loaded.size(), 3);

        FineCalEntry found;
        QVERIFY(loaded.findEntry(101, found));
        QVERIFY(std::abs(found.dGain - 1.001) < 1e-5);
        QVERIFY(std::abs(found.imbalance.x() - 0.001) < 1e-5);
    }

    void testReadEmpty()
    {
        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        tmpFile.write("# comment only\n");
        tmpFile.write("\n");
        tmpFile.close();

        FineCalibration cal = FineCalibration::read(tmpFile.fileName());
        QVERIFY(cal.isEmpty());
    }

    void testReadNonexistent()
    {
        FineCalibration cal = FineCalibration::read("/nonexistent/path/file.dat");
        QVERIFY(cal.isEmpty());
    }

    void testRoundTripPreservesValues()
    {
        FineCalibration cal;
        FineCalEntry e;
        e.chNumber = 42;
        e.dGain = 0.998765;
        e.imbalance = Vector3d(1.23e-4, -4.56e-5, 7.89e-3);
        cal.addEntry(e);

        QTemporaryFile tmpFile;
        tmpFile.setAutoRemove(true);
        QVERIFY(tmpFile.open());
        QString path = tmpFile.fileName();
        tmpFile.close();

        cal.write(path);
        FineCalibration loaded = FineCalibration::read(path);

        FineCalEntry found;
        QVERIFY(loaded.findEntry(42, found));
        QVERIFY(std::abs(found.dGain - 0.998765) < 1e-5);
        QVERIFY(std::abs(found.imbalance.x() - 1.23e-4) < 1e-7);
    }
};

QTEST_GUILESS_MAIN(TestFineCalibration)
#include "test_fine_calibration.moc"
