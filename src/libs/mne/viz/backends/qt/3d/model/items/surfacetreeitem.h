//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surfacetreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree item wrapping a FreeSurfer @ref mne::FsSurface plus optional @ref mne::FsAnnotation parcellation.
 *
 * Carries the per-surface shader mode (Standard / Holographic /
 * Anatomical) and the optional annotation that drives the
 * @c ModeAnnotation colouring path. Changing any role here
 * triggers BrainView to repack the per-vertex colour stream and
 * issue a single buffer-update without rebuilding geometry.
 */

#ifndef SURFACETREEITEM_H
#define SURFACETREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../viz_qt_global.h"

#include "abstracttreeitem.h"
#include <mne/fs/fs_surface.h>
#include <mne/fs/fs_annotation.h>

namespace mne::viz::qt
{

/**
 * @brief Tree item representing a FreeSurfer cortical surface in the 3-D scene hierarchy.
 */
class VIZQTSHARED_EXPORT SurfaceTreeItem : public AbstractTreeItem
{
public:
    enum SurfaceRole
    {
        SurfaceDataRole = AlphaRole + 1,
        AnnotationDataRole,
        ShaderModeRole
    };

    explicit SurfaceTreeItem(const QString& text = "");
    ~SurfaceTreeItem() override = default;

    // Setters
    void setSurfaceData(const mne::FsSurface& surface);
    void setAnnotationData(const mne::FsAnnotation& annotation);
    void setShaderMode(int mode); // 0=Standard, 1=Holo, 2=Glossy

    // Getters
    mne::FsSurface surfaceData() const;
    mne::FsAnnotation annotationData() const;
    int shaderMode() const;
};

} // namespace mne::viz::qt

#endif // SURFACETREEITEM_H
