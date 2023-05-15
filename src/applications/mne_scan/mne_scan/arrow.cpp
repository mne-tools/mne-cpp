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

#include <math.h>

#include <QPen>
#include <QPainter>

//=============================================================================================================
// CONSTS
//=============================================================================================================

const qreal Pi = 3.14;

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
    m_StartItem = startItem;
    m_EndItem = endItem;
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_qColor = Qt::black;
    setPen(QPen(m_qColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

//=============================================================================================================

QRectF Arrow::boundingRect() const
{
    qreal extra = (pen().width() + 20) / 2.0;

    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}

//=============================================================================================================

QPainterPath Arrow::shape() const
{
    QPainterPath path = QGraphicsLineItem::shape();
    path.addPolygon(arrowHead);
    return path;
}

//=============================================================================================================

void Arrow::updatePosition()
{
    QLineF line(mapFromItem(m_StartItem, 0, 0), mapFromItem(m_EndItem, 0, 0));
    setLine(line);
}

//=============================================================================================================

void Arrow::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
          QWidget *)
{
    if (m_StartItem->collidesWithItem(m_EndItem))
        return;

    QPen myPen = pen();
    myPen.setColor(m_qColor);
    qreal arrowSize = 10;
    painter->setPen(myPen);
    painter->setBrush(m_qColor);

    painter->setRenderHint(QPainter::Antialiasing);

    QLineF centerLine(m_StartItem->pos(), m_EndItem->pos());
    QPolygonF endPolygon = m_EndItem->polygon();
    QPointF p1 = endPolygon.first() + m_EndItem->pos();
    QPointF p2;
    QPointF intersectPoint;
    QLineF polyLine;
    for (int i = 1; i < endPolygon.count(); ++i) {
        p2 = endPolygon.at(i) + m_EndItem->pos();
        polyLine = QLineF(p1, p2);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QLineF::IntersectType intersectType = polyLine.intersect(centerLine, &intersectPoint);
#else
        QLineF::IntersectType intersectType = polyLine.intersects(centerLine, &intersectPoint);
#endif

        if (intersectType == QLineF::BoundedIntersection) {
            break;
        }
        p1 = p2;
    }

    setLine(QLineF(intersectPoint, m_StartItem->pos()));

    double angle = ::acos(line().dx() / line().length());
    if (line().dy() >= 0)
        angle = (Pi * 2) - angle;

    QPointF arrowP1 = line().p1() + QPointF(sin(angle + Pi * 2 / 5) * arrowSize,
                                    cos(angle + Pi * 2 / 5) * arrowSize);
    QPointF arrowP2 = line().p1() + QPointF(sin(angle + Pi - Pi * 2 / 5) * arrowSize,
                                    cos(angle + Pi - Pi * 2 / 5) * arrowSize);

    arrowHead.clear();
    arrowHead << line().p1() << arrowP1 << arrowP2;
    painter->drawLine(line());
    painter->drawPolygon(arrowHead);
    if (isSelected()) {
        painter->setPen(QPen(m_qColor, 1, Qt::DashLine));
        QLineF qLine = line();
        qLine.translate(0, 4.0);
        painter->drawLine(qLine);
        qLine.translate(0,-8.0);
        painter->drawLine(qLine);
    }
}
