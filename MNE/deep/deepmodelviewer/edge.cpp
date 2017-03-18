
#include "edge.h"
#include "node.h"

#include <disp/helpers/colormap.h>

#include <math.h>

#include <QPainter>
#include <QDebug>

static const double Pi = 3.14159265358979323846264338327950288419717;
static double PiHalf = Pi / 2.0;
static double PiThreeHalf = (3.0 / 2.0) * Pi;
static double TwoPi = 2.0 * Pi;

//! [0]
Edge::Edge(Node *sourceNode, Node *destNode)
: m_arrowSize(10)
, m_color(Qt::lightGray)
, m_penWidth(1)
, m_weight(0)
{
    setAcceptedMouseButtons(0);
    source = sourceNode;
    dest = destNode;
    source->addEdge(this);
    dest->addEdge(this);
    adjust();
}


//*************************************************************************************************************

Node *Edge::sourceNode() const
{
    return source;
}


//*************************************************************************************************************

Node *Edge::destNode() const
{
    return dest;
}


//*************************************************************************************************************

void Edge::adjust()
{
    if (!source || !dest)
        return;

    QLineF line(mapFromItem(source, 0, 0), mapFromItem(dest, 0, 0));
    qreal length = line.length();

    prepareGeometryChange();

    if (length > qreal(20.)) {

        QPointF edgeOffset;
        // Continuous around the circle
//        edgeOffset = QPointF ((line.dx() * 10) / length, (line.dy() * 10) / length);
        // In 90 degree steps
//        if(abs(line.dx()) > abs(line.dy())) {
            edgeOffset = QPointF(line.dx() < 0 ? -11 : 11, 0);
//        }
//        else {
//            edgeOffset = QPointF(0, line.dy() < 0 ? -11 : 11);
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
    updateLineWidth();
    updateColor();
}


//*************************************************************************************************************

QRectF Edge::boundingRect() const
{
    if (!source || !dest)
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
    if (!source || !dest)
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
    QPen pen(lg, m_penWidth);//(lg, m_penWidth, m_penStyle, m_capStyle, m_joinStyle);
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

void Edge::updateLineWidth()
{
    m_penWidth = abs(m_weight*5);
}


//*************************************************************************************************************

void Edge::updateColor()
{
    m_color = QColor(DISPLIB::ColorMap::valueToRedBlue(m_weight));
    m_color.setAlpha(abs(m_weight*255));
}
