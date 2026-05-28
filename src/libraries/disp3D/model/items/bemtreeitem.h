//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bemtreeitem.h
 * @since March 2026
 * @brief Tree item wrapping a single @ref MNELIB::MNEBemSurface (brain / inner-skull / outer-skull / scalp shell).
 *
 * BEM surfaces are triangulated boundary meshes produced by
 * @c mne_watershed_bem or the FreeSurfer @c mri_make_bem_surfaces
 * tool. Each compartment lives in its own tree item so the user
 * can toggle scalp / skull / brain layers independently &mdash;
 * essential for inspecting forward-model conductor geometry.
 */

#ifndef BEMTREEITEM_H
#define BEMTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"
#include <mne/mne_bem_surface.h>

/**
 * @brief Tree item representing a BEM surface layer in the 3-D scene hierarchy.
 */
class DISP3DSHARED_EXPORT BemTreeItem : public AbstractTreeItem
{
public:
    explicit BemTreeItem(const QString &text = "", const MNELIB::MNEBemSurface &bemSurf = MNELIB::MNEBemSurface());
    virtual ~BemTreeItem() = default;

    const MNELIB::MNEBemSurface& bemSurfaceData() const;

private:
    MNELIB::MNEBemSurface m_bemSurface;
};

#endif // BEMTREEITEM_H
