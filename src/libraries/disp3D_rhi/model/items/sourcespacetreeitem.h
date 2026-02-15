//=============================================================================================================
/**
 * @file     sourcespacetreeitem.h
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
 * @brief    SourceSpaceTreeItem class declaration.
 *
 */

#ifndef SOURCESPACETREEITEM_H
#define SOURCESPACETREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_rhi_global.h"

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
class DISP3DRHISHARED_EXPORT SourceSpaceTreeItem : public AbstractTreeItem
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
