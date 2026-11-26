//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     surfacetreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tree-item wrapping an FsSurface + FsAnnotation pair with shader-mode storage.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfacetreeitem.h"

namespace mne::viz::qt
{

SurfaceTreeItem::SurfaceTreeItem(const QString& text)
: AbstractTreeItem(text, SurfaceItem)
{
    // Default shader mode = 0 (Standard)
    setData(0, ShaderModeRole);
}

void SurfaceTreeItem::setSurfaceData(const mne::FsSurface& surface)
{
    QVariant v;
    v.setValue(surface);
    setData(v, SurfaceDataRole);
}

void SurfaceTreeItem::setAnnotationData(const mne::FsAnnotation& annotation)
{
    QVariant v;
    v.setValue(annotation);
    setData(v, AnnotationDataRole);
}

void SurfaceTreeItem::setShaderMode(int mode)
{
    setData(mode, ShaderModeRole);
}

mne::FsSurface SurfaceTreeItem::surfaceData() const
{
    return data(SurfaceDataRole).value<mne::FsSurface>();
}

mne::FsAnnotation SurfaceTreeItem::annotationData() const
{
    return data(AnnotationDataRole).value<mne::FsAnnotation>();
}

int SurfaceTreeItem::shaderMode() const
{
    return data(ShaderModeRole).toInt();
}

} // namespace mne::viz::qt
