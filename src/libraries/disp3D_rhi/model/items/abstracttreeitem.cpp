//=============================================================================================================
/**
 * @file     abstracttreeitem.cpp
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
 * @brief    AbstractTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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
