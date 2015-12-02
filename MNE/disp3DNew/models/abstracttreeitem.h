#ifndef ABSTRACTTREEITEM_H
#define ABSTRACTTREEITEM_H

#include <QObject>


namespace DISP3DNEWLIB
{

class AbstractTreeItem : public QObject
{
    Q_OBJECT
public :
    AbstractTreeItem(QObject *parent = 0);
    virtual ~AbstractTreeItem();

    virtual QHash<QByteArray, int> roleTypesFromName();
    virtual AbstractTreeItem* parentItem();
    virtual void appendChild(AbstractTreeItem *child);
    virtual AbstractTreeItem *child(int row);
    virtual int childCount() const;
    virtual int row() const;

    virtual QHash<int, QByteArray> roleNames() const = 0;
    virtual int columnCount() const;
    virtual QVariant data(int column, int role) const;
    virtual bool setData(int role, const QVariant &value);

protected :
    QList<AbstractTreeItem*>    m_childItems;       /**< The list of all children of this tree item. */

    AbstractTreeItem*           m_parentItem;
};

}

#endif // ABSTRACTTREEITEM_H
