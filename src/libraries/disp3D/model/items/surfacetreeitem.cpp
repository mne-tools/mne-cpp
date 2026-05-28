//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file surfacetreeitem.cpp
 * @since 2026
 * @date  March 2026
 * @brief Tree-item wrapping an FsSurface + FsAnnotation pair with shader-mode storage.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfacetreeitem.h"

SurfaceTreeItem::SurfaceTreeItem(const QString &text)
    : AbstractTreeItem(text, SurfaceItem)
{
    // Default shader mode = 0 (Standard)
    setData(0, ShaderModeRole);
}

void SurfaceTreeItem::setSurfaceData(const FSLIB::FsSurface &surface)
{
    // Store as QVariant. We might need Q_DECLARE_METATYPE for FSLIB::FsSurface if not already done.
    // For now assuming FSLIB types are registered or we can register them.
    QVariant v;
    v.setValue(surface);
    setData(v, SurfaceDataRole);
}

void SurfaceTreeItem::setAnnotationData(const FSLIB::FsAnnotation &annotation)
{
    QVariant v;
    v.setValue(annotation);
    setData(v, AnnotationDataRole);
}

void SurfaceTreeItem::setShaderMode(int mode)
{
    setData(mode, ShaderModeRole);
}

FSLIB::FsSurface SurfaceTreeItem::surfaceData() const
{
    return data(SurfaceDataRole).value<FSLIB::FsSurface>();
}

FSLIB::FsAnnotation SurfaceTreeItem::annotationData() const
{
    return data(AnnotationDataRole).value<FSLIB::FsAnnotation>();
}

int SurfaceTreeItem::shaderMode() const
{
    return data(ShaderModeRole).toInt();
}
