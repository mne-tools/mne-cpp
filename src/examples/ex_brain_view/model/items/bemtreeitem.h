#ifndef BEMTREEITEM_H
#define BEMTREEITEM_H

#include "abstracttreeitem.h"
#include <mne/mne_bem_surface.h>

class BemTreeItem : public AbstractTreeItem
{
public:
    explicit BemTreeItem(const QString &text = "", const MNELIB::MNEBemSurface &bemSurf = MNELIB::MNEBemSurface());
    virtual ~BemTreeItem() = default;

    const MNELIB::MNEBemSurface& bemSurfaceData() const;

private:
    MNELIB::MNEBemSurface m_bemSurface;
};

#endif // BEMTREEITEM_H
