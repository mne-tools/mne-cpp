#ifndef NODE_H
#define NODE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"


#include <QGraphicsItem>
#include <QGraphicsSvgItem>
#include <QList>

class QGraphicsSceneMouseEvent;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

class RoiSelectionWidget;


class XDISPSHARED_EXPORT Roi : public QGraphicsItem
{
public:
    Roi(const QString & fileName, RoiSelectionWidget *roiSelectionWidget);

    enum { Type = UserType + 1 };
    int type() const { return Type; }

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPointF newPos;
    RoiSelectionWidget *m_pRoiSelectionWidget;

    QGraphicsSvgItem *m_pGraphicsSvgItem;

    QGraphicsSvgItem *m_pGraphicsSvgItemActive;
};

} // NAMESPACE

#endif // NODE_H
