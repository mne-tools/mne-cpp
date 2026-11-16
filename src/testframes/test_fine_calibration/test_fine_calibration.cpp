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

#include <base/byte_stream.h>
#include <dsp/fine_calibration.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <Eigen/Core>
#include <cmath>
#include <filesystem>
#include <string>

using namespace mne;
using namespace Eigen;

class TestFineCalibration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_path = m_dir.filePath(QStringLiteral("sss_cal.dat")).toStdString();
    }

    void testReadMatchesMne()
    {
        // First lines of MNE-sample-data/SSS/sss_cal_mgh.dat, plus a 3-term gradiometer line
        QVERIFY(write_text_file(
            m_path,
            "# comment\n"
            "113\t-0.106600 0.046400 -0.060400 -0.012700 0.005700 -0.999903 -0.186801 -0.982403 -0.003300 -0.981020 0.192489 0.022924 -0.008282\n"
            "111 -0.106600 0.046400 -0.060400 -0.012700 0.005700 -0.999903 -0.186801 -0.982403 -0.003300 -0.981020 0.192489 0.022924 0.996645\n"
            "MEG2643 0.101700 -0.036100 -0.027800 0.341491 0.939874 -0.004500 0.027699 -0.005300 0.999573 0.942189 -0.333982 -0.026877 0.001 -0.002 0.003\n"));

        // mne.preprocessing.read_fine_calibration: locs[:3] position, locs[9:12] coil normal, imb_cals
        const auto cal = FineCalibration::read(m_path);
        QVERIFY(cal);
        QCOMPARE(cal->size(), 3);
        const FineCalEntry* grad = cal->find(113);
        QVERIFY(grad);
        QVERIFY((grad->position - Vector3d(-0.1066, 0.0464, -0.0604)).norm() < 1e-12);
        QVERIFY((grad->orientation.row(2).transpose() - Vector3d(-0.98102, 0.192489, 0.022924)).norm() < 1e-12);
        QCOMPARE(grad->imbalance.size(), static_cast<Index>(1));
        QVERIFY(std::abs(grad->imbalance(0) + 0.008282) < 1e-12);
        QCOMPARE(cal->find(2643)->imbalance.size(), static_cast<Index>(3));
        QVERIFY(!cal->find(2642));

        // Magnetometers (numbers ending in 1) contribute their calibration, gradiometers their imbalance
        QVERIFY((cal->gain_vector() - Vector3d(1.0, 0.996645, 1.0)).norm() < 1e-12);
        const MatrixXd imb = cal->imbalance_matrix();
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
        cal.push_back(e);
        QVERIFY(cal.write(m_path));

        // Like MaxFilter and mne.preprocessing.write_fine_calibration: bare channel number, values %0.6f
        QCOMPARE(read_text_file(m_path).value(),
                 std::string("113 -0.106600 0.046400 -0.060400 1.000000 0.000000 0.000000 0.000000 1.000000 0.000000 -0.981020 0.192489 0.022924 0.001000 -0.002000 0.003000\n"));

        const FineCalibration back = FineCalibration::read(m_path).value();
        const FineCalEntry* found = back.find(113);
        QVERIFY(found);
        QVERIFY((found->position - e.position).norm() < 1e-9 && (found->imbalance - e.imbalance).norm() < 1e-9 && (found->orientation - e.orientation).norm() < 1e-9);
    }

    void testReadRejectsWhatMneRejects()
    {
        QVERIFY(write_text_file(m_path, "# comment only\n\n"));
        QVERIFY(FineCalibration::read(m_path).value().empty());
        QVERIFY(write_text_file(m_path, "211 1.0 0.0 0.0 0.0\n"));
        QVERIFY(!FineCalibration::read(m_path));
        QVERIFY(!FineCalibration::read("/nonexistent/path/file.dat"));
        QVERIFY(!FineCalibration().write("/nonexistent/dir/file.dat"));
    }

private:
    QTemporaryDir m_dir;
    std::filesystem::path m_path;
};

QTEST_GUILESS_MAIN(TestFineCalibration)
#include "test_fine_calibration.moc"
