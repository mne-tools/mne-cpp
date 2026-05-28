//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file sourcespacetreeitem.h
 * @since 2026
 * @date  March 2026
 * @brief Tree item holding one hemisphere of source-space dipole positions rendered as batched spheres.
 *
 * Source-space points come from @ref MNELIB::MNESourceSpaces (the
 * decimated cortical grid used by the forward model). Each
 * hemisphere is rendered as a single batched-sphere mesh built by
 * @ref MeshFactory::createBatchedSpheres so the typical
 * ~7500-point ico-4 grid stays at one draw call per hemisphere.
 */

#ifndef SOURCESPACETREEITEM_H
#define SOURCESPACETREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"

#include <QVector3D>
#include <QVector>

//=============================================================================================================
/**
 * SourceSpaceTreeItem represents a single source space point in the tree model.
 * Each item stores its 3D position and rendering scale.
 *
 * @brief    Source space point tree item.
 */
class DISP3DSHARED_EXPORT SourceSpaceTreeItem : public AbstractTreeItem
{
public:
    //=========================================================================================================
    /**
     * Constructs a SourceSpaceTreeItem for a hemisphere.
     *
     * @param[in] text       Display text for the item (e.g. "LH", "RH").
     * @param[in] positions  3D positions of all source points in this hemisphere (in meters).
     * @param[in] color      Color for rendering.
     * @param[in] scale      Radius/size of each rendered sphere.
     * @param[in] type       Item type identifier.
     */
    explicit SourceSpaceTreeItem(const QString &text,
                                 const QVector<QVector3D> &positions,
                                 const QColor &color,
                                 float scale,
                                 int type = AbstractTreeItem::SourceSpaceItem);
    ~SourceSpaceTreeItem() = default;

    //=========================================================================================================
    /**
     * Returns all source point positions.
     *
     * @return Vector of position vectors.
     */
    const QVector<QVector3D>& positions() const;

    //=========================================================================================================
    /**
     * Returns the rendering scale.
     *
     * @return Scale value.
     */
    float scale() const;

private:
    QVector<QVector3D> m_positions;  /**< 3D positions of all source points. */
    float m_scale;                   /**< Radius/size for rendering. */
};

#endif // SOURCESPACETREEITEM_H
