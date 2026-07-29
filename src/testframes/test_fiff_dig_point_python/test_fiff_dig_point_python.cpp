//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_dig_point_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Cross validates the digitizer point reader against mne-python.
 *
 * Digitizer points are what tie the head to the sensor array. If they are read
 * wrong the coregistration is wrong, and every source estimate downstream is
 * placed in the wrong spot while looking entirely plausible. The cardinal
 * points matter most: nasion, LPA and RPA define the head coordinate frame
 * itself, so swapping two of them mirrors the head without any obvious symptom.
 *
 * Reference values below come from mne.io.read_raw_fif on the same file:
 *
 *   n points     146
 *   by kind      cardinal 3, HPI 4, EEG 61, extra 78
 *   coord frame  4 (FIFFV_COORD_HEAD) for every point
 *   sum of r     13.621201586968
 *   sum of ident 4927
 *   nasion  (ident 1) (-0.07137660682201385,   0.0,                  5.122274160385132e-09)
 *   LPA     (ident 2) ( 3.725290298461914e-09, 0.10260561108589172,  4.190951585769653e-09)
 *   RPA     (ident 3) ( 0.07526767998933792,   0.0,                  5.587935447692871e-09)
 *
 * The cardinal coordinates are quoted at full precision on purpose. Two of them
 * are around 1e-9, so reference values rounded for readability are not merely
 * imprecise, they are indistinguishable from zero under any tolerance loose
 * enough to accept them.
 *
 * Note that mne-python numbers EEG points as kind 3 and extra points as kind 4,
 * matching FIFFV_POINT_EEG and FIFFV_POINT_EXTRA.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_constants.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffDigPointPython
 *
 * @brief The TestFiffDigPointPython class checks the digitizer reader against mne-python.
 */
class TestFiffDigPointPython: public QObject
{
    Q_OBJECT

public:
    TestFiffDigPointPython() = default;

private:
    static QString rawPath();

