//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sourcespacetreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-hemisphere source-space point storage feeding the batched-sphere renderer.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcespacetreeitem.h"

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceSpaceTreeItem::SourceSpaceTreeItem(const QString &text,
                                         const QVector<QVector3D> &positions,
                                         const QColor &color,
                                         float scale,
                                         int type)
    : AbstractTreeItem(text, type)
    , m_positions(positions)
    , m_scale(scale)
{
    setColor(color);
}

//=============================================================================================================

const QVector<QVector3D>& SourceSpaceTreeItem::positions() const
{
    return m_positions;
}

//=============================================================================================================

float SourceSpaceTreeItem::scale() const
{
    return m_scale;
}
