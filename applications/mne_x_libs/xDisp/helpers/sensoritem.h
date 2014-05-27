#ifndef SENSORITEM_H
#define SENSORITEM_H

#include <QGraphicsItem>
#include <QStyleOptionGraphicsItem>
#include <QPainter>

class SensorItem : public QGraphicsItem
{
public:
    SensorItem(const QString& chName, const QPointF& coordinate);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);

//protected:
//    void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
//    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QString m_sChName;
    QPointF m_qPointFCoord;
    QColor m_qColor;
    float m_fWidth;
    float m_fHeight;
    QVector<QPointF> stuff;
};

#endif // SENSORITEM_H
