//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_source_space_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Checks MNESourceSpace against values read by mne-python.
 *
 * The existing source space test asserts that np and nuse are greater than
 * zero and discards the return of find_source_space_hemi, so it would pass
 * with every coordinate wrong. The expectations here instead come from
 * mne.read_source_spaces on the same file, an independent implementation, so
 * a passing run is evidence that MNE-CPP reads the geometry correctly rather
 * than merely consistently.
 *
 * Reference values were produced with:
 *
 *   import mne
 *   src = mne.read_source_spaces('sample-oct-6-src.fif')
 *   for s in src:
 *       print(s['id'], s['np'], s['ntri'], s['nuse'], s['coord_frame'],
 *             s['rr'][0], s['rr'].sum(), s['nn'][0],
 *             s['vertno'][:5], s['vertno'].sum())
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_space.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QtTest>

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

//=============================================================================================================
/**
 * DECLARE CLASS TestMneSourceSpacePython
 *
 * @brief Compares MNESourceSpace against mne-python reference values.
 */
class TestMneSourceSpacePython : public QObject
{
    Q_OBJECT

public:
    TestMneSourceSpacePython() = default;

private:
    static QString srcPath();

    std::vector<std::unique_ptr<MNESourceSpace>> m_spaces;

private slots:
    void initTestCase();

    void geometry_matchesPython_data();
    void geometry_matchesPython();

    void firstVertex_matchesPython_data();
    void firstVertex_matchesPython();

    void hemisphereIdentification_data();
    void hemisphereIdentification();

    void transform_isRigidAndUpdatesFrame();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestMneSourceSpacePython::srcPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
}

//=============================================================================================================

void TestMneSourceSpacePython::initTestCase()
{
    if(!QFile::exists(srcPath())) {
        QSKIP("Source space test data not found");
    }

    const int res = MNESourceSpace::read_source_spaces(srcPath(), m_spaces);
    QVERIFY2(res != FIFF_FAIL, "read_source_spaces reported failure");

    // mne-python reports two hemispheres for this file. Getting a different
    // number means the rest of the comparison is meaningless.
    QCOMPARE(static_cast<int>(m_spaces.size()), 2);
}

//=============================================================================================================

void TestMneSourceSpacePython::geometry_matchesPython_data()
{
    QTest::addColumn<int>("hemi");
    QTest::addColumn<int>("id");
    QTest::addColumn<int>("np");
    QTest::addColumn<int>("ntri");
    QTest::addColumn<int>("nuse");
    QTest::addColumn<int>("coordFrame");
    QTest::addColumn<int>("vertnoSum");

    // From mne.read_source_spaces. FIFFV_MNE_SURF_LEFT_HEMI is 101 and
    // FIFFV_MNE_SURF_RIGHT_HEMI is 102; coord frame 5 is FIFFV_COORD_MRI.
    QTest::newRow("left")  << 0 << 101 << 155407 << 310810 << 4098 << 5 << 318245580;
    QTest::newRow("right") << 1 << 102 << 156866 << 313728 << 4098 << 5 << 322212481;
}

//=============================================================================================================

void TestMneSourceSpacePython::geometry_matchesPython()
{
    QFETCH(int, hemi);
    QFETCH(int, id);
    QFETCH(int, np);
    QFETCH(int, ntri);
    QFETCH(int, nuse);
    QFETCH(int, coordFrame);
    QFETCH(int, vertnoSum);

    const MNESourceSpace& s = *m_spaces.at(static_cast<size_t>(hemi));

    QCOMPARE(s.id, id);
    QCOMPARE(s.np, np);
    QCOMPARE(s.ntri, ntri);
    QCOMPARE(s.nuse, nuse);
    QCOMPARE(s.coord_frame, coordFrame);

    // Sizes have to follow from the counts, otherwise a later index is unsafe.
    QCOMPARE(static_cast<int>(s.rr.rows()), np);
    QCOMPARE(static_cast<int>(s.nn.rows()), np);
    QCOMPARE(static_cast<int>(s.vertno.size()), nuse);
    QCOMPARE(static_cast<int>(s.inuse.size()), np);

    // The sum pins every entry of vertno at once. Comparing only the first few
    // would miss a reader that goes wrong partway through the list.
    QCOMPARE(static_cast<int>(s.vertno.sum()), vertnoSum);

    // inuse is the boolean form of vertno, so the two have to agree.
    QCOMPARE(static_cast<int>(s.inuse.sum()), nuse);
    for(int i = 0; i < s.vertno.size(); ++i) {
        QVERIFY2(s.inuse(s.vertno(i)) != 0,
                 qPrintable(QString("vertex %1 is in vertno but not marked in inuse")
                            .arg(s.vertno(i))));
    }
}

