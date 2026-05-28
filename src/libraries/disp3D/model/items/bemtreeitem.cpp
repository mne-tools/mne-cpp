//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bemtreeitem.cpp
 * @since 2026
 * @date  March 2026
 * @brief Tree-item wrapping a single MNEBemSurface compartment for the BEM layer stack.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bemtreeitem.h"

BemTreeItem::BemTreeItem(const QString &text, const MNELIB::MNEBemSurface &bemSurf)
    : AbstractTreeItem(text, BemItem)
    , m_bemSurface(bemSurf)
{
    // BEM surfaces are typically generic geometry without specific shader requirements initially,
    // but they might use a specific color or transparency.
}

const MNELIB::MNEBemSurface& BemTreeItem::bemSurfaceData() const
{
    return m_bemSurface;
}