    FiffDigPointSet m_dig;

private slots:
    void initTestCase();
    void countByKind_matchesPython_data();
    void countByKind_matchesPython();
    void cardinals_matchPython_data();
    void cardinals_matchPython();
    void allPoints_matchPython();
    void coordFrame_isHeadEverywhere();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffDigPointPython::rawPath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

void TestFiffDigPointPython::initTestCase()
{
    if(!QFile::exists(rawPath())) {
        QSKIP("Raw test data not found");
    }

    QFile rawFile(rawPath());
    m_dig = FiffDigPointSet(rawFile);

    QCOMPARE(m_dig.size(), 146);
}

//=============================================================================================================

void TestFiffDigPointPython::countByKind_matchesPython_data()
{
    QTest::addColumn<int>("kind");
    QTest::addColumn<int>("count");

    // A reader that silently dropped one class of point still returns a
    // plausible looking set, so the counts are checked per kind rather than
    // only in total.
    QTest::newRow("cardinal") << static_cast<int>(FIFFV_POINT_CARDINAL) <<  3;
    QTest::newRow("hpi")      << static_cast<int>(FIFFV_POINT_HPI)      <<  4;
    QTest::newRow("eeg")      << static_cast<int>(FIFFV_POINT_EEG)      << 61;
    QTest::newRow("extra")    << static_cast<int>(FIFFV_POINT_EXTRA)    << 78;
}

//=============================================================================================================

void TestFiffDigPointPython::countByKind_matchesPython()
{
    QFETCH(int, kind);
    QFETCH(int, count);

    int found = 0;
    for(qint32 i = 0; i < m_dig.size(); ++i) {
        if(m_dig[i].kind == kind) {
            ++found;
        }
    }

    QCOMPARE(found, count);
}

//=============================================================================================================

void TestFiffDigPointPython::cardinals_matchPython_data()
{
    QTest::addColumn<int>("ident");
    QTest::addColumn<QString>("name");
    QTest::addColumn<double>("x");
    QTest::addColumn<double>("y");
    QTest::addColumn<double>("z");

    // These three define the head coordinate frame. Checking them individually
    // rather than through a sum is deliberate: swapping two cardinals leaves
    // any aggregate unchanged while mirroring the head.
    QTest::newRow("nasion") << 1 << "nasion"
        << -0.07137660682201385 << 0.0                  << 5.122274160385132e-09;
    QTest::newRow("lpa")    << 2 << "LPA"
        <<  3.725290298461914e-09 << 0.10260561108589172 << 4.190951585769653e-09;
    QTest::newRow("rpa")    << 3 << "RPA"
        <<  0.07526767998933792 << 0.0                  << 5.587935447692871e-09;
}

//=============================================================================================================

void TestFiffDigPointPython::cardinals_matchPython()
{
    QFETCH(int, ident);
    QFETCH(QString, name);
    QFETCH(double, x);
    QFETCH(double, y);
    QFETCH(double, z);

    bool found = false;
    for(qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        if(p.kind != FIFFV_POINT_CARDINAL || p.ident != ident) {
            continue;
        }

        found = true;

        // The file stores these as 32 bit floats and the expected values above
        // are the exact doubles those floats widen to, so the comparison is for
        // equality. Anything looser would tolerate a genuinely different point:
        // the cardinals are only about 10 cm apart, and two of the coordinates
        // here are around 1e-9, so a tolerance of 1e-9 would call them equal to
        // zero. That is what an earlier version of this test did, which is why
        // the values are written out in full rather than rounded.
        const double dx = std::fabs(static_cast<double>(p.r[0]) - x);
        const double dy = std::fabs(static_cast<double>(p.r[1]) - y);
        const double dz = std::fabs(static_cast<double>(p.r[2]) - z);

        QVERIFY2(dx == 0.0 && dy == 0.0 && dz == 0.0,
                 qPrintable(QString("%1 is at (%2, %3, %4), mne-python says (%5, %6, %7)")
                            .arg(name)
                            .arg(static_cast<double>(p.r[0]), 0, 'e', 17)
                            .arg(static_cast<double>(p.r[1]), 0, 'e', 17)
                            .arg(static_cast<double>(p.r[2]), 0, 'e', 17)
                            .arg(x, 0, 'e', 17).arg(y, 0, 'e', 17).arg(z, 0, 'e', 17)));
        break;
    }

    QVERIFY2(found, qPrintable(QString("cardinal point %1 is missing").arg(name)));
}

//=============================================================================================================

void TestFiffDigPointPython::allPoints_matchPython()
{
    // Summing every coordinate pins all 146 points rather than the handful
    // checked individually above, so a point read at the wrong offset shows up.
    double rSum = 0.0;
    int identSum = 0;
    for(qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        rSum += static_cast<double>(p.r[0])
              + static_cast<double>(p.r[1])
              + static_cast<double>(p.r[2]);
        identSum += static_cast<int>(p.ident);
    }

    const double expRSum = 13.621201586968;
    QVERIFY2(std::fabs(rSum - expRSum) < 1.0e-6,
             qPrintable(QString("coordinates sum to %1, mne-python says %2")
                        .arg(rSum, 0, 'f', 12).arg(expRSum, 0, 'f', 12)));

    // The idents distinguish points of the same kind. If they were lost or
    // renumbered the coordinates could still be right while the points became
    // unidentifiable.
    QCOMPARE(identSum, 4927);
}

//=============================================================================================================

void TestFiffDigPointPython::coordFrame_isHeadEverywhere()
{
    // Every point in this file is in head coordinates. A point left in device
    // or unknown coordinates would be silently combined with the rest and
    // shift the coregistration.
    for(qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        QVERIFY2(p.coord_frame == FIFFV_COORD_HEAD,
                 qPrintable(QString("point %1 is in coordinate frame %2, expected %3 (head)")
                            .arg(i).arg(p.coord_frame).arg(FIFFV_COORD_HEAD)));
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffDigPointPython)
#include "test_fiff_dig_point_python.moc"
