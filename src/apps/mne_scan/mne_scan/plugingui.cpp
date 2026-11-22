//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     plugingui.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief     PluginGui class implementation
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "plugingui.h"

#include "arrow.h"
#include "pluginitem.h"
#include "pluginscene.h"

#include <scShared/Plugins/abstractplugin.h>
#include <scShared/Plugins/abstractsensor.h>
#include <scShared/Plugins/abstractalgorithm.h>

#include <mne/mna/mna_project.h>
#include <mne/mna/mna_io.h>
#include <mne/mna/mna_graph.h>
#include <mne/mna/mna_node.h>
#include <scShared/Management/mna_scan_types.h>

#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QFileInfo>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginGui::PluginGui(mne::scan::PluginManager* pPluginManager,
                     mne::scan::PluginSceneManager* pPluginSceneManager)
: m_pPluginManager(pPluginManager)
, m_pPluginSceneManager(pPluginSceneManager)
, m_pCurrentPlugin(0)
, m_pGraphicsView(nullptr)
, m_pSensorToolButton(nullptr)
, m_pAlgorithmToolButton(nullptr)
, m_pToolBarPlugins(nullptr)
, m_pPointerButton(nullptr)
, m_pLinePointerButton(nullptr)
, m_pToolBarPointer(nullptr)
, m_pToolBarItem(nullptr)
{
    createActions();
    createMenuItem();
    createToolbars();

    m_pPluginScene = new PluginScene(m_pMenuItem, this);
    //m_pPluginScene->setSceneRect(0, 0, 200, 500);

    connect(m_pPluginScene, &PluginScene::itemInserted,
            this, &PluginGui::itemInserted);

    connect(m_pPluginScene, &PluginScene::selectionChanged,
            this, &PluginGui::newItemSelected);

    m_pGraphicsView = new QGraphicsView(m_pPluginScene);
    //
    // Without an explicit scene rect QGraphicsScene derives one from
    // itemsBoundingRect(), which grows as plugins are added and never shrinks.
    // The scrollbars are then rescaled underneath the user, which is what makes
    // the scene appear to jump to an unrelated position after an insertion.
    // Keep the view anchored on what is under the mouse instead.
    //
    m_pGraphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    m_pGraphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setCentralWidget(m_pGraphicsView);

    setWindowTitle(tr("PluginScene"));
    setUnifiedTitleAndToolBarOnMac(true);

    //To prevent deadlock on loading with a broken plugin -> save loading state
    QSettings settings("MNECPP");
    bool loadingState = settings.value(QString("MNEScan/loadingState"), false).toBool();

    if (loadingState) {
        settings.setValue(QString("MNEScan/loadingState"), false);

        QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (QFile::exists(appData + QStringLiteral("/default.mna"))) {
            loadConfig(appData, QStringLiteral("default.mna"));
        }
    }

    settings.setValue(QString("MNEScan/loadingState"), true);

    m_pGraphicsView->setMinimumWidth(200);
    updateSceneRect();
    m_pGraphicsView->ensureVisible(m_pPluginScene->itemsBoundingRect());
}

//=============================================================================================================

void PluginGui::updateSceneRect()
{
    if (!m_pPluginScene || !m_pGraphicsView) {
        return;
    }

    //
    // Give the scene a rect that covers the items plus a margin, so there is
    // room to drag a plugin outwards without the scene resizing on every move.
    // It is never allowed to shrink below the viewport, which would otherwise
    // make the scrollbars appear and disappear as items are moved around.
    //
    const qreal margin = 200.0;
    QRectF rect = m_pPluginScene->itemsBoundingRect().adjusted(-margin, -margin, margin, margin);

    const QRectF viewRect = m_pGraphicsView->viewport()->rect();
    if (rect.width() < viewRect.width() || rect.height() < viewRect.height()) {
        rect.setSize(rect.size().expandedTo(viewRect.size()));
    }

    m_pPluginScene->setSceneRect(rect);
}

