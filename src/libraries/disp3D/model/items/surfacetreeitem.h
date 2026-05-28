//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surfacetreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree item wrapping a FreeSurfer @ref FSLIB::FsSurface plus optional @ref FSLIB::FsAnnotation parcellation.
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

#include "../../disp3D_global.h"

#include "abstracttreeitem.h"
#include <fs/fs_surface.h>
#include <fs/fs_annotation.h>

/**
 * @brief Tree item representing a FreeSurfer cortical surface in the 3-D scene hierarchy.
 */
class DISP3DSHARED_EXPORT SurfaceTreeItem : public AbstractTreeItem
{
public:
    enum SurfaceRole {
        SurfaceDataRole = AlphaRole + 1,
        AnnotationDataRole,
        ShaderModeRole
    };

    explicit SurfaceTreeItem(const QString &text = "");
    ~SurfaceTreeItem() override = default;

    // Setters
    void setSurfaceData(const FSLIB::FsSurface &surface);
    void setAnnotationData(const FSLIB::FsAnnotation &annotation);
    void setShaderMode(int mode); // 0=Standard, 1=Holo, 2=Glossy

    // Getters
    FSLIB::FsSurface surfaceData() const;
    FSLIB::FsAnnotation annotationData() const;
    int shaderMode() const;
};

#endif // SURFACETREEITEM_H
