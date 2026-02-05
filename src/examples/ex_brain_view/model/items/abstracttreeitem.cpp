#include "abstracttreeitem.h"

AbstractTreeItem::AbstractTreeItem(const QString &text, int type)
    : QStandardItem(text)
    , m_type(type)
{
    // Set default values
    setData(true, VisibleRole); // Visible by default
    setData(QMatrix4x4(), TransformRole); // Identity
    setData(QColor(Qt::white), ColorRole);
    setData(1.0f, AlphaRole);
}

int AbstractTreeItem::type() const
{
    return QStandardItem::UserType + m_type;
}

void AbstractTreeItem::setVisible(bool visible)
{
    setData(visible, VisibleRole);
}

bool AbstractTreeItem::isVisible() const
{
    return data(VisibleRole).toBool();
}

void AbstractTreeItem::setTransform(const QMatrix4x4 &transform)
{
    setData(transform, TransformRole);
}

QMatrix4x4 AbstractTreeItem::transform() const
{
    return data(TransformRole).value<QMatrix4x4>();
}

void AbstractTreeItem::setColor(const QColor &color)
{
    setData(color, ColorRole);
}

QColor AbstractTreeItem::color() const
{
    return data(ColorRole).value<QColor>();
}

void AbstractTreeItem::setAlpha(float alpha)
{
    setData(alpha, AlphaRole);
}

float AbstractTreeItem::alpha() const
{
    return data(AlphaRole).toFloat();
}