//=============================================================================================================

void TestMneSourceSpacePython::firstVertex_matchesPython_data()
{
    QTest::addColumn<int>("hemi");
    QTest::addColumn<double>("rrX");
    QTest::addColumn<double>("rrY");
    QTest::addColumn<double>("rrZ");
    QTest::addColumn<double>("nnX");
    QTest::addColumn<double>("nnY");
    QTest::addColumn<double>("nnZ");
    QTest::addColumn<double>("rrSum");

    // Positions are in metres and normals are unit length, both straight from
    // mne-python. rrSum covers all 155407 or 156866 vertices at once, so a
    // reader that mangles anything after the first row still fails.
    QTest::newRow("left")
        << 0
        << -0.02095193 << -0.086276777 << 0.013240334
        << -0.018733552 << -0.9565033 << 0.29111955
        << 153.167290269;
    QTest::newRow("right")
        << 1
        << 0.017825179 << -0.085788988 << 0.008352406
        << 0.20627107 << -0.937107 << 0.2815719
        << 8699.537465011;
}

//=============================================================================================================

void TestMneSourceSpacePython::firstVertex_matchesPython()
{
    QFETCH(int, hemi);
    QFETCH(double, rrX);
    QFETCH(double, rrY);
    QFETCH(double, rrZ);
    QFETCH(double, nnX);
    QFETCH(double, nnY);
    QFETCH(double, nnZ);
    QFETCH(double, rrSum);

    const MNESourceSpace& s = *m_spaces.at(static_cast<size_t>(hemi));

    // The file stores single precision, so the tolerance is set by float, not
    // by the algorithm.
    const double eps = 1.0e-6;

    QVERIFY2(std::fabs(s.rr(0, 0) - rrX) < eps,
             qPrintable(QString("rr[0].x is %1, mne-python says %2").arg(s.rr(0, 0)).arg(rrX)));
    QVERIFY2(std::fabs(s.rr(0, 1) - rrY) < eps,
             qPrintable(QString("rr[0].y is %1, mne-python says %2").arg(s.rr(0, 1)).arg(rrY)));
    QVERIFY2(std::fabs(s.rr(0, 2) - rrZ) < eps,
             qPrintable(QString("rr[0].z is %1, mne-python says %2").arg(s.rr(0, 2)).arg(rrZ)));

    QVERIFY2(std::fabs(s.nn(0, 0) - nnX) < eps,
             qPrintable(QString("nn[0].x is %1, mne-python says %2").arg(s.nn(0, 0)).arg(nnX)));
    QVERIFY2(std::fabs(s.nn(0, 1) - nnY) < eps,
             qPrintable(QString("nn[0].y is %1, mne-python says %2").arg(s.nn(0, 1)).arg(nnY)));
    QVERIFY2(std::fabs(s.nn(0, 2) - nnZ) < eps,
             qPrintable(QString("nn[0].z is %1, mne-python says %2").arg(s.nn(0, 2)).arg(nnZ)));

    // Accumulating 150k single precision values leaves more room than a single
    // coordinate does, so the tolerance is relative to the magnitude.
    const double sum = s.rr.cast<double>().sum();
    QVERIFY2(std::fabs(sum - rrSum) < 1.0e-3 * std::fabs(rrSum),
             qPrintable(QString("rr sums to %1, mne-python says %2").arg(sum).arg(rrSum)));

    // Normals must be unit length; a reader that swapped rr and nn would pass
    // the component checks above on a lucky file but not this.
    for(int i = 0; i < std::min<int>(64, s.nn.rows()); ++i) {
        const double len = s.nn.row(i).cast<double>().norm();
        QVERIFY2(std::fabs(len - 1.0) < 1.0e-4,
                 qPrintable(QString("normal %1 has length %2").arg(i).arg(len)));
    }
}

