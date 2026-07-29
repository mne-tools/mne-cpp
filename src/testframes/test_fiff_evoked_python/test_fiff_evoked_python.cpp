//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_fiff_evoked_python.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     July, 2026
 * @brief    Cross validates the evoked set reader against mne-python.
 *
 * An averaged file holds several conditions side by side, and the thing most
 * likely to go wrong is not the numbers but the bookkeeping: returning the
 * conditions in a different order, or attaching one condition's nave to
 * another's data. Neither shows up as a crash. Both produce a result that
 * looks entirely ordinary and is attributed to the wrong stimulus.
 *
 * Reference values below come from mne.read_evokeds on the same file. The four
 * conditions have distinct nave values and distinct data sums, so pairing them
 * up wrongly is detectable:
 *
 *   Left Auditory   nave 55   sum 5.710164498347e+01
 *   Right Auditory  nave 61   sum 7.910733502294e+01
 *   Left visual     nave 67   sum 1.142761471304e+02
 *   Right visual    nave 58   sum 9.442361010079e+01
 *
 * All four share 376 channels, 421 samples, aspect kind 100 (average) and run
 * from -0.19979521315838786 s to 0.49948803289596966 s.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_constants.h>

#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffEvokedPython
 *
 * @brief The TestFiffEvokedPython class checks the evoked reader against mne-python.
 */
class TestFiffEvokedPython: public QObject
{
    Q_OBJECT

public:
    TestFiffEvokedPython() = default;

private:
    static QString avePath();

    FiffEvokedSet m_set;

private slots:
    void initTestCase();
    void conditions_matchPython_data();
    void conditions_matchPython();
    void times_matchPython();
    void conditionOrder_matchesPython();
    void dataSumsAreDistinct();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffEvokedPython::avePath()
{
    return QCoreApplication::applicationDirPath()
           + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
}

//=============================================================================================================

void TestFiffEvokedPython::initTestCase()
{
    if(!QFile::exists(avePath())) {
        QSKIP("Averaged test data not found");
    }

    QFile aveFile(avePath());
    m_set = FiffEvokedSet(aveFile);

    QCOMPARE(m_set.evoked.size(), 4);
}

//=============================================================================================================

void TestFiffEvokedPython::conditions_matchPython_data()
{
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("comment");
    QTest::addColumn<int>("nave");
    QTest::addColumn<double>("dataSum");

    // nave and the data sum are checked together on the same row, which is what
    // makes a mismatched pairing visible. Checking them in separate tests would
    // let a set with swapped naves pass both.
    QTest::newRow("left auditory")  << 0 << "Left Auditory"  << 55 <<  5.710164498347e+01;
    QTest::newRow("right auditory") << 1 << "Right Auditory" << 61 <<  7.910733502294e+01;
    QTest::newRow("left visual")    << 2 << "Left visual"    << 67 <<  1.142761471304e+02;
    QTest::newRow("right visual")   << 3 << "Right visual"   << 58 <<  9.442361010079e+01;
}

//=============================================================================================================

void TestFiffEvokedPython::conditions_matchPython()
{
    QFETCH(int, index);
    QFETCH(QString, comment);
    QFETCH(int, nave);
    QFETCH(double, dataSum);

    const FiffEvoked& e = m_set.evoked.at(index);

    QCOMPARE(e.comment, comment);
    QCOMPARE(static_cast<int>(e.nave), nave);
    QCOMPARE(static_cast<int>(e.aspect_kind), static_cast<int>(FIFFV_ASPECT_AVERAGE));

    QCOMPARE(static_cast<int>(e.data.rows()), 376);
    QCOMPARE(static_cast<int>(e.data.cols()), 421);

    // Summing the whole 376 by 421 block pins every sample rather than a few
    // spot values, so a channel read at the wrong offset shows up here.
    //
    // The tolerance is set by the data, not by taste. The epoch tags are float32
    // on disk, whose relative precision is about 1.2e-7, so asking the two
    // libraries to agree more closely than that is asking for better than the
    // file can represent. They actually agree to about 1.9e-9 relative, roughly
    // sixty times tighter than float32 precision, the residual being arithmetic
    // ordering over 158296 elements rather than any difference in meaning. 1e-7
    // keeps a wide margin over that while staying far below the size of any
    // real error: a single misread channel moves this sum by orders more.
    const double sum = e.data.sum();
    QVERIFY2(std::fabs(sum - dataSum) / std::fabs(dataSum) < 1.0e-7,
             qPrintable(QString("%1 data sums to %2, mne-python says %3")
                        .arg(comment).arg(sum, 0, 'e', 12).arg(dataSum, 0, 'e', 12)));
}

//=============================================================================================================

void TestFiffEvokedPython::times_matchPython()
{
    // Every condition shares one time axis. If it were off, the whole response
    // would be shifted in time while still looking like a clean evoked field.
    for(const FiffEvoked& e : m_set.evoked) {
        QCOMPARE(static_cast<int>(e.times.size()), 421);

        const double tmin = static_cast<double>(e.times(0));
        const double tmax = static_cast<double>(e.times(e.times.size() - 1));

        // times is stored as float, so these are compared at float precision
        // rather than as exact doubles.
        QVERIFY2(std::fabs(tmin - (-0.19979521315838786)) < 1.0e-7,
                 qPrintable(QString("%1 starts at %2, mne-python says -0.19979521315838786")
                            .arg(e.comment).arg(tmin, 0, 'g', 17)));
        QVERIFY2(std::fabs(tmax - 0.49948803289596966) < 1.0e-7,
                 qPrintable(QString("%1 ends at %2, mne-python says 0.49948803289596966")
                            .arg(e.comment).arg(tmax, 0, 'g', 17)));

        // The time axis has to line up with the data it labels.
        QCOMPARE(static_cast<int>(e.times.size()), static_cast<int>(e.data.cols()));
    }
}

//=============================================================================================================

void TestFiffEvokedPython::conditionOrder_matchesPython()
{
    // Order matters because callers index into this list. mne-python returns
    // the conditions in file order and so must this.
    const QStringList expected{"Left Auditory", "Right Auditory", "Left visual", "Right visual"};

    QStringList actual;
    for(const FiffEvoked& e : m_set.evoked) {
        actual << e.comment;
    }

    QCOMPARE(actual, expected);
}

//=============================================================================================================

void TestFiffEvokedPython::dataSumsAreDistinct()
{
    // This guards the test above rather than the code. The pairing check is
    // only meaningful while the four sums differ from one another, so if new
    // data ever made two conditions identical the check would quietly weaken
    // and this states that assumption out loud.
    QList<double> sums;
    for(const FiffEvoked& e : m_set.evoked) {
        sums << e.data.sum();
    }

    for(int i = 0; i < sums.size(); ++i) {
        for(int j = i + 1; j < sums.size(); ++j) {
            QVERIFY2(std::fabs(sums.at(i) - sums.at(j)) > 1.0,
                     qPrintable(QString("conditions %1 and %2 have near identical data sums, "
                                        "which makes the pairing check meaningless").arg(i).arg(j)));
        }
    }
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffEvokedPython)
#include "test_fiff_evoked_python.moc"
