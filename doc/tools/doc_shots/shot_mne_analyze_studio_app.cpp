//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_mne_analyze_studio_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Renderer for the `mne_analyze_studio_app` shot kind: the real Analyze Studio workbench in offline
 *           mode with a workflow graph previewed and optionally its source open in an editor tab.
 */

#include "shot_mne_analyze_studio_app.h"

#include "shot_app_common.h"
#include "shot_capture.h"
#include "shot_runner.h"

#include <mne_analyze_studio/workbench/mainwindow.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QListWidget>
#include <QSettings>
#include <QStandardPaths>
#include <QTabWidget>

namespace DOCSHOTS
{

namespace
{

QTabWidget* centerTabs(MNEANALYZESTUDIO::MainWindow& mw, const QString& anyTabTitle)
{
    for (QTabWidget* tabs : mw.findChildren<QTabWidget*>()) {
        for (int i = 0; i < tabs->count(); ++i) {
            if (tabs->tabText(i).contains(anyTabTitle)) {
                return tabs;
            }
        }
    }
    return nullptr;
}

} // namespace

bool renderMneAnalyzeStudioApp(const ShotSpec& spec, const QString& outPath, bool& skipped, QString& err)
{
    skipped = false;
    const QJsonObject setup = spec.setup;

    // Offline mode: no keychain prompt and no neuro-kernel / skill-host processes; settings stay isolated.
    qputenv("MNE_ANALYZE_STUDIO_OFFLINE", "1");
    QStandardPaths::setTestModeEnabled(true);
    QSettings(QStringLiteral("MNE-CPP"), QStringLiteral("MNEAnalyzeStudio")).clear();

    const QString workflow = setup.value(QStringLiteral("workflow")).toString();
    const QString path = QDir(QStringLiteral(MNE_ANALYZE_STUDIO_WORKFLOWS_DIR)).absoluteFilePath(workflow);
    if (workflow.isEmpty() || !QFileInfo::exists(path)) {
        err = QStringLiteral("workflow '%1' not found in %2").arg(workflow, QStringLiteral(MNE_ANALYZE_STUDIO_WORKFLOWS_DIR));
        return false;
    }

    MNEANALYZESTUDIO::MainWindow mw;
    forceQRhiNullOnRhiWidgets(&mw);

    if (setup.value(QStringLiteral("open_editor")).toBool(false)) {
        mw.openInitialFiles({path});
    }
    const QString parseError = mw.previewWorkflowFile(path);
    if (!parseError.isEmpty()) {
        err = QStringLiteral("cannot preview %1: %2").arg(workflow, parseError);
        return false;
    }

    const QString center = setup.value(QStringLiteral("center")).toString(QStringLiteral("graph"));
    if (center == QLatin1String("editor")) {
        QTabWidget* tabs = centerTabs(mw, QFileInfo(path).fileName());
        if (!tabs) {
            err = QStringLiteral("no editor tab for %1 (set open_editor)").arg(workflow);
            return false;
        }
        for (int i = 0; i < tabs->count(); ++i) {
            if (tabs->tabText(i).contains(QFileInfo(path).fileName())) {
                tabs->setCurrentIndex(i);
            }
        }
    } else if (center != QLatin1String("graph")) {
        err = QStringLiteral("unknown center '%1' (expected graph or editor)").arg(center);
        return false;
    }

    if (!showAtSize(mw, spec.size, err)) {
        return false;
    }
    // Log lines name absolute paths; show them relative to the repository so the image is machine independent.
    const QString root = QDir(QStringLiteral(MNE_DOC_SHOTS_REPO_ROOT)).absolutePath() + QLatin1Char('/');
    for (QListWidget* list : mw.findChildren<QListWidget*>()) {
        for (int i = 0; i < list->count(); ++i) {
            list->item(i)->setText(list->item(i)->text().replace(root, QString()));
        }
    }

    const bool ok = captureWindow(mw, spec.size, outPath, err);
    // closeEvent would persist the preview into the (test-mode) settings; nothing else needs it.
    mw.hide();
    return ok;
}

} // namespace DOCSHOTS
