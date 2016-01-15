#include "abstracttreeitem.h"

using namespace DISP3DNEWLIB;

AbstractTreeItem::AbstractTreeItem(const int& iType, const QString& text)
: QStandardItem(text)
, m_iType(iType)
{
    this->setToolTip("Unknown");
}


//*************************************************************************************************************

AbstractTreeItem::~AbstractTreeItem()
{
}


//*************************************************************************************************************

QVariant AbstractTreeItem::data(int role) const
{
    return QStandardItem::data(role);
}


//*************************************************************************************************************

void  AbstractTreeItem::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);

    switch(role) {
        case Qt::CheckStateRole:{
            emit checkStateChanged(this->checkState());
            break;
        }
    }
}


//*************************************************************************************************************

int  AbstractTreeItem::type() const
{
    return m_iType;
}


//*************************************************************************************************************

QList<QStandardItem*> AbstractTreeItem::findChildren(const int& type)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->type() == type) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

QList<QStandardItem*> AbstractTreeItem::findChildren(const QString& text)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->text() == text) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem* newItem)
{
    this->appendRow(newItem);

    return *this;
}


//*************************************************************************************************************

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem& newItem)
{
    this->appendRow(&newItem);

    return *this;
}