//=============================================================================================================

void TestMneSourceSpacePython::hemisphereIdentification_data()
{
    QTest::addColumn<int>("hemi");
    QTest::addColumn<int>("expectedHemiId");
    QTest::addColumn<bool>("expectedIsLeft");

    QTest::newRow("left")  << 0 << 101 << true;
    QTest::newRow("right") << 1 << 102 << false;
}

//=============================================================================================================

void TestMneSourceSpacePython::hemisphereIdentification()
{
    QFETCH(int, hemi);
    QFETCH(int, expectedHemiId);
    QFETCH(bool, expectedIsLeft);

    const MNESourceSpace& s = *m_spaces.at(static_cast<size_t>(hemi));

    // The existing test called this and discarded the result with Q_UNUSED.
    QCOMPARE(static_cast<int>(s.find_source_space_hemi()), expectedHemiId);
    QCOMPARE(s.is_left_hemi(), expectedIsLeft);
}

//=============================================================================================================

void TestMneSourceSpacePython::transform_isRigidAndUpdatesFrame()
{
    MNESourceSpace& s = *m_spaces.at(0);

    const MatrixX3f rrBefore = s.rr;
    const int frameBefore = s.coord_frame;
    QCOMPARE(frameBefore, 5);   // FIFFV_COORD_MRI

    // A pure translation of 10 mm along x, expressed MRI to head.
    FiffCoordTrans t;
    t.from = 5;    // FIFFV_COORD_MRI
    t.to   = 4;    // FIFFV_COORD_HEAD
    t.trans = Matrix4f::Identity();
    t.trans(0, 3) = 0.01f;

    // The inverse of a pure translation is the opposite translation, so it can
    // be written down rather than computed. That keeps this test off Eigen's
    // LU module, which the library does not otherwise pull in.
    t.invtrans = Matrix4f::Identity();
    t.invtrans(0, 3) = -0.01f;

    QCOMPARE(s.transform_source_space(t), 0);

    // The frame has to follow the transform, otherwise later stages combine
    // coordinates from two different spaces without noticing.
    QCOMPARE(s.coord_frame, 4);

    // Every vertex moves by exactly the translation.
    for(int i = 0; i < std::min<int>(128, s.rr.rows()); ++i) {
        QVERIFY2(std::fabs(s.rr(i, 0) - (rrBefore(i, 0) + 0.01f)) < 1.0e-6f,
                 qPrintable(QString("vertex %1 x moved to %2, expected %3")
                            .arg(i).arg(s.rr(i, 0)).arg(rrBefore(i, 0) + 0.01f)));
        QVERIFY2(std::fabs(s.rr(i, 1) - rrBefore(i, 1)) < 1.0e-6f, "y should not move");
        QVERIFY2(std::fabs(s.rr(i, 2) - rrBefore(i, 2)) < 1.0e-6f, "z should not move");
    }

    // Distances survive a rigid transform.
    const double dBefore = (rrBefore.row(0) - rrBefore.row(1)).cast<double>().norm();
    const double dAfter = (s.rr.row(0) - s.rr.row(1)).cast<double>().norm();
    QVERIFY2(std::fabs(dBefore - dAfter) < 1.0e-6,
             qPrintable(QString("distance changed from %1 to %2").arg(dBefore).arg(dAfter)));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneSourceSpacePython)
#include "test_mne_source_space_python.moc"
