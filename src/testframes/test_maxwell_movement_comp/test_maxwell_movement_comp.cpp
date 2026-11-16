//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_maxwell_movement_comp.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for MaxwellMovementComp class.
 */

#include <base/byte_stream.h>
#include <dsp/maxwell_movement_comp.h>

#include <QtTest>
#include <QObject>
#include <QTemporaryDir>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

using namespace mne;
using namespace Eigen;

class TestMaxwellMovementComp : public QObject
{
    Q_OBJECT

private slots:
    void testHeadPosReadsAndWritesMaxFilterFiles()
    {
        // First lines of MNE-testing-data SSS/test_move_anon_raw_subsampled.pos (written by MaxFilter)
        const std::string maxfilter =
            " Time       q1       q2       q3       q4       q5       q6       g-value  error    velocity\n"
            "     9.000  0.07303  0.01133  0.03979  0.00747 -0.01983  0.07468  0.99955  0.00139  0.00177 \n"
            "    14.620  0.08603  0.01049  0.04267  0.00733 -0.02296  0.07470  0.99942  0.00138  0.00078 \n"
            "    14.820  0.12113  0.00887  0.04841  0.00723 -0.02991  0.07400  0.99913  0.00117  0.06945 \n";
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const std::filesystem::path in = dir.filePath(QStringLiteral("in.pos")).toStdString();
        const std::filesystem::path out = dir.filePath(QStringLiteral("out.pos")).toStdString();
        QVERIFY(write_text_file(in, maxfilter));

        const auto positions = MaxwellMovementComp::read_head_pos(in);
        QVERIFY(positions);
        QCOMPARE(positions->size(), std::size_t(3));
        const HeadPosEntry& first = positions->front();
        QCOMPARE(first.dTime, 9.0);
        QCOMPARE(first.rotation.vec(), Vector3d(0.07303, 0.01133, 0.03979));
        QVERIFY(std::abs(first.rotation.norm() - 1.0) < 1e-12);
        QCOMPARE(first.translation, Vector3d(0.00747, -0.01983, 0.07468));
        QCOMPARE(first.dGof, 0.99955);
        QCOMPARE(first.dError, 0.00139);
        QCOMPARE(first.dVelocity, 0.00177);

        // Written exactly like mne.chpi.write_head_pos
        QVERIFY(MaxwellMovementComp::write_head_pos(out, *positions));
        QCOMPARE(read_text_file(out).value(),
                 std::string(" Time       q1       q2       q3       q4       q5       q6       g-value  error    velocity\n"
                             "     9.000  0.07303  0.01133  0.03979  0.00747 -0.01983  0.07468  0.99955  0.00139  0.00177\n"
                             "    14.620  0.08603  0.01049  0.04267  0.00733 -0.02296  0.07470  0.99942  0.00138  0.00078\n"
                             "    14.820  0.12113  0.00887  0.04841  0.00723 -0.02991  0.07400  0.99913  0.00117  0.06945\n"));

        // A quaternion with a negative scalar part is the same rotation as its negation, which is what the file holds
        HeadPosEntry turned;
        turned.rotation = Quaterniond(-0.6, 0.0, 0.0, 0.8);
        QVERIFY(MaxwellMovementComp::write_head_pos(out, std::vector{turned}));
        const auto back = MaxwellMovementComp::read_head_pos(out);
        QVERIFY(back && back->size() == 1);
        QVERIFY(back->front().rotation.angularDistance(turned.rotation) < 1e-4);

        QVERIFY(write_text_file(in, "# no positions\n"));
        QVERIFY(MaxwellMovementComp::read_head_pos(in).value().empty());
        QVERIFY(write_text_file(in, std::string(maxfilter) + "    15.000  0.1  oops\n"));
        QVERIFY(!MaxwellMovementComp::read_head_pos(in));
        QVERIFY(!MaxwellMovementComp::read_head_pos("/nonexistent/path.pos"));
        QVERIFY(!MaxwellMovementComp::write_head_pos("/nonexistent/dir/file.pos", {}));
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
