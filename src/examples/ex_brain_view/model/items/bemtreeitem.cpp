#include "bemtreeitem.h"

BemTreeItem::BemTreeItem(const QString &text, const MNELIB::MNEBemSurface &bemSurf)
    : AbstractTreeItem(text, BemItem)
    , m_bemSurface(bemSurf)
{
    // BEM surfaces are typically generic geometry without specific shader requirements initially,
    // but they might use a specific color or transparency.
}

const MNELIB::MNEBemSurface& BemTreeItem::bemSurfaceData() const
{
    return m_bemSurface;
}
