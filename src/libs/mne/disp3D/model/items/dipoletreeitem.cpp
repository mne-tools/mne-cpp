//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipoletreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree-item holding a fitted InvEcdSet for the DipoleObject instancing pipeline.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipoletreeitem.h"

namespace mne::disp
{

DipoleTreeItem::DipoleTreeItem(const QString& text, const mne::InvEcdSet& set, int type)
: AbstractTreeItem(text, type)
, m_ecdSet(set)
{
}

const mne::InvEcdSet& DipoleTreeItem::ecdSet() const
{
    return m_ecdSet;
}

} // namespace mne::disp
