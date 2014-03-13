#include "roi.h"
#include "roiselectionwidget.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOption>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


Roi::Roi(const QString & fileName, RoiSelectionWidget *roiSelectionWidget)
: m_pRoiSelectionWidget(roiSelectionWidget)
{
    m_pGraphicsSvgItem = new QGraphicsSvgItem(fileName+".svg", this);
    m_pGraphicsSvgItem->setVisible(true);

    m_pGraphicsSvgItem->setFlags(QGraphicsItem::ItemClipsToShape);
    m_pGraphicsSvgItem->setCacheMode(QGraphicsItem::NoCache);
    m_pGraphicsSvgItem->setZValue(0);

    m_pGraphicsSvgItemActive = new QGraphicsSvgItem(fileName+"_active.svg", this);
    m_pGraphicsSvgItemActive->setVisible(false);

//    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsGeometryChanges);
    setCacheMode(DeviceCoordinateCache);
    setZValue(-1);
}

QRectF Roi::boundingRect() const
{
//    qreal adjust = 2;
    return m_pGraphicsSvgItem->boundingRect();//QRectF( -30 - adjust, -30 - adjust, 60 + adjust, 60 + adjust);
}

QPainterPath Roi::shape() const
{
    return m_pGraphicsSvgItem->shape();
}

void Roi::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
//    painter->setPen(Qt::NoPen);
//    painter->setBrush(Qt::darkGray);
//    painter->drawPath( m_pGraphicsSvgItem->shape());

    if (option->state & QStyle::State_Sunken)
    {
        m_pGraphicsSvgItem->setVisible(false); //painter->setBrush(QColor(Qt::blue).light(180));
        m_pGraphicsSvgItemActive->setVisible(true);
    }
    else
    {
        m_pGraphicsSvgItem->setVisible(true); //painter->setBrush(Qt::white);
        m_pGraphicsSvgItemActive->setVisible(false);
    }

//    painter->setPen(QPen(Qt::black, 0));
//    painter->drawPath( m_pGraphicsSvgItem->shape());

    Q_UNUSED(painter);

}

QVariant Roi::itemChange(GraphicsItemChange change, const QVariant &value)
{
    switch (change) {
        case ItemPositionHasChanged:
            m_pRoiSelectionWidget->itemMoved();
            break;
        default:
            break;
    };

    return QGraphicsItem::itemChange(change, value);
}

void Roi::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mousePressEvent(event);
}

void Roi::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    update();
    QGraphicsItem::mouseReleaseEvent(event);
}
