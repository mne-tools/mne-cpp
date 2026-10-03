//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_source_space_ops_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Cross validates MNESourceSpace patch statistics, volume grids and write round trips against mne-python.
 *
 * test_mne_source_space_python covers reading. This test covers what MNE-CPP
 * computes from a source space: cortical patch statistics (used by the
 * cortical-patch orientation prior), volume source spaces bounded by a BEM
 * surface, and writing a source space back. Expected values come from
 * mne-python on the same files:
 *
 *   src = mne.read_source_spaces('sample-oct-6-src.fif', patch_stats=True)
 *   len(pinfo[i]), sum of vertex areas (1/3 of adjacent triangles) over each
 *   patch, the normalised mean normal and mean angle of member normals to it
 *
 *   s = read_bem_surfaces('sample-5120-bem.fif')[0]; s['rr'] *= 1e3   # mm
 *   setup_volume_source_space(pos=10.0, surface=s, mindist=5.0, exclude=ex)
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_space.h>
#include <mne/mne_surface.h>
#include <mne/mne_patch_info.h>

#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>

#include <cmath>
#include <memory>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

namespace
{

bool closeTo(double actual, double expected, double rel)
{
    return std::abs(actual - expected) <= rel * std::abs(expected);
}

} // namespace

//=============================================================================================================
/**
 * Cross validates MNESourceSpace operations against mne-python.
 */
class TestMneSourceSpaceOpsPython : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void patchStatistics_data();
    void patchStatistics();
    void volumeGrid_data();
    void volumeGrid();
    void writeRoundTrip();

private:
    QString m_srcPath;
    QString m_bemPath;
    QTemporaryDir m_dir;
    std::vector<std::unique_ptr<MNESourceSpace>> m_spaces;
};

//=============================================================================================================

void TestMneSourceSpaceOpsPython::initTestCase()
{
    const QString base = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/subjects/sample/bem";
    m_srcPath = base + "/sample-oct-6-src.fif";
    m_bemPath = base + "/sample-5120-bem.fif";
    QVERIFY2(QFile::exists(m_srcPath) && QFile::exists(m_bemPath), "test data missing");
    QVERIFY(m_dir.isValid());
    QCOMPARE(MNESourceSpace::read_source_spaces(m_srcPath, m_spaces), 0);
    QCOMPARE(static_cast<int>(m_spaces.size()), 2);
}

//=============================================================================================================

void TestMneSourceSpaceOpsPython::patchStatistics_data()
{
    QTest::addColumn<int>("hemi");
    QTest::addColumn<int>("memberSum");
    QTest::addColumn<int>("memberMax");
    QTest::addColumn<double>("areaSum");
    QTest::addColumn<double>("area0");
    QTest::addColumn<Vector3d>("aveNn0");
    QTest::addColumn<double>("devMean");

    QTest::newRow("lh") << 0 << 155407 << 119 << 0.10062765177509901 << 3.6272687673290817e-05
                        << Vector3d(0.4407139718532562, -0.8963643908500671, 0.04797911271452904) << 0.4507984220981598;
    QTest::newRow("rh") << 1 << 156866 << 120 << 0.10180982473617825 << 3.594727340331317e-05
                        << Vector3d(0.18588027358055115, -0.7848595380783081, 0.5911378860473633) << 0.45684412121772766;
}

void TestMneSourceSpaceOpsPython::patchStatistics()
{
    QFETCH(int, hemi);
    QFETCH(int, memberSum);
    QFETCH(int, memberMax);
    QFETCH(double, areaSum);
    QFETCH(double, area0);
    QFETCH(Vector3d, aveNn0);
    QFETCH(double, devMean);

    MNESourceSpace& s = *m_spaces[hemi];
    QCOMPARE(s.add_patch_stats(), 0);
    QCOMPARE(static_cast<int>(s.patches.size()), s.nuse);

    int members = 0;
    int largest = 0;
    double area = 0.0;
    double dev = 0.0;
    for (int k = 0; k < s.nuse; ++k) {
        QVERIFY(s.patches[k].has_value());
        const MNEPatchInfo& p = *s.patches[k];
        QCOMPARE(p.vert, s.vertno[k]);
        members += static_cast<int>(p.memb_vert.size());
        largest = std::max(largest, static_cast<int>(p.memb_vert.size()));
        area += p.area;
        dev += p.dev_nn;
    }
    // Every vertex of the surface belongs to exactly one patch.
    QCOMPARE(members, s.np);
    QCOMPARE(members, memberSum);
    QCOMPARE(largest, memberMax);
    QVERIFY2(closeTo(area, areaSum, 1e-4), qPrintable(QStringLiteral("area sum %1").arg(area, 0, 'g', 10)));
    QVERIFY2(closeTo(s.patches[0]->area, area0, 1e-4), qPrintable(QStringLiteral("area0 %1").arg(s.patches[0]->area, 0, 'g', 10)));
    const Vector3d ave(s.patches[0]->ave_nn[0], s.patches[0]->ave_nn[1], s.patches[0]->ave_nn[2]);
    QVERIFY2((ave - aveNn0).norm() < 1e-5, qPrintable(QStringLiteral("ave_nn %1 %2 %3").arg(ave[0]).arg(ave[1]).arg(ave[2])));
    QVERIFY2(closeTo(dev / s.nuse, devMean, 1e-4), qPrintable(QStringLiteral("dev mean %1").arg(dev / s.nuse, 0, 'g', 10)));
}

