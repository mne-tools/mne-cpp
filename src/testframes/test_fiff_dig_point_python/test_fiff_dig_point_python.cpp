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
#include <QTemporaryDir>

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
class TestFiffDigPointPython : public QObject
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
    void writeRoundTrip();
    void polhemusIsotrak_matchesPython_data();
    void polhemusIsotrak_matchesPython();
    void polhemusFastscan_matchesPython_data();
    void polhemusFastscan_matchesPython();
    void polhemusReaders_rejectBadInput();
    void montageReaders_matchPython_data();
    void montageReaders_matchPython();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestFiffDigPointPython::rawPath()
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
}

//=============================================================================================================

void TestFiffDigPointPython::initTestCase()
{
    if (!QFile::exists(rawPath())) {
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
    QTest::newRow("cardinal") << static_cast<int>(FIFFV_POINT_CARDINAL) << 3;
    QTest::newRow("hpi") << static_cast<int>(FIFFV_POINT_HPI) << 4;
    QTest::newRow("eeg") << static_cast<int>(FIFFV_POINT_EEG) << 61;
    QTest::newRow("extra") << static_cast<int>(FIFFV_POINT_EXTRA) << 78;
}

//=============================================================================================================

void TestFiffDigPointPython::countByKind_matchesPython()
{
    QFETCH(int, kind);
    QFETCH(int, count);

    int found = 0;
    for (qint32 i = 0; i < m_dig.size(); ++i) {
        if (m_dig[i].kind == kind) {
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
                            << -0.07137660682201385 << 0.0 << 5.122274160385132e-09;
    QTest::newRow("lpa") << 2 << "LPA"
                         << 3.725290298461914e-09 << 0.10260561108589172 << 4.190951585769653e-09;
    QTest::newRow("rpa") << 3 << "RPA"
                         << 0.07526767998933792 << 0.0 << 5.587935447692871e-09;
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
    for (qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        if (p.kind != FIFFV_POINT_CARDINAL || p.ident != ident) {
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
                                .arg(x, 0, 'e', 17)
                                .arg(y, 0, 'e', 17)
                                .arg(z, 0, 'e', 17)));
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
    for (qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        rSum += static_cast<double>(p.r[0]) + static_cast<double>(p.r[1]) + static_cast<double>(p.r[2]);
        identSum += static_cast<int>(p.ident);
    }

    const double expRSum = 13.621201586968;
    QVERIFY2(std::fabs(rSum - expRSum) < 1.0e-6,
             qPrintable(QString("coordinates sum to %1, mne-python says %2")
                            .arg(rSum, 0, 'f', 12)
                            .arg(expRSum, 0, 'f', 12)));

    // The idents distinguish points of the same kind. If they were lost or
    // renumbered the coordinates could still be right while the points became
    // unidentifiable.
    QCOMPARE(identSum, 4927);
}

//=============================================================================================================

void TestFiffDigPointPython::writeRoundTrip()
{
    // The written isotrak file reads back with every point; mne.channels.read_dig_fif reads it too (146 points).
    QTemporaryDir dir;
    const QString path = dir.filePath("dig.fif");
    QString error;
    QVERIFY2(m_dig.write(path, &error), qPrintable(error));
    QFile file(path);
    const FiffDigPointSet back(file);
    QCOMPARE(back.size(), m_dig.size());
    for (qint32 i = 0; i < m_dig.size(); ++i) {
        QCOMPARE(back[i].kind, m_dig[i].kind);
        QCOMPARE(back[i].ident, m_dig[i].ident);
        for (int c = 0; c < 3; ++c)
            QCOMPARE(back[i].r[c], m_dig[i].r[c]);
    }

    QVERIFY(!m_dig.write(QString(), &error));
    QCOMPARE(error, QStringLiteral("Output path is empty."));
    QVERIFY(!m_dig.write(dir.filePath("missing/dig.fif"), &error));
    QVERIFY(error.startsWith(QStringLiteral("Destination directory")));
}

//=============================================================================================================

void TestFiffDigPointPython::coordFrame_isHeadEverywhere()
{
    // Every point in this file is in head coordinates. A point left in device
    // or unknown coordinates would be silently combined with the rest and
    // shift the coregistration.
    for (qint32 i = 0; i < m_dig.size(); ++i) {
        const FiffDigPoint& p = m_dig[i];
        QVERIFY2(p.coord_frame == FIFFV_COORD_HEAD,
                 qPrintable(QString("point %1 is in coordinate frame %2, expected %3 (head)")
                                .arg(i)
                                .arg(p.coord_frame)
                                .arg(FIFFV_COORD_HEAD)));
    }
}

//=============================================================================================================

namespace
{

// Digitizer files in data/: the Polhemus ones are mne-python's own (mne/io/kit/tests/data, BSD-3-Clause).
QString digFile(const QString& name)
{
    return QStringLiteral(DIG_DATA_DIR "/") + name;
}

using Point = QList<double>;

void compareRows(const Point& actual, const Point& expected, const char* what)
{
    for (int c = 0; c < 3; ++c) {
        QVERIFY2(std::fabs(actual[c] - expected[c]) <= 1e-6 * std::fabs(expected[c]) + 1e-12,
                 qPrintable(QString("%1[%2] is %3, mne-python says %4").arg(what).arg(c).arg(actual[c], 0, 'g', 10).arg(expected[c], 0, 'g', 10)));
    }
}

} // namespace

//=============================================================================================================

void TestFiffDigPointPython::polhemusIsotrak_matchesPython_data()
{
    // Reference values produced by mne.channels.read_dig_polhemus_isotrak:
    // cardinals (LPA, nasion, RPA) first, then the points as HPI coils (.elp),
    // head shape (.hsp) or, with channel names, EEG electrodes numbered by the
    // last three characters of the names when all of them are numbers.
    QTest::addColumn<QString>("file");
    QTest::addColumn<QStringList>("chNames");
    QTest::addColumn<QString>("unit");
    QTest::addColumn<int>("count");
    QTest::addColumn<QList<int>>("kindIdents");
    QTest::addColumn<Point>("first");
    QTest::addColumn<Point>("last");
    QTest::addColumn<Point>("sum");

    const Point lpa{-2.1075e-04, 8.0793e-02, -7.5894e-19};
    const Point elpLast{0.10746, -0.034116, 0.031846};
    const Point elpSum{0.4516462, 0.01879889999999999, 0.07785065};
    QTest::newRow("elp as HPI") << "test.elp" << QStringList() << "m" << 8
                                << QList<int>{1, 1, 1, 2, 1, 3, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5} << lpa << elpLast << elpSum;
    QTest::newRow("elp as named EEG") << "test.elp" << QStringList{"Fp1", "Fp2", "Cz", "O1", "O2"} << "m" << 8
                                      << QList<int>{1, 1, 1, 2, 1, 3, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5} << lpa << elpLast << elpSum;
    QTest::newRow("elp as numbered EEG in mm")
        << "test.elp" << QStringList{"EEG 003", "EEG 007", "EEG 011", "EEG 012", "EEG 020"} << "mm" << 8
        << QList<int>{1, 1, 1, 2, 1, 3, 3, 3, 3, 7, 3, 11, 3, 12, 3, 20}
        << Point{-2.1075e-07, 8.0793e-05, -7.5894e-22} << Point{1.0746e-04, -3.4116e-05, 3.1846e-05}
        << Point{4.516462e-04, 1.8798899999999994e-05, 7.785065e-05};
    QTest::newRow("hsp as head shape") << "test.hsp" << QStringList() << "m" << 503
                                       << QList<int>{1, 1, 1, 2, 1, 3, 4, 1, 4, 2, 4, 3, 4, 4} << lpa << Point{0.014196, -0.078057, 0.074778}
                                       << Point{-6.573226000000002, -37.38123799999998, 37.735278999999984};
}

//=============================================================================================================

void TestFiffDigPointPython::polhemusIsotrak_matchesPython()
{
    QFETCH(QString, file);
    QFETCH(QStringList, chNames);
    QFETCH(QString, unit);
    QFETCH(int, count);
    QFETCH(QList<int>, kindIdents);
    QFETCH(Point, first);
    QFETCH(Point, last);
    QFETCH(Point, sum);

    FiffDigPointSet dig;
    QVERIFY(FiffDigPointSet::readPolhemusIsotrak(digFile(file), dig, chNames, unit));
    QCOMPARE(dig.size(), count);
    for (int i = 0; i < kindIdents.size() / 2; ++i) {
        QCOMPARE(dig[i].kind, kindIdents[2 * i]);
        QCOMPARE(dig[i].ident, kindIdents[2 * i + 1]);
    }
    Point total{0.0, 0.0, 0.0};
    for (int i = 0; i < dig.size(); ++i) {
        QCOMPARE(dig[i].coord_frame, FIFFV_COORD_UNKNOWN);
        for (int c = 0; c < 3; ++c)
            total[c] += dig[i].r[c];
    }
    if (kindIdents.size() / 2 < dig.size()) // head shape points are numbered 1..n after the cardinals
        QCOMPARE(dig[dig.size() - 1].ident, dig.size() - 3);
    compareRows({dig[0].r[0], dig[0].r[1], dig[0].r[2]}, first, "first point");
    const FiffDigPoint& end = dig[dig.size() - 1];
    compareRows({end.r[0], end.r[1], end.r[2]}, last, "last point");
    compareRows(total, sum, "sum");
}

//=============================================================================================================

void TestFiffDigPointPython::polhemusFastscan_matchesPython_data()
{
    // Reference values produced by mne.channels.read_polhemus_fastscan (millimetres -> metres).
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("count");
    QTest::addColumn<Point>("first");
    QTest::addColumn<Point>("last");
    QTest::addColumn<Point>("sum");

    QTest::newRow("electrodes") << "test_elp.txt" << 8 << Point{0.001393, 0.0131613, -0.0046967}
                                << Point{-0.0277519, 0.0452628, -0.0222407}
                                << Point{-0.32169220000000004, 0.05751710000000002, 0.24655119999999997};
    QTest::newRow("head shape") << "test_hsp.txt" << 500 << Point{-0.10693, 0.0998, 0.06881} << Point{-0.12335, 0.08139, 0.02279}
                                << Point{-70.54912999999995, 38.24442999999999, 21.787739999999992};
}

//=============================================================================================================

void TestFiffDigPointPython::polhemusFastscan_matchesPython()
{
    QFETCH(QString, file);
    QFETCH(int, count);
    QFETCH(Point, first);
    QFETCH(Point, last);
    QFETCH(Point, sum);

    Eigen::MatrixX3d points;
    QVERIFY(FiffDigPointSet::readPolhemusFastscan(digFile(file), points));
    QCOMPARE(points.rows(), count);
    compareRows({points(0, 0), points(0, 1), points(0, 2)}, first, "first point");
    compareRows({points(count - 1, 0), points(count - 1, 1), points(count - 1, 2)}, last, "last point");
    const Eigen::RowVector3d total = points.colwise().sum();
    compareRows({total(0), total(1), total(2)}, sum, "sum");

    // In metres the same file is a thousand times larger
    Eigen::MatrixX3d metres;
    QVERIFY(FiffDigPointSet::readPolhemusFastscan(digFile(file), metres, QStringLiteral("m")));
    QVERIFY(metres.isApprox(1000.0 * points));
}

//=============================================================================================================

void TestFiffDigPointPython::polhemusReaders_rejectBadInput()
{
    // Each of these raises in mne-python.
    FiffDigPointSet dig;
    QVERIFY(!FiffDigPointSet::readPolhemusIsotrak(digFile("test.elp"), dig, {"Fp1"}));
    QVERIFY(!FiffDigPointSet::readPolhemusIsotrak(digFile("test.elp"), dig, {}, "inch"));
    QVERIFY(!FiffDigPointSet::readPolhemusIsotrak(digFile("test_elp.txt"), dig));
    QVERIFY(!FiffDigPointSet::readPolhemusIsotrak(digFile("missing.hsp"), dig));

    // A FastSCAN file whose header does not name FastSCAN is rejected unless asked not to
    QTemporaryDir dir;
    QFile in(digFile("test_elp.txt"));
    QVERIFY(in.open(QIODevice::ReadOnly));
    QFile out(dir.filePath("other.txt"));
    QVERIFY(out.open(QIODevice::WriteOnly));
    out.write(in.readAll().replace("FastSCAN", "XxxxXXXX"));
    out.close();
    Eigen::MatrixX3d points;
    QVERIFY(!FiffDigPointSet::readPolhemusFastscan(out.fileName(), points));
    QVERIFY(FiffDigPointSet::readPolhemusFastscan(out.fileName(), points, QStringLiteral("mm"), false));
    QCOMPARE(points.rows(), 8);
    QVERIFY(!FiffDigPointSet::readPolhemusFastscan(digFile("test.elp"), points));
}

//=============================================================================================================

void TestFiffDigPointPython::montageReaders_matchPython_data()
{
    // Reference values produced by mne.channels.read_dig_captrak, read_dig_egi (whose
    // centimetres are scaled to metres here, as mne.io.read_raw_egi does),
    // read_dig_localite(nasion="Nasion", lpa="LPA", rpa="RPA") and read_dig_dat on the
    // fixtures: the cardinals, then one EEG point per name with the ident mne gives it.
    // captrak_coords.bvct and coordinates.xml are from the MNE testing dataset; the
    // Localite and Neuroscan files are the ones mne-python's own tests write.
    QTest::addColumn<QString>("file");
    QTest::addColumn<int>("count");
    QTest::addColumn<QStringList>("firstLastNames");
    QTest::addColumn<QList<int>>("lastIdents");
    QTest::addColumn<Point>("first");
    QTest::addColumn<Point>("last");
    QTest::addColumn<Point>("sum");

    QTest::newRow("CapTrak") << "captrak_coords.bvct" << 66 << QStringList{"T7", "FT8"} << QList<int>{65, 66}
                             << Point{-9.189697162389295e-02, 7.105427357601002e-18, 7.815970093361102e-17}
                             << Point{0.0911920055367091, 0.0354346303180068, 0.04959908404797406}
                             << Point{-0.12517363475026916, 0.02491909370084076, 5.843050150410273};
    QTest::newRow("EGI") << "coordinates.xml" << 257 << QStringList{"EEG 001", "EEG 257"} << QList<int>{256, 257}
                         << Point{-0.08592, 0.00498, -0.04128} << Point{0.0, 0.0, 0.09683}
                         << Point{-7.4940054162198066e-16, -1.4278399999999958, 0.77265999999999957};
    QTest::newRow("Localite") << "localite.csv" << 15 << QStringList{"ch01", "ch15"} << QList<int>{14, 15}
                              << Point{0.07196698724, -0.02988835576, 0.1136703679}
                              << Point{-0.05582855386, -0.03477319103, 0.0258083942}
                              << Point{-0.071353608824, -0.950737338145, 0.492984028449};
    // The repeated O2 keeps its first place and its last position; the centroid is dropped
    QTest::newRow("Neuroscan") << "neuroscan.dat" << 1 << QStringList{"O2", "O2"} << QList<int>{3, 1}
                               << Point{-1.0, 0.0, 0.0} << Point{0.0, 0.01, 0.02} << Point{0.0, 1.01, 0.02};
}

//=============================================================================================================

void TestFiffDigPointPython::montageReaders_matchPython()
{
    QFETCH(QString, file);
    QFETCH(int, count);
    QFETCH(QStringList, firstLastNames);
    QFETCH(QList<int>, lastIdents);
    QFETCH(Point, first);
    QFETCH(Point, last);
    QFETCH(Point, sum);

    FiffDigPointSet dig;
    QStringList names;
    const QString path = digFile(file);
    bool ok = false;
    if (file.endsWith(".bvct"))
        ok = FiffDigPointSet::readCaptrak(path, dig, &names);
    else if (file.endsWith(".xml"))
        ok = FiffDigPointSet::readEgi(path, dig, &names);
    else if (file.endsWith(".csv"))
        ok = FiffDigPointSet::readLocalite(path, dig, &names, "Nasion", "LPA", "RPA");
    else
        ok = FiffDigPointSet::readNeuroscanDat(path, dig, &names);
    QVERIFY(ok);

    QCOMPARE(names.size(), count);
    QCOMPARE(names.first(), firstLastNames.first());
    QCOMPARE(names.last(), firstLastNames.last());
    QCOMPARE(dig.size(), count + 3);
    const QList<int> cardinals{FIFFV_POINT_LPA, FIFFV_POINT_NASION, FIFFV_POINT_RPA};
    Point total{0.0, 0.0, 0.0};
    for (int i = 0; i < dig.size(); ++i) {
        QCOMPARE(dig[i].kind, i < 3 ? FIFFV_POINT_CARDINAL : FIFFV_POINT_EEG);
        if (i < 3)
            QCOMPARE(dig[i].ident, cardinals[i]);
        QCOMPARE(dig[i].coord_frame, FIFFV_COORD_UNKNOWN);
        for (int c = 0; c < 3; ++c)
            total[c] += dig[i].r[c];
    }
    QCOMPARE(dig[dig.size() - 2].ident, lastIdents[0]);
    QCOMPARE(dig[dig.size() - 1].ident, lastIdents[1]);
    compareRows({dig[0].r[0], dig[0].r[1], dig[0].r[2]}, first, "first point");
    const FiffDigPoint& end = dig[dig.size() - 1];
    compareRows({end.r[0], end.r[1], end.r[2]}, last, "last point");
    compareRows(total, sum, "sum");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestFiffDigPointPython)
#include "test_fiff_dig_point_python.moc"
