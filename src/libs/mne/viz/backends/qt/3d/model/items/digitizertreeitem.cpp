//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     digitizertreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Storage of one digitizer-point category and its batched-sphere render parameters.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

namespace mne::viz::qt
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerTreeItem::DigitizerTreeItem(const QString& text,
                                     PointKind kind,
                                     const std::vector<QVector3D>& positions,
                                     const std::vector<std::string>& names,
                                     const QColor& color,
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

const std::vector<QVector3D>& DigitizerTreeItem::positions() const
{
    return m_positions;
}

//=============================================================================================================

const std::vector<std::string>& DigitizerTreeItem::pointNames() const
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

} // namespace mne::viz::qt
