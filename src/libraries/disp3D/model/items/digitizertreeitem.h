//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file digitizertreeitem.h
 * @since March 2026
 * @brief Tree item holding a single category of digitizer points rendered as a batched-sphere mesh.
 *
 * All points in a category share colour and radius, so the renderer
 * uploads one interleaved vertex buffer (icosahedron geometry
 * replicated and translated) and draws the whole category with one
 * @c drawIndexed call &mdash; matching the single-draw-per-surface
 * constraint of the WebGL backend.
 */

#ifndef DIGITIZERTREEITEM_H
#define DIGITIZERTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"

#include <QVector3D>
#include <QVector>

//=============================================================================================================
/**
 * DigitizerTreeItem represents a group of digitizer points of the same category
 * (Cardinal, HPI, EEG, Extra) in the tree model. Each item stores batched 3D positions
 * for efficient rendering as a single mesh with replicated sphere geometry.
 *
 * This matches the disp3D DigitizerTreeItem pattern using per-category grouping with
 * color-coded sphere rendering.
 *
 * @brief    Digitizer point group tree item.
 */
class DISP3DSHARED_EXPORT DigitizerTreeItem : public AbstractTreeItem
{
public:
    /**
     * Digitizer point category, matching FIFF digitizer point kinds.
     */
    enum PointKind {
        Cardinal = 0,   /**< Cardinal (fiducial) points: Nasion, LPA, RPA. */
        HPI,            /**< HPI (Head Position Indicator) coil positions. */
        EEG,            /**< EEG electrode positions. */
        Extra           /**< Extra head shape digitization points. */
    };

    //=========================================================================================================
    /**
     * Constructs a DigitizerTreeItem for a single category of digitizer points.
     *
     * @param[in] text       Display text for the item (e.g. "Cardinal", "HPI", "EEG", "Extra").
     * @param[in] kind       The digitizer point category.
     * @param[in] positions  3D positions of all points in this category (in meters).
     * @param[in] names      Display names for individual points (e.g. "Nasion", "LPA").
     * @param[in] color      Color for rendering this category.
     * @param[in] scale      Radius of each rendered sphere.
     * @param[in] type       Item type identifier.
     */
    explicit DigitizerTreeItem(const QString &text,
                               PointKind kind,
                               const QVector<QVector3D> &positions,
                               const QStringList &names,
                               const QColor &color,
                               float scale,
                               int type = AbstractTreeItem::DigitizerItem);
    ~DigitizerTreeItem() = default;

    //=========================================================================================================
    /**
     * Returns all point positions in this category.
     *
     * @return Vector of 3D position vectors.
     */
    const QVector<QVector3D>& positions() const;

    //=========================================================================================================
    /**
     * Returns the display names for individual points.
     *
     * @return List of point names.
     */
    const QStringList& pointNames() const;

    //=========================================================================================================
    /**
     * Returns the rendering scale (sphere radius).
     *
     * @return Scale value in meters.
     */
    float scale() const;

    //=========================================================================================================
    /**
     * Returns the digitizer point category.
     *
     * @return Point kind enum value.
     */
    PointKind pointKind() const;

private:
    PointKind m_kind;                /**< Digitizer point category. */
    QVector<QVector3D> m_positions;  /**< 3D positions of all points. */
    QStringList m_names;             /**< Display names for individual points. */
    float m_scale;                   /**< Radius/size for rendering. */
};

#endif // DIGITIZERTREEITEM_H
