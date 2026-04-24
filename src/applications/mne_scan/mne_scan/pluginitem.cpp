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

#include <QGraphicsDropShadowEffect>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>

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
, m_iWidth(MIN_WIDTH)
, m_iHeight(MIN_HEIGHT)
, m_contextMenu(contextMenu)
{
    // Polygon for shape/collision detection
    m_qPolygon << QPointF(-m_iWidth/2, -m_iHeight/2) << QPointF(m_iWidth/2, -m_iHeight/2)
               << QPointF(m_iWidth/2, m_iHeight/2) << QPointF(-m_iWidth/2, m_iHeight/2)
               << QPointF(-m_iWidth/2, -m_iHeight/2);

    // Modern color palette
    switch (m_pPlugin->getType()) {
        case SCSHAREDLIB::AbstractPlugin::_IAlgorithm:
            m_qColorContour = QColor(34, 197, 94);     // green-500
            m_qColorFillTop = QColor(240, 253, 244);   // green-50
            break;
        case SCSHAREDLIB::AbstractPlugin::_ISensor:
            m_qColorContour = QColor(59, 130, 246);    // blue-500
            m_qColorFillTop = QColor(239, 246, 255);   // blue-50
            break;
        default:
            m_qColorContour = QColor(100, 116, 139);   // slate-500
            m_qColorFillTop = QColor(248, 250, 252);   // slate-50
            break;
    }

    // Fonts
    m_nameFont.setPixelSize(13);
    m_nameFont.setWeight(QFont::DemiBold);
    m_typeFont.setPixelSize(9);
    m_typeFont.setCapitalization(QFont::AllUppercase);
    m_typeFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);

    // Transparent pen/brush — all visual rendering happens in paint()
    setPen(QPen(Qt::transparent, 0));
    setBrush(Qt::NoBrush);
    setPolygon(m_qPolygon);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // Drop shadow
    auto *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 35));
    setGraphicsEffect(shadow);
}

//=============================================================================================================

PluginItem::~PluginItem()
{
    m_pPlugin->unload();
}

//=============================================================================================================

void PluginItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    // Measure name text and resize
    painter->setFont(m_nameFont);
    auto textSize = painter->fontMetrics().size(Qt::TextSingleLine, m_pPlugin->getName());
    int requiredWidth = textSize.width() + 2 * HORIZONTAL_PADDING;
    int requiredHeight = textSize.height() + 2 * VERTICAL_PADDING + TYPE_LABEL_HEIGHT;
    resizeAsRectangle(requiredWidth, requiredHeight);

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect(-m_iWidth / 2.0, -m_iHeight / 2.0, m_iWidth, m_iHeight);

    // Rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(rect, CORNER_RADIUS, CORNER_RADIUS);

    // Fill with gradient
    QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
    grad.setColorAt(0, m_qColorFillTop);
    grad.setColorAt(1, Qt::white);
    painter->fillPath(path, QBrush(grad));

    // Selection glow
    if (isSelected()) {
        QPen glowPen(QColor(m_qColorContour.red(), m_qColorContour.green(),
                            m_qColorContour.blue(), 70), 6);
        glowPen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(glowPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
    }

    // Border
    QPen borderPen(m_qColorContour, isSelected() ? 2.5 : 1.5);
    borderPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path);

    // Type label
    QString typeLabel;
    switch (m_pPlugin->getType()) {
        case SCSHAREDLIB::AbstractPlugin::_ISensor:
            typeLabel = QStringLiteral("SENSOR");
            break;
        case SCSHAREDLIB::AbstractPlugin::_IAlgorithm:
            typeLabel = QStringLiteral("ALGORITHM");
            break;
        default:
            typeLabel = QStringLiteral("PLUGIN");
            break;
    }
    painter->setFont(m_typeFont);
    painter->setPen(QPen(m_qColorContour, 1));
    QRectF typeLabelRect(rect.left(), rect.top() + 4, rect.width(), TYPE_LABEL_HEIGHT);
    painter->drawText(typeLabelRect, Qt::AlignCenter, typeLabel);

    // Plugin name (centered in remaining space)
    painter->setFont(m_nameFont);
    painter->setPen(QPen(QColor(30, 41, 59), 1)); // slate-800
    QRectF nameRect(rect.left(), rect.top() + TYPE_LABEL_HEIGHT,
                    rect.width(), rect.height() - TYPE_LABEL_HEIGHT);
    painter->drawText(nameRect, Qt::AlignCenter, m_pPlugin->getName());

    // Input port (left center) — hollow circle
    painter->setPen(QPen(m_qColorContour, 1.5));
    painter->setBrush(Qt::white);
    painter->drawEllipse(QPointF(-m_iWidth / 2.0, 0), PORT_RADIUS, PORT_RADIUS);

    // Output port (right center) — filled circle
    painter->setBrush(m_qColorContour);
    painter->drawEllipse(QPointF(m_iWidth / 2.0, 0), PORT_RADIUS, PORT_RADIUS);
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
    const int w = 100;
    const int h = 48;
    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect(1, 1, w - 2, h - 2);
    QPainterPath path;
    path.addRoundedRect(rect, CORNER_RADIUS, CORNER_RADIUS);

    QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
    grad.setColorAt(0, m_qColorFillTop);
    grad.setColorAt(1, Qt::white);
    painter.fillPath(path, QBrush(grad));

    painter.setPen(QPen(m_qColorContour, 1.5));
    painter.drawPath(path);

    painter.setFont(m_nameFont);
    painter.setPen(QPen(QColor(30, 41, 59), 1));
    painter.drawText(rect, Qt::AlignCenter, m_pPlugin->getName().mid(0, 12));

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
    width = qMax(width, MIN_WIDTH);
    height = qMax(height, MIN_HEIGHT);

    if(m_iWidth == width && m_iHeight == height){
        return;
    }

    m_iWidth = width;
    m_iHeight = height;

    m_qPolygon = QPolygonF({QPointF(-m_iWidth/2, -m_iHeight/2),
                            QPointF(m_iWidth/2, -m_iHeight/2),
                            QPointF(m_iWidth/2, m_iHeight/2),
                            QPointF(-m_iWidth/2, m_iHeight/2),
                            QPointF(-m_iWidth/2, -m_iHeight/2)});

    this->setPolygon(m_qPolygon);
}

//=============================================================================================================

QRectF PluginItem::boundingRect() const
{
    QRectF base = QGraphicsPolygonItem::boundingRect();
    return base.adjusted(-PORT_RADIUS - 1, -PORT_RADIUS - 1,
                         PORT_RADIUS + 1, PORT_RADIUS + 1);
}

//=============================================================================================================

QPointF PluginItem::outputPortLocalPos() const
{
    return QPointF(m_iWidth / 2.0, 0);
}

//=============================================================================================================

QPointF PluginItem::inputPortLocalPos() const
{
    return QPointF(-m_iWidth / 2.0, 0);
}
