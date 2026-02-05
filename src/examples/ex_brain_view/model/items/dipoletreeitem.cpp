#include "dipoletreeitem.h"

DipoleTreeItem::DipoleTreeItem(const QString& text, const INVERSELIB::ECDSet& set, int type)
    : AbstractTreeItem(text, type)
    , m_ecdSet(set)
{
}

const INVERSELIB::ECDSet& DipoleTreeItem::ecdSet() const
{
    return m_ecdSet;
}
