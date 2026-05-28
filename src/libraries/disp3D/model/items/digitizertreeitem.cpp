//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file digitizertreeitem.cpp
 * @since March 2026
 * @brief Storage of one digitizer-point category and its batched-sphere render parameters.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerTreeItem::DigitizerTreeItem(const QString &text,
                                     PointKind kind,
                                     const QVector<QVector3D> &positions,
                                     const QStringList &names,
                                     const QColor &color,
                                     float scale,
                                     int type)
    : AbstractTreeItem(text, type)
    , m_kind(kind)
    , m_positions(positions)
    , m_names(names)
    , m_scale(scale)
{
    setColor(color);
}

//=============================================================================================================

const QVector<QVector3D>& DigitizerTreeItem::positions() const
{
    return m_positions;
}

//=============================================================================================================

const QStringList& DigitizerTreeItem::pointNames() const
{
    return m_names;
}

//=============================================================================================================

float DigitizerTreeItem::scale() const
{
    return m_scale;
}

//=============================================================================================================

DigitizerTreeItem::PointKind DigitizerTreeItem::pointKind() const
{
    return m_kind;
}
