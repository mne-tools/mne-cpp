//=============================================================================================================
/**
* @file     edge.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Edge class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "edge.h"
#include "node.h"
#include "network.h"
#include "../helpers/colormap.h"

//*************************************************************************************************************
//=============================================================================================================
// System INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPainter>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

//Definitions for fast plotting
static const double Pi = 3.14159265358979323846264338327950288419717;
static double PiHalf = Pi / 2.0;
static double PiThreeHalf = (3.0 / 2.0) * Pi;
static double TwoPi = 2.0 * Pi;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Edge::Edge(Network *network, Node *sourceNode, Node *destNode)
: m_pNetwork(network)
, m_arrowSize(10)
, m_color(Qt::lightGray)
, m_weight(0)
{
    setAcceptedMouseButtons(0);
    m_pSource = sourceNode;
    m_pDest = destNode;
    m_pSource->addEdge(this);
    m_pDest->addEdge(this);
    adjust();
}


//*************************************************************************************************************

Node *Edge::sourceNode() const
{
    return m_pSource;
}


//*************************************************************************************************************

Node *Edge::destNode() const
{
    return m_pDest;
}


//*************************************************************************************************************

void Edge::adjust()
{
    if (!m_pSource || !m_pDest)
        return;

    QLineF line(mapFromItem(m_pSource, 0, 0), mapFromItem(m_pDest, 0, 0));
    qreal length = line.length();

    prepareGeometryChange();

    if (length > qreal(20.)) {

        QPointF edgeOffset;
        // Continuous around the circle
//        edgeOffset = QPointF ((line.dx() * 10) / length, (line.dy() * 10) / length);
        // In 90 degree steps
//        if(abs(line.dx()) > abs(line.dy())) {
            edgeOffset = QPointF(line.dx() < 0 ? -10 : 10, 0);
//        }
//        else {
//            edgeOffset = QPointF(0, line.dy() < 0 ? -10 : 10);
//        }
        m_sourcePoint = line.p1() + edgeOffset;
        m_destPoint = line.p2() - edgeOffset;
    } else {
        m_sourcePoint = m_destPoint = line.p1();
    }
}


//*************************************************************************************************************

void Edge::setWeight(float weight)
{
    m_weight = weight;
    updateColor();
}


//*************************************************************************************************************

QRectF Edge::boundingRect() const
{
    if (!m_pSource || !m_pDest)
        return QRectF();

    qreal penWidth = 1;
    qreal extra = (penWidth + m_arrowSize) / 2.0;

    return QRectF(m_sourcePoint, QSizeF(m_destPoint.x() - m_sourcePoint.x(),
                                      m_destPoint.y() - m_sourcePoint.y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}


//*************************************************************************************************************

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (!m_pSource || !m_pDest)
        return;

//    QLineF center_line(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0));
    QLineF line(m_sourcePoint, m_destPoint);
    if (qFuzzyCompare(line.length(), qreal(0.)))
        return;

    //
    // Draw the line itself
    //

    // Construct a straigth path
//    painter->setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
//    painter->drawLine(line);

    // Construct a Bezier path
    QPainterPath path;
    path.moveTo(m_sourcePoint);
    QPointF c1, c2;
//    if(abs(center_line.dx()) > abs(center_line.dy())) {
        c1.setX(m_sourcePoint.x() + line.dx() / 2);
        c1.setY(m_sourcePoint.y());

        c2.setX(m_destPoint.x() - line.dx() / 2);
        c2.setY(m_destPoint.y());
//    }
//    else {
//        c1.setX(m_sourcePoint.x());
//        c1.setY(m_sourcePoint.y() + line.dy() / 2);

//        c2.setX(m_destPoint.x());
//        c2.setY(m_destPoint.y() - line.dy() / 2);
//    }
    path.cubicTo(c1, c2, m_destPoint);
    QColor lg = m_color;
    float penWidth = std::fabs(m_weight*m_pNetwork->weightStrength());
    QPen pen(lg, penWidth, m_pNetwork->getPenStyle(), Qt::FlatCap);//(lg, m_penWidth, m_penStyle, m_capStyle, m_joinStyle);
    painter->strokePath(path, pen);


    //
    // Draw the arrows
    //

    // Continuous
//    double angle = ::acos(line.dx() / line.length());
//    if (line.dy() >= 0)
//        angle = TwoPi - angle;

    // In 90 degree steps
    double angle = 0;
//    if(abs(center_line.dx()) > abs(center_line.dy())) {
        angle = line.dx() < 0 ? Pi : 0;
//    }
//    else {
//        angle = line.dy() < 0 ? PiHalf : PiThreeHalf;
//    }

//    QPointF sourceArrowP1 = sourcePoint + QPointF(sin(angle + Pi / 3) * arrowSize,
//                                                  cos(angle + Pi / 3) * arrowSize);
//    QPointF sourceArrowP2 = sourcePoint + QPointF(sin(angle + Pi - Pi / 3) * arrowSize,
//                                                  cos(angle + Pi - Pi / 3) * arrowSize);
    QPointF destArrowP1 = m_destPoint + QPointF(sin(angle - Pi / 3) * m_arrowSize,
                                              cos(angle - Pi / 3) * m_arrowSize);
    QPointF destArrowP2 = m_destPoint + QPointF(sin(angle - Pi + Pi / 3) * m_arrowSize,
                                              cos(angle - Pi + Pi / 3) * m_arrowSize);

    painter->setBrush(Qt::lightGray);
//    painter->drawPolygon(QPolygonF() << line.p1() << sourceArrowP1 << sourceArrowP2);
    painter->drawPolygon(QPolygonF() << line.p2() << destArrowP1 << destArrowP2);
}


//*************************************************************************************************************

void Edge::updateColor()
{
    float weigt = m_weight;
    if(weigt < -1.0f) {
        weigt = -1.0f;
    }
    if(weigt > 1.0f) {
        weigt = 1.0f;
    }

    m_color = QColor(DISPLIB::ColorMap::valueToRedBlue(weigt));
    m_color.setAlpha(static_cast<int>(std::abs(weigt*255.0f)));
}

