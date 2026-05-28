//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file dipoletreeitem.h
 * @since 2026
 * @date  March 2026
 * @brief Tree item wrapping a fitted @ref INVLIB::InvEcdSet of equivalent current dipoles.
 *
 * Each dipole becomes one instance in the @ref DipoleObject arrow
 * mesh: position drives the instance translation, orientation the
 * rotation, goodness-of-fit the colour mapped through the active
 * dipole colormap. Toggling visibility on this item simply hides
 * the corresponding instance range in the GPU buffer.
 */

#ifndef DIPOLETREEITEM_H
#define DIPOLETREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"
#include <inv/dipole_fit/inv_ecd_set.h>

/**
 * @brief Tree item representing a set of fitted dipoles in the 3-D scene hierarchy.
 */
class DISP3DSHARED_EXPORT DipoleTreeItem : public AbstractTreeItem
{
public:
    explicit DipoleTreeItem(const QString& text, const INVLIB::InvEcdSet& set, int type = AbstractTreeItem::DipoleItem);
    ~DipoleTreeItem() = default;

    const INVLIB::InvEcdSet& ecdSet() const;

private:
    INVLIB::InvEcdSet m_ecdSet;
};

#endif // DIPOLETREEITEM_H
