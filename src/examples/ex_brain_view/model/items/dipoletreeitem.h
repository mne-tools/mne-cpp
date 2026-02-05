#ifndef DIPOLETREEITEM_H
#define DIPOLETREEITEM_H

#include "abstracttreeitem.h"
#include <inverse/dipoleFit/ecd_set.h>

class DipoleTreeItem : public AbstractTreeItem
{
public:
    explicit DipoleTreeItem(const QString& text, const INVERSELIB::ECDSet& set, int type = AbstractTreeItem::DipoleItem);
    ~DipoleTreeItem() = default;

    const INVERSELIB::ECDSet& ecdSet() const;

private:
    INVERSELIB::ECDSet m_ecdSet;
};

#endif // DIPOLETREEITEM_H
