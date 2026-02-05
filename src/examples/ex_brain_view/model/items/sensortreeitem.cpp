#include "sensortreeitem.h"

SensorTreeItem::SensorTreeItem(const QString& text, const QVector3D& pos, const QColor& color, float scale, int type)
    : AbstractTreeItem(text, type)
    , m_pos(pos)
    , m_scale(scale)
{
    setColor(color);
}

QVector3D SensorTreeItem::position() const
{
    return m_pos;
}

float SensorTreeItem::scale() const
{
    return m_scale;
}
