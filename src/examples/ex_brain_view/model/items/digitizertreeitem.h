//=============================================================================================================
/**
 * @file     digitizertreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    DigitizerTreeItem class declaration.
 *
 */

#ifndef DIGITIZERTREEITEM_H
#define DIGITIZERTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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
class DigitizerTreeItem : public AbstractTreeItem
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
