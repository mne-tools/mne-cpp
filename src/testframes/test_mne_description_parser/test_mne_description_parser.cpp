//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_description_parser.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Checks the averaging / covariance description parser against MNE-C interpret.c.
 *
 * The reference is MNE-C mne_browse_raw/interpret.c (interpret_ave_desc,
 * interpret_cov_desc), which the parser ports: every accepted file must yield
 * the values MNE-C would store, and every input MNE-C rejects must be
 * rejected here, not silently accepted with a default.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_description_parser.h>
#include <mne/mne_process_description.h>
#include <fiff/fiff_evoked_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtTest>
#include <QTemporaryDir>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;

//=============================================================================================================
/**
 * DECLARE CLASS TestMneDescriptionParser
 *
 * @brief Checks MNEDescriptionParser against the MNE-C description-file rules.
 */
class TestMneDescriptionParser : public QObject
{
    Q_OBJECT

private:
    QString write(const QString& content);

    QTemporaryDir m_dir;
    int m_count = 0;

private slots:
    void initTestCase();

    void average_full();
    void average_masks();
    void average_rejected_data();
    void average_rejected();

    void covariance_full();
    void covariance_rejected_data();
    void covariance_rejected();
};

//=============================================================================================================
// DEFINE METHODS
//=============================================================================================================

QString TestMneDescriptionParser::write(const QString& content)
{
    const QString path = m_dir.filePath(QString("desc%1.txt").arg(++m_count));
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << content;
    }
    return path;
}

//=============================================================================================================

void TestMneDescriptionParser::initTestCase()
{
    QVERIFY(m_dir.isValid());
}

//=============================================================================================================

void TestMneDescriptionParser::average_full()
{
    // Layout of MNE-sample-data/MEG/sample/audvis.ave plus every optional keyword.
    const QString path = write(R"(# comment
average {
    name        "Audio visual"
    outfile     out-ave.fif
    logfile     out-ave.log   # trailing comment
    eventfile   out.eve
    fixskew
    gradReject  2000e-13
    magReject   4e-12
    eegReject   0e-6
    eogReject   150e-6
    ecgReject   1e-3
    gradFlat    1e-13
    magFlat     1e-15
    eegFlat     1e-7
    eogFlat     2e-7
    ecgFlat     3e-7
    stimIgnore  -0.05
    category {
        name    "Left Auditory"
        event   1
        event   5
        tmin    -0.2
        tmax    0.5
        bmin    -0.2
        bmax    0
        delay   0.012
        nextevent 3
        prevevent 4
        ignore  64
        stderr
        abs
        color   1 0.5 0
    }
    CONDITION {
        name    second
        event   2
        tmin    -0.1
        tmax    0.3
        basemin -0.1
    }
}
)");

    AverageDescription d;
    QVERIFY(MNEDescriptionParser::parseAverageFile(path, d));

    QCOMPARE(d.comment, QString("Audio visual"));
    QCOMPARE(d.filename, QString("out-ave.fif"));
    QCOMPARE(d.logFile, QString("out-ave.log"));
    QCOMPARE(d.eventFile, QString("out.eve"));
    QVERIFY(d.fixSkew);

    QCOMPARE(d.rej.megGradReject, 2000e-13f);
    QCOMPARE(d.rej.megMagReject, 4e-12f);
    QCOMPARE(d.rej.eegReject, 0.0f);
    QCOMPARE(d.rej.eogReject, 150e-6f);
    QCOMPARE(d.rej.ecgReject, 1e-3f);
    QCOMPARE(d.rej.megGradFlat, 1e-13f);
    QCOMPARE(d.rej.megMagFlat, 1e-15f);
    QCOMPARE(d.rej.eegFlat, 1e-7f);
    QCOMPARE(d.rej.eogFlat, 2e-7f);
    QCOMPARE(d.rej.ecgFlat, 3e-7f);
    QCOMPARE(d.rej.stimIgnore, 0.05f);

    QCOMPARE(d.categories.size(), 2);
    const AverageCategory& a = d.categories[0];
    QCOMPARE(a.comment, QString("Left Auditory"));
    QCOMPARE(a.events, (QVector<unsigned int>{1, 5}));
    QCOMPARE(a.tmin, -0.2f);
    QCOMPARE(a.tmax, 0.5f);
    QVERIFY(a.doBaseline);
    QCOMPARE(a.bmin, -0.2f);
    QCOMPARE(a.bmax, 0.0f);
    QCOMPARE(a.delay, 0.012f);
    QCOMPARE(a.nextEvent, 3u);
    QCOMPARE(a.prevEvent, 4u);
    QCOMPARE(a.ignore, 64u);
    // check_cat: unset prev/next ignore masks inherit "ignore".
    QCOMPARE(a.prevIgnore, 64u);
    QCOMPARE(a.nextIgnore, 64u);
    QVERIFY(a.doStdErr);
    QVERIFY(a.doAbs);
    QCOMPARE(a.color[1], 0.5f);

    // Only one baseline limit given: no baseline correction.
    const AverageCategory& b = d.categories[1];
    QCOMPARE(b.comment, QString("second"));
    QVERIFY(!b.doBaseline);
    QVERIFY(!b.doStdErr);
    QCOMPARE(b.ignore, 0u);
}

