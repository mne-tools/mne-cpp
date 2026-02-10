//=============================================================================================================
/**
 * @file     abstracttreeitem.h
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
 * @brief    AbstractTreeItem class declaration.
 *
 */

#ifndef ABSTRACTTREEITEM_H
#define ABSTRACTTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QStandardItem>
#include <QVariant>
#include <QMatrix4x4>
#include <QVector3D>

class AbstractTreeItem : public QStandardItem
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
        DigitizerItem
    };

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
