//=============================================================================================================
/**
 * @file     pluginscene.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Gabriel B Motta. All rights reserved.
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
 * @brief     PluginScene class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginscene.h"
#include "pluginscenegui.h"
#include "arrow.h"

#include <QTextCursor>
#include <QGraphicsSceneMouseEvent>
#include <QAction>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginScene::PluginScene(QMenu *pMenuPluginItem, PluginSceneGui *pPluginGui)
: QGraphicsScene(pPluginGui)
, m_pPluginGui(pPluginGui)
, m_pActionPluginItem(Q_NULLPTR)
, leftButtonDown(false)
{
    m_pMenuPluginItem = pMenuPluginItem;
    m_mode = MovePluginItem;
//    m_itemType = PluginItem::Sensor;
    line = 0;
    m_qColorLine = QColor(65,113,156);
}

//=============================================================================================================

PluginScene::~PluginScene()
{
    this->clear();
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