//=============================================================================================================

PluginGui::~PluginGui()
{
    //
    // Save current configuration (MNA format)
    //
    saveConfig(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "default.mna");

    m_pCurrentPlugin.reset();

    // The tool bars, their buttons and the graphics view are children of this window and are destroyed with it.
}

//=============================================================================================================

void PluginGui::clearScene()
{
    foreach (QGraphicsItem* item, m_pPluginScene->items()) {
        if (item->type() == PluginItem::Type) {
            if (removePlugin(qgraphicsitem_cast<PluginItem*>(item)->plugin())) {
                qgraphicsitem_cast<PluginItem*>(item)->removeArrows();
                m_pPluginScene->removeItem(item);
                // removeItem() hands the ownership of the item back to the caller
                std::unique_ptr<QGraphicsItem> pRemoved(item);
            }
        }
    }
}

//=============================================================================================================

void PluginGui::loadConfig(const QString& sPath, const QString& sFileName)
{
    mne::Log::debug("load {}", (sPath + "/" + sFileName).toStdString());

    QString fullPath = sPath + QStringLiteral("/") + sFileName;

    loadConfigMna(fullPath);
}

//=============================================================================================================

void PluginGui::loadConfigMna(const QString& fullPath)
{
    mne::Expected<mne::MnaProject> read = mne::MnaIO::read(mne::path_from_utf8(fullPath.toStdString()));
    if (!read || read->pipeline.empty()) {
        mne::Log::warning("Empty or invalid MNA pipeline in {} {}", fullPath.toStdString(), (read ? "" : read.error().what()));
        return;
    }
    mne::MnaProject& project = *read;

    clearScene();

    // Phase 1: Create plugins from pipeline nodes
    std::map<std::string, PluginItem*, std::less<>> itemMap;
    for (const mne::MnaNode& node : project.pipeline) {
        QAction* pCurrentAction = nullptr;
        for (int i = 0; i < m_pActionGroupPlugins->actions().size(); ++i) {
            if (m_pActionGroupPlugins->actions()[i]->text().toStdString() == node.op_type) {
                pCurrentAction = m_pActionGroupPlugins->actions()[i];
                break;
            }
        }
        if (!pCurrentAction) {
            //
            // The configuration refers to a plugin this build does not provide.
            // Skipping it silently leaves the user with a pipeline that is
            // quietly missing a stage, so say which one was dropped.
            //
            mne::Log::warning("[PluginGui::loadConfigMna] Configuration refers to unknown plugin {} - skipping it.", QString::fromStdString(node.op_type).toStdString());
            continue;
        }

        const qreal posX = node.attributes["gui_x"].to_double();
        const qreal posY = node.attributes["gui_y"].to_double();
        QPointF pos(posX, posY);

        mne::scan::AbstractPlugin::SPtr pPlugin;
        pCurrentAction->setChecked(true);
        m_pPluginScene->setActionPluginItem(pCurrentAction);
        m_pPluginScene->setMode(PluginScene::InsertPluginItem);
        m_pPluginScene->insertItem(pos);

        // Find the just-inserted PluginItem
        //
        // insertItem() silently does nothing when the plugin cannot be created,
        // which is what happens for a configuration that names a plugin this
        // build no longer ships. The scene then still holds the items inserted
        // for earlier nodes, so this loop has to tolerate an item without a
        // plugin rather than dereferencing it.
        //
        for (QGraphicsItem* item : m_pPluginScene->items()) {
            if (item->type() == PluginItem::Type) {
                PluginItem* pi = qgraphicsitem_cast<PluginItem*>(item);
                if (!pi || !pi->plugin()) {
                    continue;
                }
                if (pi->plugin()->getName().toStdString() == node.op_type && !itemMap.contains(node.id)) {
                    itemMap.emplace(node.id, pi);
                    // Restore plugin-specific settings from node attributes
                    pi->plugin()->setAttributes(mne::scan::toPluginAttributes(node.attributes));
                    break;
                }
            }
        }
    }

    // Phase 2: Create connections from port links
    for (const mne::MnaNode& node : project.pipeline) {
        for (const mne::MnaPort& inPort : node.inputs) {
            const auto start = itemMap.find(inPort.source_node_id);
            const auto end = itemMap.find(node.id);
            if (inPort.source_node_id.empty() || start == itemMap.end() || end == itemMap.end())
                continue;
            PluginItem* startItem = start->second;
            PluginItem* endItem = end->second;

            mne::scan::PluginConnectorConnection::SPtr pConnection =
                mne::scan::PluginConnectorConnection::create(startItem->plugin(), endItem->plugin());

            if (pConnection->isConnected()) {
                Arrow* arrow = new Arrow(startItem, endItem, pConnection);
                arrow->setColor(QColor(148, 163, 184)); // slate-400
                startItem->addArrow(arrow);
                endItem->addArrow(arrow);
                arrow->setZValue(-1000.0);
                m_pPluginScene->addItem(arrow);
                arrow->updatePosition();

                m_pPluginSceneManager->connectGraphNodes(startItem->plugin(), endItem->plugin());
            }
        }
    }

    m_loadedMnaProject = std::move(project);
}

