//=============================================================================================================
/**
 * @file     plugingui.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     PluginGui class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginscenegui.h"

#include "arrow.h"
#include "pluginitem.h"
#include "pluginscene.h"

#include <scShared/Plugins/abstractplugin.h>
#include <scShared/Plugins/abstractsensor.h>
#include <scShared/Plugins/abstractalgorithm.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QDomDocument>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginSceneGui::PluginSceneGui(SCSHAREDLIB::PluginManager *pPluginManager,
                     SCSHAREDLIB::PluginSceneManager *pPluginSceneManager)
: m_pPluginManager(pPluginManager)
, m_pPluginSceneManager(pPluginSceneManager)
, m_pCurrentPlugin(0)
, m_pGraphicsView(Q_NULLPTR)
, m_pSensorToolButton(Q_NULLPTR)
, m_pAlgorithmToolButton(Q_NULLPTR)
, m_pToolBarPlugins(Q_NULLPTR)
, m_pPointerButton(Q_NULLPTR)
, m_pLinePointerButton(Q_NULLPTR)
, m_pToolBarPointer(Q_NULLPTR)
, m_pToolBarItem(Q_NULLPTR)
{
    createActions();
    createMenuItem();
    createToolbars();

    m_pPluginScene = new PluginScene(m_pMenuItem, this);
    //m_pPluginScene->setSceneRect(0, 0, 200, 500);

    connect(m_pPluginScene, &PluginScene::itemInserted,
            this, &PluginSceneGui::itemInserted);

    connect(m_pPluginScene, &PluginScene::selectionChanged,
            this, &PluginSceneGui::newItemSelected);

    m_pGraphicsView = new QGraphicsView(m_pPluginScene);
    setCentralWidget(m_pGraphicsView);

    setWindowTitle(tr("PluginScene"));
    setUnifiedTitleAndToolBarOnMac(true);

    //To prevent deadlock on loading with a broken plugin -> save loading state
    QSettings settings("MNECPP");
    bool loadingState = settings.value(QString("MNEScan/loadingState"), false).toBool();

    if(loadingState)
    {
        settings.setValue(QString("MNEScan/loadingState"), false);
        loadConfig(QStandardPaths::writableLocation(QStandardPaths::DataLocation), "default.xml");
    }

    settings.setValue(QString("MNEScan/loadingState"), true);

    m_pGraphicsView->setMinimumWidth(200);
    m_pGraphicsView->ensureVisible(m_pPluginScene->itemsBoundingRect());
}

//=============================================================================================================

PluginSceneGui::~PluginSceneGui()
{
    //
    // Save current configuration
    //
    saveConfig(QStandardPaths::writableLocation(QStandardPaths::DataLocation),"default.xml");

    m_pCurrentPlugin.reset();

    //Plugin Toolbar
    if(m_pSensorToolButton)
        delete m_pSensorToolButton;
    if(m_pAlgorithmToolButton)
        delete m_pAlgorithmToolButton;
    if(m_pToolBarPlugins)
        delete m_pToolBarPlugins;
    //Pointers Toolbar
    if(m_pPointerButton)
        delete m_pPointerButton;
    if(m_pLinePointerButton)
        delete m_pLinePointerButton;
    if(m_pToolBarPointer)
        delete m_pToolBarPointer;
    //Item
    if(m_pToolBarItem)
        delete m_pToolBarItem;

    if(m_pGraphicsView)
        delete m_pGraphicsView;
}

//=============================================================================================================

void PluginSceneGui::clearScene()
{
    foreach (QGraphicsItem *item, m_pPluginScene->items())
    {
        if(item->type() == PluginItem::Type)
        {
            if(removePlugin(qgraphicsitem_cast<PluginItem *>(item)->plugin()))
            {
                qgraphicsitem_cast<PluginItem *>(item)->removeArrows();
                m_pPluginScene->removeItem(item);
                delete item;
            }
        }
    }
}

//=============================================================================================================

void PluginSceneGui::loadConfig(const QString& sPath, const QString& sFileName)
{
    qDebug() << "load" << sPath+"/"+sFileName;

    QDomDocument doc("PluginConfig");
    QFile file(sPath+"/"+sFileName);
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    clearScene();

    QDomElement docElem = doc.documentElement();
    if(docElem.tagName() != "PluginTree")
    {
        qWarning() << sFileName << "not found!";
        return;
    }

    QDomNode nodePluginTree = docElem.firstChild();
    while(!nodePluginTree.isNull()) {
        QDomElement elementPluginTree = nodePluginTree.toElement();
        //
        // Create Plugins
        //
        if(elementPluginTree.tagName() == "Plugins") {
            QDomNode nodePlugins = elementPluginTree.firstChild();
            while(!nodePlugins.isNull())
            {
                QDomElement e = nodePlugins.toElement();
                nodePlugins = nodePlugins.nextSibling();
                if(!e.isNull()) {
                    QPointF pos((qreal)e.attribute("pos_x").toInt(),(qreal)e.attribute("pos_y").toInt());
                    QAction *pCurrentAction = Q_NULLPTR;

                    for(qint32 i = 0; i < m_pActionGroupPlugins->actions().size(); ++i)
                    {
                        if(m_pActionGroupPlugins->actions()[i]->text() == e.attribute("name"))
                        {
                            pCurrentAction = m_pActionGroupPlugins->actions()[i];
                            break;
                        }
                    }

                    if(!pCurrentAction)
                        continue;

                    pCurrentAction->setChecked(true);
                    m_pPluginScene->setActionPluginItem(pCurrentAction);
                    m_pPluginScene->setMode(PluginScene::InsertPluginItem);
                    m_pPluginScene->insertItem(pos);
                }
            }
        }
        //
        // Create Connections
        //
        if(elementPluginTree.tagName() == "Connections") {
            QDomNode nodeConections = elementPluginTree.firstChild();
            while(!nodeConections.isNull())
            {
                QDomElement e = nodeConections.toElement();
                nodeConections = nodeConections.nextSibling();
                if(!e.isNull()) {
                    qDebug() << qPrintable(e.tagName()) << e.attribute("receiver") << e.attribute("sender");

                    QString sSender = e.attribute("sender");
                    QString sReceiver = e.attribute("receiver");

                    //
                    // Start & End
                    //
                    PluginItem* startItem = Q_NULLPTR;
                    PluginItem* endItem = Q_NULLPTR;
                    for(qint32 i = 0; i < m_pPluginScene->items().size(); ++i)
                    {
                        if(PluginItem* item = qgraphicsitem_cast<PluginItem *>(m_pPluginScene->items()[i])) {
                            if(item->plugin()->getName() == sSender)
                                startItem = item;

                            if(item->plugin()->getName() == sReceiver)
                                endItem = item;
                        }
                    }

                    if(!startItem || !endItem)
                        continue;

                    SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection = SCSHAREDLIB::PluginConnectorConnection::create(startItem->plugin(), endItem->plugin());

                    if(pConnection->isConnected())
                    {
                        Arrow *arrow = new Arrow(startItem, endItem, pConnection);
                        arrow->setColor(QColor(65,113,156));
                        startItem->addArrow(arrow);
                        endItem->addArrow(arrow);
                        arrow->setZValue(-1000.0);
                        m_pPluginScene->addItem(arrow);
                        arrow->updatePosition();
                    }

                }
            }
        }
        nodePluginTree = nodePluginTree.nextSibling();
    }
}

//=============================================================================================================

void PluginSceneGui::saveConfig(const QString& sPath, const QString& sFileName)
{
    qDebug() << "Save Config" << sPath+"/"+sFileName;
    QDomDocument doc("PluginConfig");
    QDomElement root = doc.createElement("PluginTree");
    doc.appendChild(root);

    //
    // Plugins
    //
    QDomElement plugins = doc.createElement("Plugins");
    root.appendChild(plugins);
    SCSHAREDLIB::AbstractPlugin::SPtr pPlugin;
    foreach (QGraphicsItem *item, m_pPluginScene->items())
    {
        if(item->type() == PluginItem::Type)
        {
            pPlugin = qgraphicsitem_cast<PluginItem *>(item)->plugin();

            QDomElement plugin = doc.createElement("Plugin");
            plugin.setAttribute("name",pPlugin->getName());
            plugin.setAttribute("pos_x",item->x());
            plugin.setAttribute("pos_y",item->y());
            plugins.appendChild(plugin);
        }
    }

    //
    // Connections
    //
    QDomElement connections = doc.createElement("Connections");
    root.appendChild(connections);
    SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection;
    foreach (QGraphicsItem *item, m_pPluginScene->items())
    {
        if(item->type() == Arrow::Type)
        {
            pConnection = qgraphicsitem_cast<Arrow *>(item)->connection();

            QDomElement connection = doc.createElement("Connection");
            connection.setAttribute("sender",pConnection->getSender()->getName());
            connection.setAttribute("receiver",pConnection->getReceiver()->getName());
            connections.appendChild(connection);
        }
    }

    QString xml = doc.toString();

    QDir dir;
    if(!dir.exists(sPath))
        if(!dir.mkpath(sPath))
            return;

    QFile file(sPath+"/"+sFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << xml;
}

//=============================================================================================================

void PluginSceneGui::uiSetupRunningState(bool state)
{
    if(state)
    {
        m_pToolBarPlugins->setEnabled(false);
        m_pButtonGroupPointers->button(1)->setEnabled(false);
        deleteAction->setEnabled(false);
    }
    else
    {
        m_pToolBarPlugins->setEnabled(true);
        m_pButtonGroupPointers->button(1)->setEnabled(true);
        deleteAction->setEnabled(true);
    }
}

//=============================================================================================================

bool PluginSceneGui::removePlugin(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin)
{
    bool bRemoved = m_pPluginSceneManager->removePlugin(pPlugin);

    if(bRemoved)
    {
        //If single instance activate menu again
        if(!pPlugin->multiInstanceAllowed())
        {
            QString sPluginName = pPlugin->getName();

            foreach (QAction *action, m_pActionGroupPlugins->actions())
            {
                if(action->text() == sPluginName)
                {
                    action->setEnabled(true);
                    break;
                }
            }
        }

        //Select the last added plugin
        if(m_pPluginSceneManager->getPlugins().size() > 0)
            m_pCurrentPlugin = m_pPluginSceneManager->getPlugins()[m_pPluginSceneManager->getPlugins().size()-1];
        else
            m_pCurrentPlugin.reset();

        selectedPluginChanged(m_pCurrentPlugin);
    }

    saveConfig(QStandardPaths::writableLocation(QStandardPaths::DataLocation),"default.xml");

    return bRemoved;
}

//=============================================================================================================

void PluginSceneGui::actionGroupTriggered(QAction* action)
{
    m_pPluginScene->setActionPluginItem(action);
    m_pPluginScene->setMode(PluginScene::InsertPluginItem);
}

//=============================================================================================================

void PluginSceneGui::itemInserted(PluginItem *item)
{
    if(item) {
        m_pCurrentPlugin = item->plugin();
        emit selectedPluginChanged(m_pCurrentPlugin);
    }

    m_pButtonGroupPointers->button(int(PluginScene::MovePluginItem))->setChecked(true);
    m_pPluginScene->setMode(PluginScene::Mode(m_pButtonGroupPointers->checkedId()));

    saveConfig(QStandardPaths::writableLocation(QStandardPaths::DataLocation),"default.xml");
}

//=============================================================================================================

void PluginSceneGui::newItemSelected()
{
    SCSHAREDLIB::AbstractPlugin::SPtr pPlugin;
    SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection;

    foreach (QGraphicsItem *item, m_pPluginScene->selectedItems())
    {
        if(item->type() == PluginItem::Type)
            pPlugin = qgraphicsitem_cast<PluginItem *>(item)->plugin();
        else if(item->type() == Arrow::Type)
            pConnection = qgraphicsitem_cast<Arrow *>(item)->connection();

    }

    if(!pPlugin.isNull() && pPlugin != m_pCurrentPlugin)
    {
        m_pCurrentPlugin = pPlugin;
        m_pCurrentConnection = SCSHAREDLIB::PluginConnectorConnection::SPtr();
        emit selectedPluginChanged(m_pCurrentPlugin);
    }
    else if(!pConnection.isNull() && pConnection != m_pCurrentConnection)
    {
        m_pCurrentConnection = pConnection;
        m_pCurrentPlugin = SCSHAREDLIB::AbstractPlugin::SPtr();
        emit selectedConnectionChanged(m_pCurrentConnection);
    }
}

//=============================================================================================================

void PluginSceneGui::deleteItem()
{
    //Remove Arrows ToDo Connections
    foreach (QGraphicsItem *item, m_pPluginScene->selectedItems())
    {
        if (item->type() == Arrow::Type)
        {
            m_pPluginScene->removeItem(item);
            Arrow *arrow = qgraphicsitem_cast<Arrow *>(item);
            arrow->startItem()->removeArrow(arrow);
            arrow->endItem()->removeArrow(arrow);
            delete item;
        }
    }

    //Remove Items
    foreach (QGraphicsItem *item, m_pPluginScene->selectedItems())
    {
         if (item->type() == PluginItem::Type)
         {
             if(removePlugin(qgraphicsitem_cast<PluginItem *>(item)->plugin()))
             {
                 qgraphicsitem_cast<PluginItem *>(item)->removeArrows();

                 m_pPluginScene->removeItem(item);
                 delete item;
             }
         }
     }
}

//=============================================================================================================

void PluginSceneGui::pointerGroupClicked(int)
{
    m_pPluginScene->setMode(PluginScene::Mode(m_pButtonGroupPointers->checkedId()));
}

//=============================================================================================================

void PluginSceneGui::bringToFront()
{
    if (m_pPluginScene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = m_pPluginScene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() >= zValue && item->type() == PluginItem::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);
}

//=============================================================================================================

void PluginSceneGui::sendToBack()
{
    if (m_pPluginScene->selectedItems().isEmpty())
        return;

    QGraphicsItem *selectedItem = m_pPluginScene->selectedItems().first();
    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();

    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() <= zValue && item->type() == PluginItem::Type)
            zValue = item->zValue() - 0.1;
    }
    selectedItem->setZValue(zValue);
}

//=============================================================================================================

void PluginSceneGui::createActions()
{
    deleteAction = new QAction(QIcon(":/images/delete.png"), tr("&Delete"), this);
    deleteAction->setShortcut(tr("Delete"));
    deleteAction->setStatusTip(tr("Delete item from diagram (Del)"));
    connect(deleteAction, &QAction::triggered, this, &PluginSceneGui::deleteItem);

    toFrontAction = new QAction(QIcon(":/images/bringtofront.png"),
                                tr("Bring to &Front"), this);
    toFrontAction->setShortcut(tr("Ctrl+F"));
    toFrontAction->setStatusTip(tr("Bring item to front (Ctrl+F)"));
    connect(toFrontAction, &QAction::triggered, this, &PluginSceneGui::bringToFront);

    sendBackAction = new QAction(QIcon(":/images/sendtoback.png"), tr("Send to &Back"), this);
    sendBackAction->setShortcut(tr("Ctrl+B"));
    sendBackAction->setStatusTip(tr("Send item to back (Ctrl+B)"));
    connect(sendBackAction, &QAction::triggered, this, &PluginSceneGui::sendToBack);
}

//=============================================================================================================

void PluginSceneGui::createMenuItem()
{
    m_pMenuItem = new QMenu;
    m_pMenuItem->addAction(deleteAction);
    m_pMenuItem->addSeparator();
    m_pMenuItem->addAction(toFrontAction);
    m_pMenuItem->addAction(sendBackAction);
}

//=============================================================================================================

void PluginSceneGui::createToolbars()
{
    //Plugins Toolbar
    m_pActionGroupPlugins = new QActionGroup(this);
    m_pActionGroupPlugins->setExclusive(false);
    connect(m_pActionGroupPlugins, &QActionGroup::triggered,
            this, &PluginSceneGui::actionGroupTriggered);

    //Sensors
    m_pSensorToolButton = new QToolButton;
    QMenu *menuSensors = new QMenu;
    for(auto& plugin : m_pPluginManager->getSensorPlugins()){
        if(plugin->isScenePlugin()){
            createItemAction(plugin->getName(), menuSensors);
        }
    }

    m_pSensorToolButton->setMenu(menuSensors);
    m_pSensorToolButton->setPopupMode(QToolButton::InstantPopup);
    m_pSensorToolButton->setIcon(QIcon(":/images/sensor.png"));
    m_pSensorToolButton->setStatusTip(tr("Sensor Plugins"));
    m_pSensorToolButton->setToolTip(tr("Sensor Plugins"));

    //Algorithms
    m_pAlgorithmToolButton = new QToolButton;
    QMenu *menuAlgorithms = new QMenu;
    for(auto& plugin : m_pPluginManager->getAlgorithmPlugins()){
        if(plugin->isScenePlugin()){
            createItemAction(plugin->getName(), menuAlgorithms);
        }
    }

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

    connect(m_pButtonGroupPointers, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &PluginSceneGui::pointerGroupClicked);

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

QAction* PluginSceneGui::createItemAction(QString name, QMenu* menu)
{
    QAction* action = menu->addAction(name);
    m_pActionGroupPlugins->addAction(action);
    return action;
}
