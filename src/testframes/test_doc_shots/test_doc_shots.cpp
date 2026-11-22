//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_doc_shots.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Tests for mne_doc_shots: deterministic output at the declared size, the readiness barrier, and
 *           failure diagnostics.
 */

#include "shot_capture.h"

#include <mne/base/thread_pool.h>

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <QtTest/QtTest>

#include <cstdio>

namespace
{

//=============================================================================================================
/**
 * Paints a solid colour; tests change it from timers or worker threads to emulate late rendering.
 */
class ColourWidget : public QWidget
{
public:
    void setColour(const QColor& colour)
    {
        m_colour = colour;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter(this).fillRect(rect(), m_colour);
    }

private:
    QColor m_colour = Qt::red;
};

/** Paints slowly, like a heavy view on a loaded machine. */
class SlowWidget : public ColourWidget
{
protected:
    void paintEvent(QPaintEvent* event) override
    {
        QThread::msleep(150);
        ColourWidget::paintEvent(event);
    }
};

struct ToolRun
{
    int exitCode = -1;
    QString out;
    QString err;
};

ToolRun runTool(const QString& manifestPath, const QString& outDir)
{
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    process.setProcessEnvironment(env);
    process.start(QStringLiteral(MNE_DOC_SHOTS_PATH),
                  {manifestPath, QStringLiteral("--out"), outDir, QStringLiteral("--force")});
    ToolRun run;
    if (!process.waitForStarted(10000) || !process.waitForFinished(180000)) {
        run.err = QStringLiteral("mne_doc_shots did not start or finish: %1").arg(process.errorString());
        return run;
    }
    run.exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    run.out = QString::fromLocal8Bit(process.readAllStandardOutput());
    run.err = QString::fromLocal8Bit(process.readAllStandardError()) +
        QStringLiteral("\n[exit status %1, code %2]")
            .arg(process.exitStatus() == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash"))
            .arg(static_cast<quint32>(process.exitCode()), 0, 16);
    return run;
}

bool writeManifest(const QString& path, const QByteArray& shots)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write("{\"out_dir\": \"out\", \"shots\": " + shots + "}") > 0;
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace

class TestDocShots : public QObject
{
    Q_OBJECT

private slots:
    void waitsForTimerDrivenRepaints();
    void waitsForThreadPoolWork();
    void waitsForTimersThatFallDueDuringASlowGrab();
    void reportsWindowsThatNeverSettle();
    void savesOpaquePngAtomically();
    void producesDeclaredSizeDeterministically();
    void failsWithDiagnostics();
};

void TestDocShots::waitsForTimerDrivenRepaints()
{
    ColourWidget widget;
    widget.resize(64, 48);
    widget.show();
    int step = 0;
    QTimer timer;
    connect(&timer, &QTimer::timeout, &widget, [&]() {
        widget.setColour(++step < 6 ? QColor::fromHsv(step * 40, 255, 255) : QColor(Qt::green));
        if (step == 6) {
            timer.stop();
        }
    });
    timer.start(50);

    QString err;
    QVERIFY2(DOCSHOTS::waitUntilSettled(widget, err), qPrintable(err));
    QCOMPARE(step, 6);
    QCOMPARE(widget.grab().toImage().pixelColor(10, 10), QColor(Qt::green));
}

void TestDocShots::waitsForThreadPoolWork()
{
    ColourWidget widget;
    widget.resize(64, 48);
    widget.show();
    mne::ThreadPool::global().submit([&widget]() {
        QThread::msleep(600);
        QMetaObject::invokeMethod(&widget, [&widget]() { widget.setColour(Qt::blue); }, Qt::QueuedConnection);
    });

    QString err;
    QVERIFY2(DOCSHOTS::waitUntilSettled(widget, err, 200), qPrintable(err));
    QCOMPARE(widget.grab().toImage().pixelColor(10, 10), QColor(Qt::blue));
}

void TestDocShots::waitsForTimersThatFallDueDuringASlowGrab()
{
    SlowWidget widget;
    widget.resize(64, 48);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    // Falls due while the second 150 ms grab runs, after the frame has already looked unchanged for 100 ms.
    QTimer::singleShot(230, &widget, [&widget]() { widget.setColour(Qt::green); });

    QString err;
    QVERIFY2(DOCSHOTS::waitUntilSettled(widget, err, 100), qPrintable(err));
    QCOMPARE(widget.grab().toImage().pixelColor(10, 10), QColor(Qt::green));
}

void TestDocShots::reportsWindowsThatNeverSettle()
{
    ColourWidget widget;
    widget.resize(64, 48);
    widget.show();
    bool flip = false;
    QTimer timer;
    connect(&timer, &QTimer::timeout, &widget, [&]() { widget.setColour((flip = !flip) ? Qt::black : Qt::white); });
    timer.start(20);

    QString err;
    QVERIFY(!DOCSHOTS::waitUntilSettled(widget, err, 200, 1000));
    QVERIFY2(err.contains(QStringLiteral("did not settle within 1000 ms")), qPrintable(err));
}

void TestDocShots::savesOpaquePngAtomically()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QImage image(32, 16, QImage::Format_ARGB32);
    image.fill(QColor(10, 200, 30, 128));
    const QString path = tmp.filePath(QStringLiteral("shot.png"));

    QString err;
    QVERIFY2(DOCSHOTS::savePng(QImage(8, 8, QImage::Format_RGB32), path, err), qPrintable(err));
    QVERIFY2(DOCSHOTS::savePng(image, path, err), qPrintable(err));
    QImage saved(path);
    QCOMPARE(saved.size(), QSize(32, 16));
    QVERIFY(!saved.hasAlphaChannel());
    QCOMPARE(QDir(tmp.path()).entryList(QDir::Files), QStringList{QStringLiteral("shot.png")});
}

void TestDocShots::producesDeclaredSizeDeterministically()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString manifest = tmp.filePath(QStringLiteral("manifest.json"));
    QVERIFY(writeManifest(manifest, R"([
        {"id": "mockup", "kind": "widget_mockup", "size": [800, 600],
         "setup": {"toolbar": ["Open", "Run"], "canvas_title": "Scene", "status": "Ready"}},
        {"id": "align/fiducials", "kind": "mne_align_app", "size": [1100, 700],
         "setup": {"wizard_step": 1, "simulate_capture": {"kind": "fiducial", "count": 2}}},
        {"id": "inspect/pick", "kind": "mne_inspect_app", "size": [1440, 900],
         "setup": {"load_demo_electrodes": true, "focus_dock": "pick",
                   "simulate_pick": {"kind": "contact", "target": ["LA", "LA2"]}}},
        {"id": "scan/pipeline", "kind": "mne_scan_app", "size": [1280, 800],
         "setup": {"pipeline": [{"plugin": "Fiff Simulator", "x": -150, "y": 0},
                                {"plugin": "Write To File", "x": 150, "y": 0}],
                   "select": "Write To File"}},
        {"id": "studio/graph", "kind": "mne_analyze_studio_app", "size": [1440, 900],
         "setup": {"workflow": "temporal_filter_demo.mna", "open_editor": true}}
    ])"));
    const QList<QPair<QString, QSize>> shots{{QStringLiteral("mockup"), QSize(800, 600)},
                                             {QStringLiteral("align/fiducials"), QSize(1100, 700)},
                                             {QStringLiteral("inspect/pick"), QSize(1440, 900)},
                                             {QStringLiteral("scan/pipeline"), QSize(1280, 800)},
                                             {QStringLiteral("studio/graph"), QSize(1440, 900)}};

