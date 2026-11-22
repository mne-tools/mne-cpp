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
 * data/old_style-ave.fif (make_old_style_evoked_fixture.py) holds an older
 * layout: channel information inside the evoked block and one FIFF_EPOCH tag
 * per channel.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_evoked_set.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
/**
 * DECLARE CLASS TestFiffEvokedPython
 *
 * @brief The TestFiffEvokedPython class checks the evoked reader against mne-python.
 */
class TestFiffEvokedPython : public QObject
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
    void oldStyleEvoked_matchesPython();
    void readSelectors_matchPython();
    void readsAndSavesByPath();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffEvokedPython::avePath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis-ave.fif";
}

//=============================================================================================================

void TestFiffEvokedPython::initTestCase()
{
    if (!QFile::exists(avePath())) {
        QSKIP("Averaged test data not found");
    }

    m_set = FiffEvokedSet(avePath().toStdString());

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
    QTest::newRow("left auditory") << 0 << "Left Auditory" << 55 << 5.710164498347e+01;
    QTest::newRow("right auditory") << 1 << "Right Auditory" << 61 << 7.910733502294e+01;
    QTest::newRow("left visual") << 2 << "Left visual" << 67 << 1.142761471304e+02;
    QTest::newRow("right visual") << 3 << "Right visual" << 58 << 9.442361010079e+01;
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
                            .arg(comment)
                            .arg(sum, 0, 'e', 12)
                            .arg(dataSum, 0, 'e', 12)));
}

//=============================================================================================================

void TestFiffEvokedPython::readSelectors_matchPython()
{
    const auto path = avePath().toStdU16String();

    // mne.read_evokeds(..., condition="Right visual", proj=False / True, baseline=None): the file's
    // projectors are already active, so both give 94.42361010079253.
    for (const bool proj : {false, true}) {
        const FiffEvoked e(path, "Right visual", {}, proj);
        QCOMPARE(static_cast<int>(e.nave), 58);
        QVERIFY(std::fabs(e.data.sum() - 94.42361010079253) < 1e-7 * 94.42361010079253);
        QCOMPARE(e.proj.rows() > 0, proj);
    }

    // condition=1, baseline=(-0.2, 0.0): sum 4.109845103910902, data[0, 0] 4.086686400793876e-13.
    const FiffEvoked e(path, 1, {{-0.2f, 0.0f}});
    QCOMPARE(e.comment, QString("Right Auditory"));
    QCOMPARE(e.baseline, QPair(-0.2f, 0.0f));
    QVERIFY2(std::fabs(e.data.sum() - 4.109845103910902) < 1e-6 * 4.109845103910902, qPrintable(QString::number(e.data.sum(), 'g', 17)));
    QVERIFY(std::fabs(e.data(0, 0) - 4.086686400793876e-13) < 1e-6 * 4.086686400793876e-13);

    // Selectors that mne-python rejects too.
    for (const auto& bad : {FiffEvoked::read(path, "Left Auditory", {}, true, FIFFV_ASPECT_STD_ERR),
                            FiffEvoked::read(path, "Left Auditory", {}, true, 99),
                            FiffEvoked::read(path, 4),
                            FiffEvoked::read(path, -1)}) {
        QVERIFY(!bad);
        QCOMPARE(bad.error().code, mne::ErrorCode::NotFound);
    }
    QVERIFY_THROWS_EXCEPTION(mne::Error, FiffEvoked(path, "No such condition"));
}

//=============================================================================================================

