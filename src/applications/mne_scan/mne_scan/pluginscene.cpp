//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     pluginscene.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August, 2013
 * @brief     PluginScene class implementation
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginscene.h"
#include "plugingui.h"
#include "arrow.h"

#include <QPainter>
#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QAction>
#include <QtMath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginScene::PluginScene(QMenu *pMenuPluginItem, PluginGui *pPluginGui)
: QGraphicsScene(pPluginGui)
, m_pPluginGui(pPluginGui)
, m_pActionPluginItem(Q_NULLPTR)
, leftButtonDown(false)
{
    m_pMenuPluginItem = pMenuPluginItem;
    m_mode = MovePluginItem;
//    m_itemType = PluginItem::Sensor;
    line = 0;
    m_qColorLine = QColor(148, 163, 184); // slate-400
}

//=============================================================================================================

PluginScene::~PluginScene()
{
    this->clear();
}

//=============================================================================================================

void PluginScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Solid background
    painter->fillRect(rect, QColor(249, 250, 251)); // gray-50

    // Dot grid
    const qreal gridSize = 20.0;
    painter->setPen(QPen(QColor(209, 213, 219), 1.5)); // gray-300

    qreal left = qFloor(rect.left() / gridSize) * gridSize;
    qreal top  = qFloor(rect.top()  / gridSize) * gridSize;

    QVector<QPointF> points;
    for (qreal x = left; x <= rect.right(); x += gridSize)
        for (qreal y = top; y <= rect.bottom(); y += gridSize)
            points.append(QPointF(x, y));

    painter->drawPoints(points.data(), points.size());
}

//=============================================================================================================

void PluginScene::insertItem(const QPointF& pos)
{
    PluginItem *item;
    SCSHAREDLIB::AbstractPlugin::SPtr pPlugin;
    QString name;
    switch (m_mode) {
        case InsertPluginItem:
            if(insertPlugin(m_pActionPluginItem, pPlugin))
            {
                name = m_pActionPluginItem->text();
                item = new PluginItem(pPlugin, m_pMenuPluginItem);
                addItem(item);
                item->setPos(pos);
                emit itemInserted(item);
            }
            else
            {
                //If insertion failed, disable insert action
                m_pActionPluginItem->setEnabled(false);
            }
            break;
        case InsertLine:
            line = new QGraphicsLineItem(QLineF(pos,pos));
            line->setPen(QPen(m_qColorLine, 1));
            addItem(line);
            break;
        default:
        ;
    }
}

//=============================================================================================================

bool PluginScene::insertPlugin(QAction* pActionPluginItem, SCSHAREDLIB::AbstractPlugin::SPtr &pAddedPlugin)
{
    if(pActionPluginItem->isEnabled())
    {
        QString name = pActionPluginItem->text();
        qint32 idx = m_pPluginGui->m_pPluginManager->findByName(name);
        if(idx < 0) {
            qDebug() << "Unable to find index";
            return false;
        }
        SCSHAREDLIB::AbstractPlugin* pPlugin = m_pPluginGui->m_pPluginManager->getPlugins()[idx];

        if(m_pPluginGui->m_pPluginSceneManager->addPlugin(pPlugin, pAddedPlugin))
        {
            //If only single instance -> disable insert action
            if(!pPlugin->multiInstanceAllowed())
                pActionPluginItem->setEnabled(false);
            return true;
        }
    }
    return false;

//    return true;//DEBUG
}

//=============================================================================================================

void PluginScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() != Qt::LeftButton)
        return;

    insertItem(mouseEvent->scenePos());

    QGraphicsScene::mousePressEvent(mouseEvent);
}

//=============================================================================================================

void PluginScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (m_mode == InsertLine && line != 0) {
        QLineF newLine(line->line().p1(), mouseEvent->scenePos());
        line->setLine(newLine);
    } else if (m_mode == MovePluginItem) {
        QGraphicsScene::mouseMoveEvent(mouseEvent);
    }
}

//=============================================================================================================

void PluginScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (line != 0 && m_mode == InsertLine) {
        QList<QGraphicsItem *> startItems = items(line->line().p1());
        if (startItems.count() && startItems.first() == line)
            startItems.removeFirst();
        QList<QGraphicsItem *> endItems = items(line->line().p2());
        if (endItems.count() && endItems.first() == line)
            endItems.removeFirst();

        removeItem(line);
        delete line;

        //Insert Connection
        if (startItems.count() > 0 && endItems.count() > 0 &&
            startItems.first()->type() == PluginItem::Type &&
            endItems.first()->type() == PluginItem::Type &&
            startItems.first() != endItems.first()) {

            PluginItem *startItem = qgraphicsitem_cast<PluginItem *>(startItems.first());
            PluginItem *endItem = qgraphicsitem_cast<PluginItem *>(endItems.first());

            SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection = SCSHAREDLIB::PluginConnectorConnection::create(startItem->plugin(), endItem->plugin());

            if(pConnection->isConnected())
            {
                Arrow *arrow = new Arrow(startItem, endItem, pConnection);
                arrow->setColor(m_qColorLine);
                startItem->addArrow(arrow);
                endItem->addArrow(arrow);
                arrow->setZValue(-1000.0);
                addItem(arrow);
                arrow->updatePosition();

                // Record connection in MNA pipeline graph
                m_pPluginGui->m_pPluginSceneManager->connectGraphNodes(
                    startItem->plugin(), endItem->plugin());
            }

        }
    }
    line = 0;
    QGraphicsScene::mouseReleaseEvent(mouseEvent);
}

////=============================================================================================================

//bool PluginScene::isItemChange(int type)
//{
//    foreach (QGraphicsItem *item, selectedItems()) {
//        if (item->type() == type)
//            return true;
//    }
//    return false;
//}
