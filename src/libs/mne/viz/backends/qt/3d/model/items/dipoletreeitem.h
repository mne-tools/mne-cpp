//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipoletreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree item wrapping a fitted @ref mne::InvEcdSet of equivalent current dipoles.
 *
 * Each dipole becomes one instance in the @ref mne::viz::DipoleObject arrow
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

#include "../../../viz_qt_global.h"

#include "abstracttreeitem.h"
#include <mne/inv/dipole_fit/inv_ecd_set.h>

namespace mne::viz::qt
{

/**
 * @brief Tree item representing a set of fitted dipoles in the 3-D scene hierarchy.
 */
class VIZQTSHARED_EXPORT DipoleTreeItem : public AbstractTreeItem
{
public:
    explicit DipoleTreeItem(const QString& text, const mne::InvEcdSet& set, int type = AbstractTreeItem::DipoleItem);
    ~DipoleTreeItem() = default;

    const mne::InvEcdSet& ecdSet() const;

private:
    mne::InvEcdSet m_ecdSet;
};

} // namespace mne::viz::qt

#endif // DIPOLETREEITEM_H
