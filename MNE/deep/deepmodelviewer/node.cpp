#include "edge.h"
#include "node.h"
#include "deepviewerwidget.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>



static const double Pi = 3.14159265358979323846264338327950288419717;
static double TwoPi = 2.0 * Pi;


Node::Node(DeepViewerWidget *graphWidget)
: graph(graphWidget)
, m_diameter(20)
, m_radius(m_diameter/2)
{


    setFlag(ItemIsMovable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
}


//*************************************************************************************************************

void Node::addEdge(Edge *edge)
{
    edgeList << edge;
    edge->adjust();
}


//*************************************************************************************************************

QList<Edge *> Node::edges() const
{
    return edgeList;
}


//*************************************************************************************************************

QRectF Node::boundingRect() const
{
    qreal adjust = 2;
    return QRectF( -m_radius - adjust, -m_radius - adjust, m_diameter + 3 + adjust, m_diameter + 3 + adjust);
}


//*************************************************************************************************************

QPainterPath Node::shape() const
{
    QPainterPath path;
    path.addEllipse(-10, -10, 20, 20);
    return path;
}


//*************************************************************************************************************

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
    painter->setPen(QColor(50, 100, 120, 200));
    painter->setBrush(QColor(200, 200, 210, 120));

    painter->drawEllipse(-m_radius, -m_radius, m_diameter, m_diameter);
}


//*************************************************************************************************************

QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
    case ItemPositionHasChanged:
        foreach (Edge *edge, edgeList)
            edge->adjust();
        break;
    default:
        break;
    };

    return QGraphicsItem::itemChange(change, value);
}


//*************************************************************************************************************

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mousePressEvent(event);
}


//*************************************************************************************************************

void Node::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}
