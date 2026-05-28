//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file abstracttreeitem.h
 * @since March 2026
 * @brief Base @ref QStandardItem with check-state, visibility, transform, colour and alpha roles shared by every disp3D scene item.
 *
 * AbstractTreeItem centralises the data roles every scene object
 * needs (@c VisibleRole, @c TransformRole, @c ColorRole,
 * @c AlphaRole) so the renderer can read render state directly
 * from the model without per-type if-chains. The @ref ItemType enum
 * drives type-safe down-casts at the few sites that need them
 * (picking, persistence).
 */

#ifndef ABSTRACTTREEITEM_H
#define ABSTRACTTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include <QStandardItem>
#include <QVariant>
#include <QMatrix4x4>
#include <QVector3D>

/**
 * @brief Base tree item providing check-state, visibility, and data-role storage for all 3-D scene items.
 */
class DISP3DSHARED_EXPORT AbstractTreeItem : public QStandardItem
{
public:
    enum ItemRole {
        TypeRole = Qt::UserRole + 100,
        VisibleRole,
        TransformRole,
        ColorRole,
        AlphaRole
    };

    enum ItemType {
        AbstractItem = 0,
        SurfaceItem,
        BemItem,
        SensorItem,
        DipoleItem,
        SourceSpaceItem,
        DigitizerItem,
        NetworkItem
    };

    static constexpr int itemTypeId(ItemType type)
    {
        return QStandardItem::UserType + static_cast<int>(type);
    }

    explicit AbstractTreeItem(const QString &text = "", int type = AbstractItem);
    virtual ~AbstractTreeItem() = default;

    int type() const override;

    // Type-safe accessors for common properties
    void setVisible(bool visible);
    bool isVisible() const;

    void setTransform(const QMatrix4x4 &transform);
    QMatrix4x4 transform() const;

    void setColor(const QColor &color);
    QColor color() const;

    void setAlpha(float alpha);
    float alpha() const;

protected:
    int m_type;
};

#endif // ABSTRACTTREEITEM_H