//=============================================================================================================

void PluginGui::saveConfig(const QString& sPath, const QString& sFileName)
{
    mne::Log::debug("Save Config {}", (sPath + "/" + sFileName).toStdString());

    QString fullPath = sPath + QStringLiteral("/") + sFileName;

    saveConfigMna(fullPath);
}

//=============================================================================================================

namespace
{
constexpr const char* kUiLayoutPrefix = "ui.layout.";
}

QByteArray PluginGui::uiLayoutBlob(const QString& sWindowKey) const
{
    const std::string fullKey = kUiLayoutPrefix + sWindowKey.toStdString();
    const std::string_view text = m_loadedMnaProject.extras[fullKey].to_string();
    return QByteArray::fromBase64(QByteArray(text.data(), static_cast<qsizetype>(text.size())));
}

//=============================================================================================================

void PluginGui::setUiLayoutBlob(const QString& sWindowKey, const QByteArray& blob)
{
    const std::string fullKey = kUiLayoutPrefix + sWindowKey.toStdString();
    if (blob.isEmpty()) {
        m_loadedMnaProject.extras.remove(fullKey);
        return;
    }
    m_loadedMnaProject.extras.insert(fullKey, blob.toBase64().toStdString());
}

//=============================================================================================================

void PluginGui::saveConfigMna(const QString& fullPath)
{
    // Update GUI positions in the pipeline graph
    for (QGraphicsItem* item : m_pPluginScene->items()) {
        if (item->type() == PluginItem::Type) {
            PluginItem* pItem = qgraphicsitem_cast<PluginItem*>(item);
            if (!pItem || !pItem->plugin()) {
                continue;
            }
            m_pPluginSceneManager->updateGraphNodePosition(pItem->plugin(), item->x(), item->y());
        }
    }

    // Refresh plugin-specific attributes (file paths, parameters)
    m_pPluginSceneManager->refreshGraphNodeAttributes();

    // ── Enriching mode: start from loaded project to preserve foreign data ──
    mne::MnaProject project;
    if (!m_loadedMnaProject.subjects.empty() || !m_loadedMnaProject.pipeline.empty()) {
        project = m_loadedMnaProject;
    } else {
        project.name = "MNE Scan Pipeline";
        project.description = "MNE Scan plugin configuration";
    }
    project.pipeline = m_pPluginSceneManager->pipelineGraph().nodes();
    project.modified = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());

    // Ensure directory exists
    QFileInfo fi(fullPath);
    QDir dir;
    if (!dir.exists(fi.path()))
        if (!dir.mkpath(fi.path()))
            return;

    if (const mne::Expected<void> written = project.write(mne::path_from_utf8(fullPath.toStdString())); !written) {
        mne::Log::warning("[PluginGui::saveConfigMna] {}", written.error().what());
    }
    m_loadedMnaProject = std::move(project);
}

