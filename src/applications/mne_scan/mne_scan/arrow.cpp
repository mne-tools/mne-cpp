//=============================================================================================================
/**
 * @file     arrow.cpp
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
 * @brief     Arrow class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "arrow.h"

#include <QtMath>

#include <QPen>
#include <QPainter>
#include <QPainterPath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Arrow::Arrow(PluginItem *startItem, PluginItem *endItem, SCSHAREDLIB::PluginConnectorConnection::SPtr &connection, QGraphicsItem *parent)
: QGraphicsLineItem(parent)
, m_StartItem(startItem)
, m_EndItem(endItem)
, m_pConnection(connection)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_qColor = QColor(148, 163, 184); // slate-400
    setPen(QPen(m_qColor, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

//=============================================================================================================

QRectF Arrow::boundingRect() const
{
    if (!m_bezierPath.isEmpty())
        return m_bezierPath.boundingRect().adjusted(-15, -15, 15, 15);

    qreal extra = (pen().width() + 20) / 2.0;

    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}

//=============================================================================================================

QPainterPath Arrow::shape() const
{
    if (!m_bezierPath.isEmpty()) {
        QPainterPathStroker stroker;
        stroker.setWidth(15);
        QPainterPath result = stroker.createStroke(m_bezierPath);
        result.addPolygon(arrowHead);
        return result;
    }
    QPainterPath path = QGraphicsLineItem::shape();
    path.addPolygon(arrowHead);
    return path;
}

//=============================================================================================================

void Arrow::updatePosition()
{
    QPointF startPt = mapFromItem(m_StartItem, m_StartItem->outputPortLocalPos());
    QPointF endPt   = mapFromItem(m_EndItem,   m_EndItem->inputPortLocalPos());

    setLine(QLineF(startPt, endPt));

    // Build cubic bezier path
    qreal dx = qAbs(endPt.x() - startPt.x());
    qreal offset = qMax(50.0, dx * 0.4);
    QPointF cp1(startPt.x() + offset, startPt.y());
    QPointF cp2(endPt.x()   - offset, endPt.y());

    m_bezierPath = QPainterPath();
    m_bezierPath.moveTo(startPt);
    m_bezierPath.cubicTo(cp1, cp2, endPt);
}

//=============================================================================================================

void Arrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
          QWidget *)
{
    if (m_StartItem->collidesWithItem(m_EndItem) || m_bezierPath.isEmpty())
        return;

    painter->setRenderHint(QPainter::Antialiasing);

    QPointF endPt = m_bezierPath.pointAtPercent(1.0);

    // Draw bezier curve
    QColor lineColor = isSelected() ? QColor(59, 130, 246) : m_qColor;
    QPen curvePen(lineColor, isSelected() ? 2.5 : 1.5);
    curvePen.setCapStyle(Qt::RoundCap);
    curvePen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(curvePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_bezierPath);

    // Arrowhead at end point — compute tangent angle near curve end
    constexpr qreal arrowSize = 9.0;
    QPointF nearEnd = m_bezierPath.pointAtPercent(0.97);
    qreal angle = qAtan2(-(endPt.y() - nearEnd.y()), endPt.x() - nearEnd.x());

    QPointF arrowP1 = endPt + QPointF(qSin(angle + M_PI / 3.0) * arrowSize,
                                       qCos(angle + M_PI / 3.0) * arrowSize);
    QPointF arrowP2 = endPt + QPointF(qSin(angle + M_PI - M_PI / 3.0) * arrowSize,
                                       qCos(angle + M_PI - M_PI / 3.0) * arrowSize);

    arrowHead.clear();
    arrowHead << endPt << arrowP1 << arrowP2;
    painter->setPen(QPen(lineColor, 1));
    painter->setBrush(lineColor);
    painter->drawPolygon(arrowHead);
}
