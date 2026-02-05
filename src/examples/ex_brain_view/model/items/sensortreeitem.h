#ifndef SENSORTREEITEM_H
#define SENSORTREEITEM_H

#include "abstracttreeitem.h"
#include <QVector3D>

class SensorTreeItem : public AbstractTreeItem
{
public:
    explicit SensorTreeItem(const QString& text, const QVector3D& pos, const QColor& color, float scale, int type = AbstractTreeItem::SensorItem);
    ~SensorTreeItem() = default;

    QVector3D position() const;
    float scale() const;

private:
    QVector3D m_pos;
    float m_scale;
};

#endif // SENSORTREEITEM_H
