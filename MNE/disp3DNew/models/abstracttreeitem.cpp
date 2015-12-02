#include "abstracttreeitem.h"

using namespace DISP3DNEWLIB;

AbstractTreeItem::AbstractTreeItem(QObject *parent)
: QObject(parent)
, m_parentItem(parent)
{
}


//*************************************************************************************************************

AbstractTreeItem::~AbstractTreeItem()
{
    qDeleteAll(m_childItems);
}


//*************************************************************************************************************

QHash<QByteArray, int> AbstractTreeItem::roleTypesFromName()
{
    QHash<int, QByteArray> roles = this->roleNames();
    QHash<QByteArray, int> typesForNameHash;

    foreach (const QByteArray &val, roles.values())
        typesForNameHash[val] = roles.key(val);

    return typesForNameHash;
}


//*************************************************************************************************************

AbstractTreeItem *AbstractTreeItem::parentItem()
{
    return m_parentItem;
}


//*************************************************************************************************************

void AbstractTreeItem::appendChild(AbstractTreeItem *item)
{
    m_childItems.append(item);
}


//*************************************************************************************************************

AbstractTreeItem *AbstractTreeItem::child(int row)
{
    return m_childItems.value(row);
}


//*************************************************************************************************************

int AbstractTreeItem::childCount() const
{
    return m_childItems.count();
}


//*************************************************************************************************************

int AbstractTreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<AbstractTreeItem*>(this));

    return 0;
}

