#ifndef SENSORTREEITEM_H
#define SENSORTREEITEM_H

#include "abstracttreeitem.h"
#include <QVector3D>
#include <QMatrix4x4>

class SensorTreeItem : public AbstractTreeItem
{
public:
    explicit SensorTreeItem(const QString& text, const QVector3D& pos, const QColor& color, float scale, int type = AbstractTreeItem::SensorItem);
    ~SensorTreeItem() = default;

    QVector3D position() const;
    float scale() const;

    //=========================================================================================================
    /**
     * Set the coil orientation matrix (3x3 rotation from coil_trans).
     */
    void setOrientation(const QMatrix4x4 &orient);

    //=========================================================================================================
    /**
     * Returns the coil orientation (identity if not set).
     */
    const QMatrix4x4& orientation() const;

    //=========================================================================================================
    /**
     * Returns true if an explicit orientation was set.
     */
    bool hasOrientation() const;

private:
    QVector3D m_pos;
    float m_scale;
    QMatrix4x4 m_orientation;   /**< Coil orientation (3x3 rotation in 4x4). */
    bool m_hasOrientation = false;
};

#endif // SENSORTREEITEM_H
