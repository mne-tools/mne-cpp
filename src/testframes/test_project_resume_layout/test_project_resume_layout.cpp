//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *

 * @file     test_project_resume_layout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @brief    Tests that QMainWindow saveState/restoreState bytes round-trip through
 *           the MnaProject extras section (used by MNE Scan's project resume).
 */

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>

#include <mne/base/json.h>
#include <mne/base/string_utils.h>

#include <filesystem>
#include <string>

#include <QByteArray>
#include <QDir>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QObject>
#include <QTemporaryDir>
#include <QTest>

using namespace mne;

namespace
{

constexpr const char* kLayoutKeyPrefix = "ui.layout.";

QString layoutKey(const QString& windowKey)
{
    return QString::fromLatin1(kLayoutKeyPrefix) + windowKey;
}

QMainWindow* buildSyntheticMainWindow()
{
    auto* w = new QMainWindow();
    w->setObjectName(QStringLiteral("SyntheticMainWindow"));
    w->resize(1024, 768);

    auto* dockA = new QDockWidget(QStringLiteral("DockA"), w);
    dockA->setObjectName(QStringLiteral("DockA"));
    dockA->setMinimumSize(120, 80);
    w->addDockWidget(Qt::LeftDockWidgetArea, dockA);

    auto* dockB = new QDockWidget(QStringLiteral("DockB"), w);
    dockB->setObjectName(QStringLiteral("DockB"));
    dockB->setMinimumSize(140, 90);
    w->addDockWidget(Qt::RightDockWidgetArea, dockB);

    auto* dockC = new QDockWidget(QStringLiteral("DockC"), w);
    dockC->setObjectName(QStringLiteral("DockC"));
    dockC->setMinimumSize(100, 60);
    w->addDockWidget(Qt::BottomDockWidgetArea, dockC);

    return w;
}

} // namespace

class TestProjectResumeLayout : public QObject
{
    Q_OBJECT

private slots:
    void layoutBlob_roundTripsThroughMnaExtras_data();
    void layoutBlob_roundTripsThroughMnaExtras();
};

//=============================================================================================================

void TestProjectResumeLayout::layoutBlob_roundTripsThroughMnaExtras_data()
{
    QTest::addColumn<QString>("extension");
    QTest::newRow("mna_json") << QStringLiteral("mna");
    QTest::newRow("mnx_cbor") << QStringLiteral("mnx");
}

//=============================================================================================================

void TestProjectResumeLayout::layoutBlob_roundTripsThroughMnaExtras()
{
    QFETCH(QString, extension);

    QTemporaryDir tmp(QDir::temp().filePath(QStringLiteral("test_proj_resume_XXXXXX")));
    QVERIFY(tmp.isValid());
    const QString path = tmp.filePath(QStringLiteral("synthetic.") + extension);

    // 1. Build a synthetic QMainWindow with three docks and capture its state.
    QByteArray expectedState;
    {
        QMainWindow* w = buildSyntheticMainWindow();
        expectedState = w->saveState();
        QVERIFY2(!expectedState.isEmpty(), "QMainWindow::saveState() returned empty");
        delete w;
    }

    // 2. Build a minimal MnaProject, stash the layout blob (base64) under ui.layout.MainWindow.
    MnaProject proj;
    proj.name = "Synthetic";

    const std::string key = layoutKey(QStringLiteral("MainWindow")).toStdString();
    proj.extras.insert(key, expectedState.toBase64().toStdString());

    // 3. Persist via MnaIO (mna -> JSON, mnx -> CBOR).
    const std::filesystem::path file = path_from_utf8(path.toStdString());
    QVERIFY(MnaIO::write(proj, file));

    // 4. Reopen and read back through the same code path used by MNE Scan.
    const MnaProject reloaded = MnaIO::read(file).value();
    QVERIFY2(reloaded.extras.contains(key), (key + " missing; have: " + JsonDocument(reloaded.extras).to_json(JsonDocument::Compact)).c_str());

    const std::string_view restoredBase64 = reloaded.extras[key].to_string();
    QVERIFY(!restoredBase64.empty());
    const QByteArray restored = QByteArray::fromBase64(QByteArray(restoredBase64.data(), qsizetype(restoredBase64.size())));
    QCOMPARE(restored, expectedState);

    // 5. Apply restored bytes to a *fresh* QMainWindow and verify restoreState succeeds.
    QMainWindow* fresh = buildSyntheticMainWindow();
    QVERIFY2(fresh->restoreState(restored),
             "QMainWindow::restoreState() rejected the round-tripped bytes");
    // After restore, the bytes round-trip exactly.
    QCOMPARE(fresh->saveState(), expectedState);
    delete fresh;
}

QTEST_MAIN(TestProjectResumeLayout)
#include "test_project_resume_layout.moc"