//=============================================================================================================

void TestMneDescriptionParser::average_masks()
{
    const QString path = write(R"(average {
    category {
        name c
        event 1
        tmin -0.1
        tmax 0.1
        mask 255
        prevmask 15
        nextignore 7
        prevignore 3
    }
})");

    AverageDescription d;
    QVERIFY(MNEDescriptionParser::parseAverageFile(path, d));
    const AverageCategory& c = d.categories[0];
    // A mask keeps the given trigger lines, i.e. ignores the complement.
    QCOMPARE(c.ignore, ~255u);
    // Later keywords overwrite earlier ones, as in interpret.c.
    QCOMPARE(c.prevIgnore, 3u);
    QCOMPARE(c.nextIgnore, 7u);
    // Without a name the average gets the default comment.
    QCOMPARE(d.comment, QString("Average"));
}

//=============================================================================================================

void TestMneDescriptionParser::average_rejected_data()
{
    QTest::addColumn<QString>("content");

    const QString cat = "category { name c event 1 tmin -0.1 tmax 0.1 %1 }";
    const auto inCat = [&](const QString& s) {
        return QString("average { %1 }").arg(cat.arg(s));
    };

    QTest::newRow("no categories") << "average { name x }";
    QTest::newRow("empty file") << "# nothing";
    QTest::newRow("missing brace") << "average name x";
    QTest::newRow("stray brace") << "{ average { }";
    QTest::newRow("nested average") << "average { average { } }";
    QTest::newRow("category outside average") << "category { name c event 1 }";
    QTest::newRow("nested category") << QString("average { category { %1 } }").arg(cat.arg(""));
    QTest::newRow("category without name") << "average { category { event 1 tmin -0.1 tmax 0.1 } }";
    QTest::newRow("category without event") << "average { category { name c tmin -0.1 tmax 0.1 } }";
    QTest::newRow("empty time range") << "average { category { name c event 1 tmin 0.1 tmax 0.1 } }";
    QTest::newRow("reversed baseline") << inCat("bmin 0.05 bmax -0.05");
    QTest::newRow("zero event") << inCat("event 0");
    QTest::newRow("negative nextevent") << inCat("nextevent -1");
    QTest::newRow("zero prevevent") << inCat("prevevent 0");
    QTest::newRow("negative ignore") << inCat("ignore -1");
    QTest::newRow("negative prevignore") << inCat("prevignore -2");
    QTest::newRow("negative nextignore") << inCat("nextignore -3");
    QTest::newRow("zero mask") << inCat("mask 0");
    QTest::newRow("negative prevmask") << inCat("prevmask -1");
    QTest::newRow("zero nextmask") << inCat("nextmask 0");
    QTest::newRow("color above one") << inCat("color 1 2 0");
    QTest::newRow("color below zero") << inCat("color 0 -0.5 0");
    QTest::newRow("bad float") << inCat("tmin abc");
    QTest::newRow("bad integer") << inCat("event x");
    QTest::newRow("truncated value") << "average { category { name c event";
    QTest::newRow("rejection in category") << inCat("gradReject 1e-10");
    QTest::newRow("rejection outside average") << QString("gradReject 1e-10 average { %1 }").arg(cat.arg(""));
    QTest::newRow("bad rejection value") << QString("average { eegReject much %1 }").arg(cat.arg(""));
    QTest::newRow("outfile in category") << inCat("outfile x.fif");
    QTest::newRow("tmin outside category") << QString("average { tmin 0 %1 }").arg(cat.arg(""));
    QTest::newRow("stderr outside category") << QString("average { stderr %1 }").arg(cat.arg(""));
    QTest::newRow("name outside average") << "name x";
    QTest::newRow("fixskew in category") << inCat("fixskew");
}

