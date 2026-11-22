//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     shot_mne_scan_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 *
 * @brief    Renderer for the `mne_scan_app` shot kind: the real MNE Scan main window with the plugins of this
 *           build, an optional pipeline, a selected plugin or an open plugin menu.
 */

#include "shot_mne_scan_app.h"

#include "shot_app_common.h"
#include "shot_capture.h"
#include "shot_runner.h"

#include <mne_scan/mne_scan/mainwindow.h>
#include <mne_scan/mne_scan/plugingui.h>
#include <mne_scan/mne_scan/pluginitem.h>

#include <mne/mna/mna_io.h>
#include <mne/mna/mna_project.h>
#include <mne/base/string_utils.h>
#include <scMeas/measurementtypes.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QToolButton>

#include <format>
#include <string>

// Q_INIT_RESOURCE must be used outside a namespace; mne_scan.qrc lives in the static mne_scan_app_core.
static void initScanResources()
{
    Q_INIT_RESOURCE(mne_scan);
}

namespace DOCSHOTS
{

namespace
{

bool writePipeline(const QJsonArray& pipeline, const QString& path, QString& err)
{
    mne::MnaProject project;
    project.name = "MNE Scan Pipeline";
    std::string previous;
    for (int i = 0; i < pipeline.size(); ++i) {
        const QJsonObject entry = pipeline.at(i).toObject();
        mne::MnaNode node;
        node.id = std::format("node_{}", i);
        node.op_type = entry.value(QStringLiteral("plugin")).toString().toStdString();
        node.attributes.insert("gui_x", entry.value(QStringLiteral("x")).toDouble());
        node.attributes.insert("gui_y", entry.value(QStringLiteral("y")).toDouble());
        if (!previous.empty()) {
            mne::MnaPort input;
            input.name = "in";
            input.source_node_id = previous;
            input.source_port_name = "out";
            node.inputs.push_back(std::move(input));
        }
        previous = node.id;
        project.pipeline.push_back(std::move(node));
    }
    if (const auto written = mne::MnaIO::write(project, mne::path_from_utf8(path.toStdString())); !written) {
        err = QString::fromUtf8(written.error().what());
        return false;
    }
    return true;
}

QGraphicsScene* pluginScene(MNESCAN::PluginGui& gui)
{
    auto* view = gui.findChild<QGraphicsView*>();
    return view ? view->scene() : nullptr;
}

QList<MNESCAN::PluginItem*> pluginItems(QGraphicsScene& scene)
{
    QList<MNESCAN::PluginItem*> items;
    for (QGraphicsItem* item : scene.items()) {
        if (auto* pluginItem = qgraphicsitem_cast<MNESCAN::PluginItem*>(item); pluginItem && pluginItem->plugin()) {
            items << pluginItem;
        }
    }
    return items;
}

QToolButton* menuButton(MNESCAN::PluginGui& gui, const QString& which)
{
    const QString tip = which == QLatin1String("sensor") ? QStringLiteral("Sensor Plugins")
                                                         : QStringLiteral("Algorithm Plugins");
    for (QToolButton* button : gui.findChildren<QToolButton*>()) {
        if (button->toolTip() == tip && button->menu()) {
            return button;
        }
    }
    return nullptr;
}

} // namespace

bool renderMneScanApp(const ShotSpec& spec, const QString& outPath, bool& skipped, QString& err)
{
    skipped = false;
    const QJsonObject setup = spec.setup;

    // Keep the user's MNE Scan settings and its auto-saved default.mna out of the capture.
    QStandardPaths::setTestModeEnabled(true);
    QSettings settings(QStringLiteral("MNECPP"));
    settings.remove(QStringLiteral("MNEScan"));
    settings.remove(QStringLiteral("MNESCAN"));
    settings.sync();

    if (qEnvironmentVariableIsEmpty("MNE_SCAN_PLUGIN_DIR")) {
        qputenv("MNE_SCAN_PLUGIN_DIR", QByteArray(MNE_SCAN_PLUGIN_DIR));
    }
    if (!QDir(qEnvironmentVariable("MNE_SCAN_PLUGIN_DIR")).exists()) {
        err = QStringLiteral("MNE Scan plugin directory %1 does not exist; build the scan_* plugin targets")
                  .arg(qEnvironmentVariable("MNE_SCAN_PLUGIN_DIR"));
        return false;
    }

    static const bool typesRegistered = [] {
        initScanResources();
        mne::scan::MeasurementTypes::registerTypes();
        return true;
    }();
    Q_UNUSED(typesRegistered);

    const QFont deterministicFont = QApplication::font();
    MNESCAN::MainWindow mw;
    // MNE Scan applies its own Roboto font; screenshots keep the bundled one
    QApplication::setFont(deterministicFont);
    forceQRhiNullOnRhiWidgets(&mw);
    auto* gui = mw.findChild<MNESCAN::PluginGui*>();
    if (!gui) {
        err = QStringLiteral("MNE Scan main window has no plugin GUI");
        return false;
    }

    const QJsonArray pipeline = setup.value(QStringLiteral("pipeline")).toArray();
    if (!pipeline.isEmpty()) {
        QTemporaryDir dir;
        if (!dir.isValid() || !writePipeline(pipeline, dir.filePath(QStringLiteral("pipeline.mna")), err)) {
            err = err.isEmpty() ? QStringLiteral("cannot create a temporary directory") : err;
            return false;
        }
        gui->loadConfig(dir.path(), QStringLiteral("pipeline.mna"));
    }

    QGraphicsScene* scene = pluginScene(*gui);
    if (!scene) {
        err = QStringLiteral("MNE Scan plugin GUI has no scene");
        return false;
    }
    const QList<MNESCAN::PluginItem*> placed = pluginItems(*scene);
    if (placed.size() != pipeline.size()) {
        QStringList available;
        for (const QString& which : {QStringLiteral("sensor"), QStringLiteral("algorithm")}) {
            if (QToolButton* button = menuButton(*gui, which)) {
                for (const QAction* action : button->menu()->actions()) {
                    available << action->text();
                }
            }
        }
        err = QStringLiteral("pipeline has %1 plugins but %2 were placed (plugins of this build: %3)")
                  .arg(pipeline.size())
                  .arg(placed.size())
                  .arg(available.join(QStringLiteral(", ")));
        return false;
    }

    if (setup.contains(QStringLiteral("select"))) {
        const QString name = setup.value(QStringLiteral("select")).toString();
        scene->clearSelection();
        MNESCAN::PluginItem* target = nullptr;
        for (MNESCAN::PluginItem* item : placed) {
            if (item->plugin()->getName() == name) {
                target = item;
            }
        }
        if (!target) {
            err = QStringLiteral("no plugin named '%1' in the pipeline").arg(name);
            return false;
        }
        target->setSelected(true);
    }

    if (!showAtSize(mw, spec.size, err)) {
        return false;
    }

    const QString menuName = setup.value(QStringLiteral("open_menu")).toString();
    if (menuName.isEmpty()) {
        return captureWindow(mw, spec.size, outPath, err);
    }

    // Popup menus are separate top-level windows, so the open menu is painted into the window image.
    QToolButton* button = menuButton(*gui, menuName);
    if (!button) {
        err = QStringLiteral("unknown open_menu '%1' (expected sensor or algorithm)").arg(menuName);
        return false;
    }
    QMenu* menu = button->menu();
    menu->adjustSize();
    if (!waitUntilSettled(mw, err)) {
        return false;
    }
    QImage image = mw.grab().toImage();
    QPainter painter(&image);
    const QPoint anchor = button->mapTo(&mw, QPoint(button->width(), 0));
    painter.drawImage(anchor, menu->grab().toImage());
    painter.end();
    return savePng(image, outPath, err);
}

} // namespace DOCSHOTS
