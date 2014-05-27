#ifndef SENSORITEM_H
#define SENSORITEM_H

#include <QGraphicsObject>
#include <QStyleOptionGraphicsItem>
#include <QPainter>

class SensorItem : public QGraphicsObject
{
    Q_OBJECT
public:
    SensorItem(const QString& chName, const QPointF& coordinate, QGraphicsItem *parent = 0);

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget);

    inline const QString& getChannelName() const;

    inline bool isSelected() const;

    inline void setSelected(bool selected);


protected:
//    void mousePressEvent(QGraphicsSceneMouseEvent *event);
//    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
    void itemChanged(SensorItem* item);

private:
    QString m_sChName;
    QPointF m_qPointFCoord;
    float m_fWidth;
    float m_fHeight;
    bool m_bIsSelected;
};




inline const QString& SensorItem::getChannelName() const
{
    return m_sChName;
}


inline bool SensorItem::isSelected() const
{
    return m_bIsSelected;
}


inline void SensorItem::setSelected(bool selected)
{
    m_bIsSelected = selected;
    update();
}

#endif // SENSORITEM_H
