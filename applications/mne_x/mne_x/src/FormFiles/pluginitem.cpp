//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "pluginitem.h"
#include "arrow.h"

#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

PluginItem::PluginItem(QString name, DiagramType diagramType, QMenu *contextMenu, QGraphicsItem *parent)
: QGraphicsPolygonItem(parent)
, m_sName(name)
, m_diagramType(diagramType)
, m_contextMenu(contextMenu)
, m_iWidth(60)
, m_iHeight(40)
{
    QPainterPath path;
    m_qLinearGradientFace = QLinearGradient(m_iWidth/2, -10, m_iWidth/2, m_iHeight);
    m_qLinearGradientFace.setColorAt(1, Qt::white);
    m_qPolygon << QPointF(-m_iWidth/2, -m_iHeight/2) << QPointF(m_iWidth/2, -m_iHeight/2)
               << QPointF(m_iWidth/2, m_iHeight/2) << QPointF(-m_iWidth/2, m_iHeight/2)
               << QPointF(-m_iWidth/2, -m_iHeight/2);
    switch (m_diagramType) {
        case StartEnd:
            m_qColorContour = QColor(79, 136, 187);
            m_qLinearGradientFace.setColorAt(0, QColor(234, 239, 247));
            break;
        case Algorithm:
            m_qColorContour = QColor(98, 152, 61);
            m_qLinearGradientFace.setColorAt(0, QColor(235, 241, 233));
            break;
        case Sensor:
            m_qColorContour = QColor(79, 136, 187);
            m_qLinearGradientFace.setColorAt(0, QColor(234, 239, 247));
            break;
        default:
            m_qColorContour = QColor(224, 169, 0);
            m_qLinearGradientFace.setColorAt(0, QColor(255, 244, 231));
            break;
    }
    setBrush(m_qLinearGradientFace);
    setPen(QPen(m_qColorContour,1));
    setPolygon(m_qPolygon);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}


//*************************************************************************************************************

void PluginItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
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

    painter->drawText(-m_iWidth/2+4,-m_iHeight/2+14,m_sName.mid(0,10));

}


//*************************************************************************************************************

void PluginItem::removeArrow(Arrow *arrow)
{
    int index = arrows.indexOf(arrow);

    if (index != -1)
        arrows.removeAt(index);
}


//*************************************************************************************************************

void PluginItem::removeArrows()
{
    foreach (Arrow *arrow, arrows) {
        arrow->startItem()->removeArrow(arrow);
        arrow->endItem()->removeArrow(arrow);
        scene()->removeItem(arrow);
        delete arrow;
    }
}


//*************************************************************************************************************

void PluginItem::addArrow(Arrow *arrow)
{
    arrows.append(arrow);
}


//*************************************************************************************************************

QPixmap PluginItem::image() const
{
    QPixmap pixmap(m_iWidth, m_iHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(QPen(m_qColorContour, 2));
    painter.translate(m_iWidth/2, m_iHeight/2);

    painter.drawPolyline(m_qPolygon);

    painter.drawText(-m_iWidth/2+4,-m_iHeight/2+14,m_sName.mid(0,10));

    return pixmap;
}


//*************************************************************************************************************

void PluginItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    scene()->clearSelection();
    setSelected(true);
    m_contextMenu->exec(event->screenPos());
}


//*************************************************************************************************************

QVariant PluginItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange) {
        foreach (Arrow *arrow, arrows) {
            arrow->updatePosition();
        }
    }

    return value;
}