//=============================================================================================================

void PluginGui::uiSetupRunningState(bool state)
{
    if (state) {
        m_pToolBarPlugins->setEnabled(false);
        m_pButtonGroupPointers->button(1)->setEnabled(false);
        deleteAction->setEnabled(false);
    } else {
        m_pToolBarPlugins->setEnabled(true);
        m_pButtonGroupPointers->button(1)->setEnabled(true);
        deleteAction->setEnabled(true);
    }
}

//=============================================================================================================

bool PluginGui::removePlugin(mne::scan::AbstractPlugin::SPtr pPlugin)
{
    bool bRemoved = m_pPluginSceneManager->removePlugin(pPlugin);

    if (bRemoved) {
        //If single instance activate menu again
        if (!pPlugin->multiInstanceAllowed()) {
            QString sPluginName = pPlugin->getName();

            foreach (QAction* action, m_pActionGroupPlugins->actions()) {
                if (action->text() == sPluginName) {
                    action->setEnabled(true);
                    break;
                }
            }
        }

        //Select the last added plugin
        if (m_pPluginSceneManager->getPlugins().size() > 0)
            m_pCurrentPlugin = m_pPluginSceneManager->getPlugins()[m_pPluginSceneManager->getPlugins().size() - 1];
        else
            m_pCurrentPlugin.reset();

        selectedPluginChanged(m_pCurrentPlugin);
    }

    saveConfig(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "default.mna");

    return bRemoved;
}

//=============================================================================================================

void PluginGui::actionGroupTriggered(QAction* action)
{
    m_pPluginScene->setActionPluginItem(action);
    m_pPluginScene->setMode(PluginScene::InsertPluginItem);
}

//=============================================================================================================

void PluginGui::itemInserted(PluginItem* item)
{
    if (item) {
        m_pCurrentPlugin = item->plugin();
        emit selectedPluginChanged(m_pCurrentPlugin);
    }

    //
    // Grow the scene to take in the new item before the view reacts to it,
    // otherwise the implicit rect changes and the scroll position jumps.
    //
    updateSceneRect();

    m_pButtonGroupPointers->button(int(PluginScene::MovePluginItem))->setChecked(true);
    m_pPluginScene->setMode(PluginScene::Mode(m_pButtonGroupPointers->checkedId()));

    saveConfig(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation), "default.mna");
}

//=============================================================================================================

void PluginGui::newItemSelected()
{
    mne::scan::AbstractPlugin::SPtr pPlugin;
    mne::scan::PluginConnectorConnection::SPtr pConnection;

    foreach (QGraphicsItem* item, m_pPluginScene->selectedItems()) {
        if (item->type() == PluginItem::Type)
            pPlugin = qgraphicsitem_cast<PluginItem*>(item)->plugin();
        else if (item->type() == Arrow::Type)
            pConnection = qgraphicsitem_cast<Arrow*>(item)->connection();
    }

    if (!pPlugin.isNull() && pPlugin != m_pCurrentPlugin) {
        m_pCurrentPlugin = pPlugin;
        m_pCurrentConnection = mne::scan::PluginConnectorConnection::SPtr();
        emit selectedPluginChanged(m_pCurrentPlugin);
    } else if (!pConnection.isNull() && pConnection != m_pCurrentConnection) {
        m_pCurrentConnection = pConnection;
        m_pCurrentPlugin = mne::scan::AbstractPlugin::SPtr();
        emit selectedConnectionChanged(m_pCurrentConnection);
    }
}

//=============================================================================================================

