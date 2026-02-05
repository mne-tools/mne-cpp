#include "surfacetreeitem.h"

SurfaceTreeItem::SurfaceTreeItem(const QString &text)
    : AbstractTreeItem(text, SurfaceItem)
{
    // Default shader mode = 0 (Standard)
    setData(0, ShaderModeRole);
}

void SurfaceTreeItem::setSurfaceData(const FSLIB::Surface &surface)
{
    // Store as QVariant. We might need Q_DECLARE_METATYPE for FSLIB::Surface if not already done.
    // For now assuming FSLIB types are registered or we can register them.
    QVariant v;
    v.setValue(surface);
    setData(v, SurfaceDataRole);
}

void SurfaceTreeItem::setAnnotationData(const FSLIB::Annotation &annotation)
{
    QVariant v;
    v.setValue(annotation);
    setData(v, AnnotationDataRole);
}

void SurfaceTreeItem::setShaderMode(int mode)
{
    setData(mode, ShaderModeRole);
}

FSLIB::Surface SurfaceTreeItem::surfaceData() const
{
    return data(SurfaceDataRole).value<FSLIB::Surface>();
}

FSLIB::Annotation SurfaceTreeItem::annotationData() const
{
    return data(AnnotationDataRole).value<FSLIB::Annotation>();
}

int SurfaceTreeItem::shaderMode() const
{
    return data(ShaderModeRole).toInt();
}
