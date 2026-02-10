//=============================================================================================================
/**
 * @file     sensortreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
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
 * @brief    SensorTreeItem class declaration.
 *
 */

#ifndef SENSORTREEITEM_H
#define SENSORTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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
