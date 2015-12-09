#include "abstracttreeitem.h"

using namespace DISP3DNEWLIB;

AbstractTreeItem::AbstractTreeItem(const int &iType, const QString & text)
: QStandardItem(text)
, m_iType(iType)
{
}


//*************************************************************************************************************

AbstractTreeItem::~AbstractTreeItem()
{
}


//*************************************************************************************************************

int  AbstractTreeItem::type() const
{
    return m_iType;
}