    // Concurrent runs must not read each other's saved pipelines, so nothing may persist.
    QStandardPaths::setTestModeEnabled(true);
    QDir shared(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/mne_doc_shots"));
    shared.removeRecursively();

    const ToolRun first = runTool(manifest, tmp.filePath(QStringLiteral("first")));
    QVERIFY2(first.exitCode == 0, qPrintable(first.out + first.err));
    QVERIFY2(!shared.exists(), qPrintable(shared.path()));
    QVERIFY2(first.out.contains(QStringLiteral("font 'DejaVu Sans' 13px, device pixel ratio 1")),
             qPrintable(first.out));
    const ToolRun second = runTool(manifest, tmp.filePath(QStringLiteral("second")));
    QVERIFY2(second.exitCode == 0, qPrintable(second.out + second.err));

    for (const auto& [id, size] : shots) {
        const QString a = tmp.filePath(QStringLiteral("first/%1.png").arg(id));
        const QString b = tmp.filePath(QStringLiteral("second/%1.png").arg(id));
        const QImage image(a);
        QCOMPARE(image.size(), size);
        QVERIFY(!image.hasAlphaChannel());
        QVERIFY2(readFile(a) == readFile(b), qPrintable(QStringLiteral("%1 differs between two runs").arg(id)));
    }
}

void TestDocShots::failsWithDiagnostics()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString manifest = tmp.filePath(QStringLiteral("manifest.json"));
    QVERIFY(writeManifest(manifest, R"([
        {"id": "too-small", "kind": "widget_mockup", "size": [320, 200], "setup": {}},
        {"id": "no-kind", "kind": "does_not_exist", "size": [800, 600]},
        {"id": "no-fixture", "kind": "mne_align_app", "size": [1100, 700], "setup": {"fixtures": ["nope"]}},
        {"id": "fine", "kind": "widget_mockup", "size": [800, 600], "setup": {}}
    ])"));
    const QString out = tmp.filePath(QStringLiteral("out"));
    QVERIFY(QDir().mkpath(out));
    QFile stale(out + QStringLiteral("/too-small.png"));
    QVERIFY(stale.open(QIODevice::WriteOnly) && stale.write("stale") > 0);
    stale.close();

    const ToolRun run = runTool(manifest, out);
    QCOMPARE(run.exitCode, 1);
    for (const QString& expected : {QStringLiteral("FAIL    too-small: window is"),
                                    QStringLiteral("requested 320x200"),
                                    QStringLiteral("has minimum size 640x480"),
                                    QStringLiteral("Unknown shot kind: does_not_exist"),
                                    QStringLiteral("unknown fixture 'nope'"),
                                    QStringLiteral("1 generated, 0 cached, 0 skipped, 3 failed")}) {
        QVERIFY2((run.out + run.err).contains(expected), qPrintable(expected + QStringLiteral(" missing in:\n") + run.out + run.err));
    }
    QVERIFY(!QFileInfo::exists(out + QStringLiteral("/too-small.png")));
    QVERIFY(!QFileInfo::exists(out + QStringLiteral("/no-fixture.png")));
    QCOMPARE(QImage(out + QStringLiteral("/fine.png")).size(), QSize(800, 600));
    QCOMPARE(QDir(out).entryList(QDir::Files), QStringList{QStringLiteral("fine.png")});
}

int main(int argc, char* argv[])
{
    DOCSHOTS::prepareProcessEnvironment();
    QApplication app(argc, argv);
    QString err;
    if (!DOCSHOTS::applyDeterministicTheme(app, err)) {
        std::fprintf(stderr, "test_doc_shots: cannot pin the capture theme: %s\n", qPrintable(err));
        return 1;
    }
    TestDocShots test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_doc_shots.moc"