void PluginGui::deleteItem()
{
    //Remove Arrows ToDo Connections
    foreach (QGraphicsItem* item, m_pPluginScene->selectedItems()) {
        if (item->type() == Arrow::Type) {
            m_pPluginScene->removeItem(item);
            std::unique_ptr<QGraphicsItem> pRemoved(item);
            Arrow* arrow = qgraphicsitem_cast<Arrow*>(item);
            arrow->startItem()->removeArrow(arrow);
            arrow->endItem()->removeArrow(arrow);
        }
    }

    //Remove Items
    foreach (QGraphicsItem* item, m_pPluginScene->selectedItems()) {
        if (item->type() == PluginItem::Type) {
            if (removePlugin(qgraphicsitem_cast<PluginItem*>(item)->plugin())) {
                qgraphicsitem_cast<PluginItem*>(item)->removeArrows();

                m_pPluginScene->removeItem(item);
                std::unique_ptr<QGraphicsItem> pRemoved(item);
            }
        }
    }

    //The remaining items occupy less space, so let the scene follow them back in
    updateSceneRect();
}

//=============================================================================================================

void PluginGui::pointerGroupClicked()
{
    m_pPluginScene->setMode(PluginScene::Mode(m_pButtonGroupPointers->checkedId()));
}

//=============================================================================================================

