//=============================================================================================================
/**
 * @file     pluginitem.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh. All rights reserved.
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
 * @brief     PluginItem class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include "arrow.h"

#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginItem::PluginItem(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin, QMenu *contextMenu, QGraphicsItem *parent)
: QGraphicsPolygonItem(parent)
, m_pPlugin(pPlugin)
, m_iWidth(60)
, m_iHeight(40)
, m_contextMenu(contextMenu)
{
    QPainterPath path;
    m_qLinearGradientFace = QLinearGradient(m_iWidth/2, -10, m_iWidth/2, m_iHeight);
    m_qLinearGradientFace.setColorAt(1, Qt::white);
    m_qPolygon << QPointF(-m_iWidth/2, -m_iHeight/2) << QPointF(m_iWidth/2, -m_iHeight/2)
               << QPointF(m_iWidth/2, m_iHeight/2) << QPointF(-m_iWidth/2, m_iHeight/2)
               << QPointF(-m_iWidth/2, -m_iHeight/2);
    switch (m_pPlugin->getType()) {
//        case StartEnd:
//            m_qColorContour = QColor(79, 136, 187);
//            m_qLinearGradientFace.setColorAt(0, QColor(234, 239, 247));
//            break;
        case SCSHAREDLIB::AbstractPlugin::_IAlgorithm:
            m_qColorContour = QColor(98, 152, 61);
            m_qLinearGradientFace.setColorAt(0, QColor(235, 241, 233));
            break;
        case SCSHAREDLIB::AbstractPlugin::_ISensor:
            m_qColorContour = QColor(79, 136, 187);
            m_qLinearGradientFace.setColorAt(0, QColor(234, 239, 247));
            break;
        default:
            m_qColorContour = QColor(125, 125, 125);
            m_qLinearGradientFace.setColorAt(0, QColor(125, 125, 125));
            break;
    }
    setBrush(m_qLinearGradientFace);
    setPen(QPen(m_qColorContour,1));
    setPolygon(m_qPolygon);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

//=============================================================================================================

PluginItem::~PluginItem()
{
    m_pPlugin->unload();
}

//=============================================================================================================

void PluginItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    auto textSize = painter->fontMetrics().size(Qt::TextSingleLine, m_pPlugin->getName());
    int iHorizontalSpacing = 4;
    int iVerticalSpacing = 4;

    resizeAsRectangle(textSize.width() + 2 * iHorizontalSpacing,
                      textSize.height() + 2* iVerticalSpacing);

    QGraphicsPolygonItem::paint(painter, option, widget);

    painter->setPen(QPen(m_qColorContour, 1));

//    QString sKind("");
//    switch (m_diagramType) {
//        case StartEnd:
//            break;
//        case Algorithm:
//            sKind = QString("Tool");
//            break;
//        case Sensor:
//            sKind = QString("Sensor");
//            break;
//        default:
//            sKind = QString("IO");
//            break;
//    }

    painter->drawText(-m_iWidth/2+ iHorizontalSpacing, iVerticalSpacing,m_pPlugin->getName());
}

//=============================================================================================================

void PluginItem::removeArrow(Arrow *arrow)
{
    int index = arrows.indexOf(arrow);

    if (index != -1)
        arrows.removeAt(index);
}

//=============================================================================================================

void PluginItem::removeArrows()
{
    foreach (Arrow *arrow, arrows) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }
}

//=============================================================================================================

void PluginItem::addArrow(Arrow *arrow)
{
    arrows.append(arrow);
}

//=============================================================================================================

QPixmap PluginItem::image() const
{
    QPixmap pixmap(m_iWidth, m_iHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(m_qColorContour, 2));
    painter.translate(m_iWidth/2, m_iHeight/2);

    painter.drawPolyline(m_qPolygon);

    painter.drawText(-m_iWidth/2+4,-m_iHeight/2+14,m_pPlugin->getName().mid(0,10));

    return pixmap;
}

//=============================================================================================================

void PluginItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    m_contextMenu->exec(event->screenPos());
}

//=============================================================================================================

QVariant PluginItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (Arrow *arrow, arrows) {
            arrow->updatePosition();
        }
    }

    return value;

//    if (change == ItemPositionChange && scene()) {
//        // value is the new position.
//        QPointF newPos = value.toPointF();
//        QRectF rect = scene()->sceneRect();
//        if (!rect.contains(newPos)) {
//            // Keep the item inside the scene rect.
//            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
//            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
//            return newPos;
//        }
//    }

//    return QGraphicsItem::itemChange(change, value);
}

//=============================================================================================================

void PluginItem::resizeAsRectangle(int width, int height)
{
    if(m_iWidth == width && m_iHeight == height){
        return;
    }

    m_iWidth = width;
    m_iHeight = height;

    m_qPolygon = QPolygonF();
    m_qPolygon << QPointF(-m_iWidth/2, -m_iHeight/2) << QPointF(m_iWidth/2, -m_iHeight/2)
               << QPointF(m_iWidth/2, m_iHeight/2) << QPointF(-m_iWidth/2, m_iHeight/2)
               << QPointF(-m_iWidth/2, -m_iHeight/2);

    this->setPolygon(m_qPolygon);
}
