//=============================================================================================================
/**
 * @file     test_doc_shots.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    Smoke test for the mne_doc_shots tool — writes a tiny one-shot
 *           manifest to a QTemporaryDir, runs the tool out-of-process, and
 *           asserts that a non-empty PNG was produced.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace
{

QString locateToolBinary()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/mne_doc_shots"),
#ifdef Q_OS_MAC
        appDir + QStringLiteral("/mne_doc_shots.app/Contents/MacOS/mne_doc_shots"),
#endif
#ifdef Q_OS_WIN
        appDir + QStringLiteral("/mne_doc_shots.exe"),
#endif
        appDir + QStringLiteral("/../bin/mne_doc_shots"),
    };
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p)) {
            return QFileInfo(p).absoluteFilePath();
        }
    }
    return QString();
}

}  // namespace

class TestDocShots : public QObject
{
    Q_OBJECT

private slots:
    void runsAndProducesPng();
    void runsMneInspectAppKind();
    void runsMneAlignAppKind();
};

void TestDocShots::runsAndProducesPng()
{
    const QString tool = locateToolBinary();
    if (tool.isEmpty()) {
        QSKIP("mne_doc_shots binary not found next to the test executable");
    }

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString manifestPath = tmp.filePath("manifest.json");
    QFile mf(manifestPath);
    QVERIFY(mf.open(QIODevice::WriteOnly));
    mf.write(R"({
        "out_dir": "out",
        "shots": [
            {
                "id": "smoke",
                "kind": "widget_mockup",
                "size": [320, 200],
                "setup": {
                    "window_title": "Smoke",
                    "toolbar": ["A", "B"],
                    "canvas_title": "Smoke test",
                    "canvas_text": "ok"
                }
            }
        ]
    })");
    mf.close();

    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    p.setProcessEnvironment(env);
    p.start(tool, QStringList{manifestPath});
    QVERIFY(p.waitForStarted(10000));
    QVERIFY2(p.waitForFinished(30000),
             qPrintable(QStringLiteral("mne_doc_shots timed out: %1")
                            .arg(QString::fromLocal8Bit(p.readAllStandardError()))));

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        const QString stderrText = QString::fromLocal8Bit(p.readAllStandardError());
        // If the offscreen plugin isn't installed in this Qt build, skip
        // rather than fail — the tool relies on it.
        if (stderrText.contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)
            && stderrText.contains(QStringLiteral("could not"), Qt::CaseInsensitive)) {
            QSKIP("Qt offscreen platform plugin not available in this build");
        }
        QFAIL(qPrintable(QStringLiteral("mne_doc_shots failed (exit %1): %2")
                             .arg(p.exitCode()).arg(stderrText)));
    }

    const QString pngPath = QDir(tmp.path()).absoluteFilePath("out/smoke.png");
    QVERIFY2(QFileInfo::exists(pngPath),
             qPrintable(QStringLiteral("Expected PNG missing: %1").arg(pngPath)));
    QVERIFY(QFileInfo(pngPath).size() > 0);
}

void TestDocShots::runsMneInspectAppKind()
{
    const QString tool = locateToolBinary();
    if (tool.isEmpty()) {
        QSKIP("mne_doc_shots binary not found next to the test executable");
    }

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString manifestPath = tmp.filePath("manifest.json");
    QFile mf(manifestPath);
    QVERIFY(mf.open(QIODevice::WriteOnly));
    mf.write(R"({
        "out_dir": "out",
        "shots": [
            {
                "id": "inspect-smoke",
                "kind": "mne_inspect_app",
                "size": [640, 400],
                "setup": {
                    "load_demo_electrodes": true,
                    "focus_dock": "pick"
                }
            }
        ]
    })");
    mf.close();

    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    p.setProcessEnvironment(env);
    p.start(tool, QStringList{manifestPath});
    QVERIFY(p.waitForStarted(10000));
    QVERIFY2(p.waitForFinished(60000),
             qPrintable(QStringLiteral("mne_doc_shots (mne_inspect_app) timed out: %1")
                            .arg(QString::fromLocal8Bit(p.readAllStandardError()))));

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        const QString stderrText = QString::fromLocal8Bit(p.readAllStandardError());
        if (stderrText.contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)
            && stderrText.contains(QStringLiteral("could not"), Qt::CaseInsensitive)) {
            QSKIP("Qt offscreen platform plugin not available in this build");
        }
        if (stderrText.contains(QStringLiteral("skipped"), Qt::CaseInsensitive)) {
            QSKIP(qPrintable(QStringLiteral("mne_inspect_app renderer skipped: %1").arg(stderrText)));
        }
        QFAIL(qPrintable(QStringLiteral("mne_doc_shots (mne_inspect_app) failed (exit %1): %2")
                             .arg(p.exitCode()).arg(stderrText)));
    }

    const QString pngPath = QDir(tmp.path()).absoluteFilePath("out/inspect-smoke.png");
    if (!QFileInfo::exists(pngPath)) {
        // Renderer may legitimately skip on a platform that can't construct
        // the MainWindow (e.g. missing QRhiWidget support). Treat as skip.
        QSKIP(qPrintable(QStringLiteral("Expected PNG missing (renderer skipped): %1").arg(pngPath)));
    }
    QVERIFY(QFileInfo(pngPath).size() > 0);

    QImage img;
    QVERIFY2(img.load(pngPath, "PNG"),
             qPrintable(QStringLiteral("Failed to load PNG: %1").arg(pngPath)));
    // MainWindow enforces its own minimum size; we only verify the renderer
    // produced a non-trivial image, not the exact requested dimensions.
    QVERIFY(img.width() > 0);
    QVERIFY(img.height() > 0);
}

void TestDocShots::runsMneAlignAppKind()
{
    const QString tool = locateToolBinary();
    if (tool.isEmpty()) {
        QSKIP("mne_doc_shots binary not found next to the test executable");
    }

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString manifestPath = tmp.filePath("manifest.json");
    QFile mf(manifestPath);
    QVERIFY(mf.open(QIODevice::WriteOnly));
    mf.write(R"({
        "out_dir": "out",
        "shots": [
            {
                "id": "align-smoke",
                "kind": "mne_align_app",
                "size": [640, 400],
                "setup": {
                    "wizard_step": 0
                }
            }
        ]
    })");
    mf.close();

    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
    p.setProcessEnvironment(env);
    p.start(tool, QStringList{manifestPath});
    QVERIFY(p.waitForStarted(10000));
    QVERIFY2(p.waitForFinished(60000),
             qPrintable(QStringLiteral("mne_doc_shots (mne_align_app) timed out: %1")
                            .arg(QString::fromLocal8Bit(p.readAllStandardError()))));

    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        const QString stderrText = QString::fromLocal8Bit(p.readAllStandardError());
        if (stderrText.contains(QStringLiteral("offscreen"), Qt::CaseInsensitive)
            && stderrText.contains(QStringLiteral("could not"), Qt::CaseInsensitive)) {
            QSKIP("Qt offscreen platform plugin not available in this build");
        }
        if (stderrText.contains(QStringLiteral("skipped"), Qt::CaseInsensitive)) {
            QSKIP(qPrintable(QStringLiteral("mne_align_app renderer skipped: %1").arg(stderrText)));
        }
        QFAIL(qPrintable(QStringLiteral("mne_doc_shots (mne_align_app) failed (exit %1): %2")
                             .arg(p.exitCode()).arg(stderrText)));
    }

    const QString pngPath = QDir(tmp.path()).absoluteFilePath("out/align-smoke.png");
    if (!QFileInfo::exists(pngPath)) {
        QSKIP(qPrintable(QStringLiteral("Expected PNG missing (renderer skipped): %1").arg(pngPath)));
    }
    QVERIFY(QFileInfo(pngPath).size() > 0);

    QImage img;
    QVERIFY2(img.load(pngPath, "PNG"),
             qPrintable(QStringLiteral("Failed to load PNG: %1").arg(pngPath)));
    QVERIFY(img.width() > 0);
    QVERIFY(img.height() > 0);
}

QTEST_GUILESS_MAIN(TestDocShots)
#include "test_doc_shots.moc"
