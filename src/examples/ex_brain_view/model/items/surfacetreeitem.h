#ifndef SURFACETREEITEM_H
#define SURFACETREEITEM_H

#include "abstracttreeitem.h"
#include <fs/surface.h>
#include <fs/annotation.h>

class SurfaceTreeItem : public AbstractTreeItem
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
    void setSurfaceData(const FSLIB::Surface &surface);
    void setAnnotationData(const FSLIB::Annotation &annotation);
    void setShaderMode(int mode); // 0=Standard, 1=Holo, 2=Glossy

    // Getters
    FSLIB::Surface surfaceData() const;
    FSLIB::Annotation annotationData() const;
    int shaderMode() const;
};

#endif // SURFACETREEITEM_H