//=============================================================================================================

void TestMneSourceSpaceOpsPython::volumeGrid_data()
{
    QTest::addColumn<double>("exclude");
    QTest::addColumn<int>("nuse");
    QTest::addColumn<int>("vertnoSum");
    QTest::addColumn<double>("rrUsedSum");

    QTest::newRow("no exclusion") << 0.0 << 1306 << 3227661 << 31.28;
    QTest::newRow("exclude 30 mm") << 0.03 << 1194 << 2926151 << 27.3;
}

void TestMneSourceSpaceOpsPython::volumeGrid()
{
    QFETCH(double, exclude);
    QFETCH(int, nuse);
    QFETCH(int, vertnoSum);
    QFETCH(double, rrUsedSum);

    std::unique_ptr<MNESurface> surf = MNESurface::read_bem_surface(m_bemPath, FIFFV_BEM_SURF_ID_BRAIN, true);
    QVERIFY(surf);
    QCOMPARE(surf->np, 2562);

    std::unique_ptr<MNESourceSpace> vol(MNESourceSpace::make_volume_source_space(*surf, 0.01f, static_cast<float>(exclude), 0.005f));
    QVERIFY(vol);
    QCOMPARE(vol->np, 4590);
    QCOMPARE(vol->vol_dims[0], 15);
    QCOMPARE(vol->vol_dims[1], 18);
    QCOMPARE(vol->vol_dims[2], 17);
    QVERIFY((vol->rr.row(0).cast<double>() - RowVector3d(-0.07, -0.09, -0.05)).norm() < 1e-6);
    QVERIFY((vol->rr.row(vol->np - 1).cast<double>() - RowVector3d(0.07, 0.08, 0.11)).norm() < 1e-6);

    QCOMPARE(vol->nuse, nuse);
    QCOMPARE(vol->vertno.sum(), vertnoSum);
    double used = 0.0;
    for (int k = 0; k < vol->nuse; ++k)
        used += vol->rr.row(vol->vertno[k]).cast<double>().sum();
    QVERIFY2(closeTo(used, rrUsedSum, 1e-5), qPrintable(QStringLiteral("rr used sum %1").arg(used, 0, 'g', 10)));
}

//=============================================================================================================

void TestMneSourceSpaceOpsPython::writeRoundTrip()
{
    const QString path = m_dir.filePath("roundtrip-src.fif");
    {
        QFile file(path);
        FiffStream::SPtr stream = FiffStream::start_file(file);
        QVERIFY(stream);
        for (const auto& s : m_spaces)
            QCOMPARE(s->writeToStream(stream, false), 0);
        stream->end_file();
    }

    std::vector<std::unique_ptr<MNESourceSpace>> back;
    QCOMPARE(MNESourceSpace::read_source_spaces(path, back), 0);
    QCOMPARE(static_cast<int>(back.size()), 2);
    for (int h = 0; h < 2; ++h) {
        const MNESourceSpace& a = *m_spaces[h];
        const MNESourceSpace& b = *back[h];
        QCOMPARE(b.np, a.np);
        QCOMPARE(b.nuse, a.nuse);
        QCOMPARE(b.ntri, a.ntri);
        QCOMPARE(b.id, a.id);
        QCOMPARE(b.coord_frame, a.coord_frame);
        QCOMPARE(b.vertno, a.vertno);
        QVERIFY((b.rr - a.rr).cwiseAbs().maxCoeff() < 1e-7f);
        QVERIFY((b.nn - a.nn).cwiseAbs().maxCoeff() < 1e-6f);
    }

    // Writing only the selected points keeps exactly the used vertices.
    const QString selPath = m_dir.filePath("selected-src.fif");
    {
        QFile file(selPath);
        FiffStream::SPtr stream = FiffStream::start_file(file);
        QVERIFY(stream);
        QCOMPARE(m_spaces[0]->writeToStream(stream, true), 0);
        stream->end_file();
    }
    std::vector<std::unique_ptr<MNESourceSpace>> sel;
    QCOMPARE(MNESourceSpace::read_source_spaces(selPath, sel), 0);
    QCOMPARE(static_cast<int>(sel.size()), 1);
    QCOMPARE(sel[0]->np, m_spaces[0]->nuse);
    // Like MNE-C, the point list carries no selection, so a surface space reads back with none in use.
    QCOMPARE(sel[0]->nuse, 0);
    for (int k = 0; k < sel[0]->np; ++k)
        QVERIFY((sel[0]->rr.row(k) - m_spaces[0]->rr.row(m_spaces[0]->vertno[k])).cwiseAbs().maxCoeff() < 1e-7f);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneSourceSpaceOpsPython)
#include "test_mne_source_space_ops_python.moc"
