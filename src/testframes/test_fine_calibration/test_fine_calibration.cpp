//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fine_calibration.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
    void testReadMatchesMne()
    {
        // First lines of MNE-sample-data/SSS/sss_cal_mgh.dat, plus a 3-term gradiometer line and a malformed one
        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write("# comment\n"
                      "113 -0.106600 0.046400 -0.060400 -0.012700 0.005700 -0.999903 -0.186801 -0.982403 -0.003300 -0.981020 0.192489 0.022924 -0.008282\n"
                      "111 -0.106600 0.046400 -0.060400 -0.012700 0.005700 -0.999903 -0.186801 -0.982403 -0.003300 -0.981020 0.192489 0.022924 0.996645\n"
                      "MEG2643 0.101700 -0.036100 -0.027800 0.341491 0.939874 -0.004500 0.027699 -0.005300 0.999573 0.942189 -0.333982 -0.026877 0.001 -0.002 0.003\n"
                      "211 1.0 0.0 0.0 0.0\n");
        tmpFile.close();

        // mne.preprocessing.read_fine_calibration: locs[:3] position, locs[9:12] coil normal, imb_cals
        FineCalibration cal = FineCalibration::read(tmpFile.fileName());
        QCOMPARE(cal.size(), 3);
        FineCalEntry grad;
        QVERIFY(cal.findEntry(113, grad));
        QVERIFY((grad.position - Vector3d(-0.1066, 0.0464, -0.0604)).norm() < 1e-12);
        QVERIFY((grad.orientation.row(2).transpose() - Vector3d(-0.98102, 0.192489, 0.022924)).norm() < 1e-12);
        QCOMPARE(grad.imbalance.size(), static_cast<Index>(1));
        QVERIFY(std::abs(grad.imbalance(0) + 0.008282) < 1e-12);

        FineCalEntry grad3;
        QVERIFY(cal.findEntry(2643, grad3));
        QCOMPARE(grad3.imbalance.size(), static_cast<Index>(3));

        // Magnetometers (numbers ending in 1) contribute their calibration, gradiometers their imbalance
        QVERIFY((cal.gainVector() - Vector3d(1.0, 0.996645, 1.0)).norm() < 1e-12);
        const MatrixXd imb = cal.imbalanceMatrix();
        QVERIFY(std::abs(imb(0, 0) + 0.008282) < 1e-12 && (imb.row(1).array() == 0.0).all() && std::abs(imb(2, 2) - 0.003) < 1e-12);
    }

    void testWriteAndRead()
    {
        FineCalibration cal;
        FineCalEntry e;
        e.chNumber = 113;
        e.position = Vector3d(-0.1066, 0.0464, -0.0604);
        e.orientation.row(2) = Vector3d(-0.98102, 0.192489, 0.022924).transpose();
        e.imbalance = Vector3d(0.001, -0.002, 0.003);
        cal.addEntry(e);

        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        const QString path = tmpFile.fileName();
        tmpFile.close();
        QVERIFY(cal.write(path));

        // Same line layout as mne.preprocessing.write_fine_calibration (number zero-padded, values %0.6f)
        QFile written(path);
        QVERIFY(written.open(QIODevice::ReadOnly));
        QCOMPARE(QString(written.readLine()).trimmed(),
                 QStringLiteral("0113 -0.106600 0.046400 -0.060400 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 -0.981020 0.192489 0.022924 0.001000 -0.002000 0.003000"));

        FineCalEntry found;
        QVERIFY(FineCalibration::read(path).findEntry(113, found));
        QVERIFY((found.position - e.position).norm() < 1e-9 && (found.imbalance - e.imbalance).norm() < 1e-9 && (found.orientation - e.orientation).norm() < 1e-9);
    }

    void testReadEmptyOrMissing()
    {
        QTemporaryFile tmpFile;
        QVERIFY(tmpFile.open());
        tmpFile.write("# comment only\n\n");
        tmpFile.close();
        QVERIFY(FineCalibration::read(tmpFile.fileName()).isEmpty());
        QVERIFY(FineCalibration::read("/nonexistent/path/file.dat").isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestFineCalibration)
#include "test_fine_calibration.moc"