void TestFiffEvokedPython::times_matchPython()
{
    // Every condition shares one time axis. If it were off, the whole response
    // would be shifted in time while still looking like a clean evoked field.
    for (const FiffEvoked& e : m_set.evoked) {
        QCOMPARE(static_cast<int>(e.times.size()), 421);

        const double tmin = static_cast<double>(e.times(0));
        const double tmax = static_cast<double>(e.times(e.times.size() - 1));

        // times is stored as float, so these are compared at float precision
        // rather than as exact doubles.
        QVERIFY2(std::fabs(tmin - (-0.19979521315838786)) < 1.0e-7,
                 qPrintable(QString("%1 starts at %2, mne-python says -0.19979521315838786")
                                .arg(e.comment)
                                .arg(tmin, 0, 'g', 17)));
        QVERIFY2(std::fabs(tmax - 0.49948803289596966) < 1.0e-7,
                 qPrintable(QString("%1 ends at %2, mne-python says 0.49948803289596966")
                                .arg(e.comment)
                                .arg(tmax, 0, 'g', 17)));

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
    for (const FiffEvoked& e : m_set.evoked) {
        actual << QString::fromStdString(e.comment);
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
    for (const FiffEvoked& e : m_set.evoked) {
        sums << e.data.sum();
    }

    for (int i = 0; i < sums.size(); ++i) {
        for (int j = i + 1; j < sums.size(); ++j) {
            QVERIFY2(std::fabs(sums.at(i) - sums.at(j)) > 1.0,
                     qPrintable(QString("conditions %1 and %2 have near identical data sums, "
                                        "which makes the pairing check meaningless")
                                    .arg(i)
                                    .arg(j)));
        }
    }
}

//=============================================================================================================

void TestFiffEvokedPython::oldStyleEvoked_matchesPython()
{
    const FiffEvoked e(MNE_FIFF_EVOKED_DATA_DIR "/old_style-ave.fif", 0, {}, false);

    // mne.read_evokeds(..., baseline=None, proj=False): the local 200 Hz channel
    // definitions and calibrations replace the measurement info's.
    QCOMPARE(e.info.nchan, 6);
    QCOMPARE(e.info.sfreq, 200.0);
    QCOMPARE(e.info.chs[5].ch_name, QString("EEG 3"));
    QCOMPARE(e.nave, 1);
    QCOMPARE(e.aspect_kind, FIFFV_ASPECT_STD_ERR);
    QCOMPARE(e.comment, QString("aspect comment"));
    QCOMPARE(static_cast<int>(e.data.rows()), 6);
    QCOMPARE(static_cast<int>(e.data.cols()), 20);
    QVERIFY(std::fabs(e.times[0] + 0.02f) < 1e-7f);
    QVERIFY(std::fabs(e.data(0, 0) - 0.2766302824020386) < 1e-12);
    QVERIFY(std::fabs(e.data(5, 19) + 0.12329661194235086) < 1e-12);
    QVERIFY2(std::fabs(e.data.cwiseAbs().sum() - 304.2275329530239) < 1e-9,
             qPrintable(QString("abs sum %1").arg(e.data.cwiseAbs().sum(), 0, 'g', 17)));
}

void TestFiffEvokedPython::readsAndSavesByPath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const std::string out = dir.filePath("set-ave.fif").toStdString();

    QVERIFY(m_set.save(out));
    const FiffEvokedSet back(out);
    QCOMPARE(back.evoked.size(), m_set.evoked.size());
    for (qsizetype i = 0; i < std::ssize(back.evoked); ++i) {
        QCOMPARE(back.evoked[i].comment, m_set.evoked[i].comment);
        QVERIFY(back.evoked[i].data.isApprox(m_set.evoked[i].data, 1e-6));
    }

    QCOMPARE(FiffEvoked(out, 2).comment, m_set.evoked[2].comment);

    // The baseline reaches every condition of the set.
    const FiffEvokedSet corrected(out, {}, {{-0.2f, 0.0f}});
    QCOMPARE(corrected.evoked[3].baseline, QPair(-0.2f, 0.0f));
    QVERIFY(corrected.evoked[3].data.isApprox(FiffEvoked(out, 3, {{-0.2f, 0.0f}}).data));

    // mne.read_evokeds(fname, condition=["Right visual", 0]) keeps the requested order.
    const FiffEvokedSet picked(out, {"Right visual", 0});
    QCOMPARE(picked.evoked.size(), 2);
    QCOMPARE(picked.evoked[0].comment, QString("Right visual"));
    QCOMPARE(picked.evoked[1].comment, m_set.evoked[0].comment);
    QVERIFY(picked.evoked[1].data.isApprox(back.evoked[0].data));
    QCOMPARE(FiffEvokedSet::read(out, {0, 4}).error().code, mne::ErrorCode::NotFound);

    const std::string missing = dir.filePath("missing-ave.fif").toStdString();
    QCOMPARE(FiffEvokedSet::read(missing).error().code, mne::ErrorCode::Io);
    QCOMPARE(FiffEvoked::read(missing).error().code, mne::ErrorCode::Io);
    QVERIFY_THROWS_EXCEPTION(mne::Error, FiffEvokedSet{missing});
    const auto covariance = FiffEvokedSet::read(QString(avePath()).replace("-ave.fif", "-cov.fif").toStdU16String());
    QCOMPARE(covariance.error().code, mne::ErrorCode::NotFound);
    QCOMPARE(m_set.save(dir.filePath("no/dir/set-ave.fif").toStdString()).error().code, mne::ErrorCode::Io);
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffEvokedPython)
#include "test_fiff_evoked_python.moc"