//=============================================================================================================

void TestMneDescriptionParser::average_rejected()
{
    QFETCH(QString, content);

    AverageDescription d;
    QVERIFY(!MNEDescriptionParser::parseAverageFile(write(content), d));
}

//=============================================================================================================

void TestMneDescriptionParser::covariance_full()
{
    // Layout of MNE-sample-data/MEG/sample/audvis.cov plus every optional keyword.
    const QString path = write(R"(cov {
    outfile         audvis-cov.fif
    logfile         audvis-cov.log
    eventfile       audvis.eve
    keepsamplemean
    fixskew
    gradReject      4000e-13
    magReject       4e-12
    eegReject       80e-6
    eogReject       150e-6
    def {
        event   1
        event   2
        ignore  0
        tmin    -0.2
        tmax    0
        bmin    -0.2
        bmax    0
        delay   0.01
    }
    def {
        event   3
        mask    15
        tmin    -0.15
        tmax    0.1
    }
})");

    CovDescription d;
    QVERIFY(MNEDescriptionParser::parseCovarianceFile(path, d));

    QCOMPARE(d.filename, QString("audvis-cov.fif"));
    QCOMPARE(d.logFile, QString("audvis-cov.log"));
    QCOMPARE(d.eventFile, QString("audvis.eve"));
    QVERIFY(!d.removeSampleMean);
    QVERIFY(d.fixSkew);
    QCOMPARE(d.rej.megGradReject, 4000e-13f);
    QCOMPARE(d.rej.eegReject, 80e-6f);

    QCOMPARE(d.defs.size(), 2);
    QCOMPARE(d.defs[0].events, (QVector<unsigned int>{1, 2}));
    QVERIFY(d.defs[0].doBaseline);
    QCOMPARE(d.defs[0].delay, 0.01f);
    QCOMPARE(d.defs[1].ignore, ~15u);
    QVERIFY(!d.defs[1].doBaseline);
}

//=============================================================================================================

void TestMneDescriptionParser::covariance_rejected_data()
{
    QTest::addColumn<QString>("content");

    const auto inDef = [](const QString& s) {
        return QString("cov { def { event 1 tmin -0.1 tmax 0 %1 } }").arg(s);
    };

    QTest::newRow("no cov block") << "# only a comment";
    QTest::newRow("nested cov") << "cov { cov { } }";
    QTest::newRow("def outside cov") << "def { event 1 }";
    QTest::newRow("missing brace") << "cov def";
    QTest::newRow("empty time range") << "cov { def { event 1 tmin 0 tmax 0 } }";
    QTest::newRow("reversed baseline") << inDef("bmin 0 bmax -0.1");
    QTest::newRow("zero event") << inDef("event 0");
    QTest::newRow("negative ignore") << inDef("ignore -1");
    QTest::newRow("zero mask") << inDef("mask 0");
    QTest::newRow("outfile without value") << "cov { outfile";
    QTest::newRow("outfile in def") << inDef("outfile x.fif");
    QTest::newRow("keepsamplemean in def") << inDef("keepsamplemean");
    QTest::newRow("rejection in def") << inDef("eegReject 1e-4");
    QTest::newRow("tmin outside def") << "cov { tmin 0 }";
    QTest::newRow("bad float") << inDef("delay x");
}

//=============================================================================================================

void TestMneDescriptionParser::covariance_rejected()
{
    QFETCH(QString, content);

    CovDescription d;
    QVERIFY(!MNEDescriptionParser::parseCovarianceFile(write(content), d));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_GUILESS_MAIN(TestMneDescriptionParser)
#include "test_mne_description_parser.moc"