void PluginGui::bringToFront()
{
    if (m_pPluginScene->selectedItems().isEmpty())
        return;

    QGraphicsItem* selectedItem = m_pPluginScene->selectedItems().first();
    QList<QGraphicsItem*> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem* item, overlapItems) {
        if (item->zValue() >= zValue && item->type() == PluginItem::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
}

//=============================================================================================================

void PluginGui::sendToBack()
{
    if (m_pPluginScene->selectedItems().isEmpty())
        return;

    QGraphicsItem* selectedItem = m_pPluginScene->selectedItems().first();
    QList<QGraphicsItem*> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem* item, overlapItems) {
        if (item->zValue() <= zValue && item->type() == PluginItem::Type)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
}

//=============================================================================================================

void PluginGui::createActions()
{
    deleteAction = new QAction(QIcon(":/images/delete.png"), tr("&Delete"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from diagram (Del)"));
    connect(deleteAction, &QAction::triggered, this, &PluginGui::deleteItem);

    toFrontAction = new QAction(QIcon(":/images/bringtofront.png"),
                                tr("Bring to &Front"), this);
    toFrontAction->setShortcut(tr("Ctrl+F"));
    toFrontAction->setStatusTip(tr("Bring item to front (Ctrl+F)"));
    connect(toFrontAction, &QAction::triggered, this, &PluginGui::bringToFront);

    sendBackAction = new QAction(QIcon(":/images/sendtoback.png"), tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+B"));
    sendBackAction->setStatusTip(tr("Send item to back (Ctrl+B)"));
    connect(sendBackAction, &QAction::triggered, this, &PluginGui::sendToBack);
}

//=============================================================================================================

void PluginGui::createMenuItem()
{
    m_pMenuItem = new QMenu;
    m_pMenuItem->addAction(deleteAction);
    m_pMenuItem->addSeparator();
    m_pMenuItem->addAction(toFrontAction);
    m_pMenuItem->addAction(sendBackAction);
}

//=============================================================================================================

void PluginGui::createToolbars()
{
    //Plugins Toolbar
    m_pActionGroupPlugins = new QActionGroup(this);
    m_pActionGroupPlugins->setExclusive(false);
    connect(m_pActionGroupPlugins, &QActionGroup::triggered,
            this, &PluginGui::actionGroupTriggered);

    //Sensors
    m_pSensorToolButton = new QToolButton;
    QMenu* menuSensors = new QMenu;
    for (qint32 i = 0; i < m_pPluginManager->getSensorPlugins().size(); ++i)
        createItemAction(m_pPluginManager->getSensorPlugins()[i]->getName(), menuSensors);

    m_pSensorToolButton->setMenu(menuSensors);
    m_pSensorToolButton->setPopupMode(QToolButton::InstantPopup);
    m_pSensorToolButton->setIcon(QIcon(":/images/sensor.png"));
    m_pSensorToolButton->setStatusTip(tr("Sensor Plugins"));
    m_pSensorToolButton->setToolTip(tr("Sensor Plugins"));

    //Algorithms
    m_pAlgorithmToolButton = new QToolButton;
    QMenu* menuAlgorithms = new QMenu;
    for (qint32 i = 0; i < m_pPluginManager->getAlgorithmPlugins().size(); ++i)
        createItemAction(m_pPluginManager->getAlgorithmPlugins()[i]->getName(), menuAlgorithms);

    m_pAlgorithmToolButton->setMenu(menuAlgorithms);
    m_pAlgorithmToolButton->setPopupMode(QToolButton::InstantPopup);
    m_pAlgorithmToolButton->setIcon(QIcon(":/images/algorithm.png"));
    m_pAlgorithmToolButton->setStatusTip(tr("Algorithm Plugins"));
    m_pAlgorithmToolButton->setToolTip(tr("Algorithm Plugins"));

    m_pToolBarPlugins = new QToolBar(tr("Plugins"), this);
    m_pToolBarPlugins->addWidget(m_pSensorToolButton);
    m_pToolBarPlugins->addWidget(m_pAlgorithmToolButton);

    m_pToolBarPlugins->setAllowedAreas(Qt::LeftToolBarArea);
    m_pToolBarPlugins->setFloatable(false);
    m_pToolBarPlugins->setMovable(false);

    addToolBar(Qt::LeftToolBarArea, m_pToolBarPlugins);

    //Pointers Toolbar
    m_pPointerButton = new QToolButton;
    m_pPointerButton->setCheckable(true);
    m_pPointerButton->setChecked(true);
    m_pPointerButton->setIcon(QIcon(":/images/pointer.png"));
    m_pPointerButton->setShortcut(tr("Ctrl+P"));
    m_pPointerButton->setStatusTip(tr("Select/Place (Ctrl+P)"));
    m_pPointerButton->setToolTip(tr("Select/Place"));

    m_pLinePointerButton = new QToolButton;
    m_pLinePointerButton->setCheckable(true);
    m_pLinePointerButton->setIcon(QIcon(":/images/linepointer.png"));
    m_pLinePointerButton->setShortcut(tr("Ctrl+L"));
    m_pLinePointerButton->setStatusTip(tr("Connection (Ctrl+L)"));
    m_pLinePointerButton->setToolTip(tr("Connection"));

    m_pButtonGroupPointers = new QButtonGroup(this);
    m_pButtonGroupPointers->addButton(m_pPointerButton, int(PluginScene::MovePluginItem));
    m_pButtonGroupPointers->addButton(m_pLinePointerButton, int(PluginScene::InsertLine));

    connect(m_pButtonGroupPointers, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &PluginGui::pointerGroupClicked);

    m_pToolBarPointer = new QToolBar(tr("Pointer type"), this);

    m_pToolBarPointer->addWidget(m_pPointerButton);
    m_pToolBarPointer->addWidget(m_pLinePointerButton);

    m_pToolBarPointer->setAllowedAreas(Qt::LeftToolBarArea);
    m_pToolBarPointer->setFloatable(false);
    m_pToolBarPointer->setMovable(false);

    addToolBar(Qt::LeftToolBarArea, m_pToolBarPointer);

    //Item
    m_pToolBarItem = new QToolBar(tr("Item"), this);

    m_pToolBarItem->addAction(deleteAction);
    m_pToolBarItem->addAction(toFrontAction);
    m_pToolBarItem->addAction(sendBackAction);

    m_pToolBarItem->setAllowedAreas(Qt::LeftToolBarArea);
    m_pToolBarItem->setFloatable(false);
    m_pToolBarItem->setMovable(false);

    addToolBar(Qt::LeftToolBarArea, m_pToolBarItem);
}

//=============================================================================================================

QAction* PluginGui::createItemAction(QString name, QMenu* menu)
{
    QAction* action = menu->addAction(name);
    m_pActionGroupPlugins->addAction(action);
    return action;
}
