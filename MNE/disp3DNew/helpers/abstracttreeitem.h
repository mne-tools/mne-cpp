#ifndef ABSTRACTTREEITEM_H
#define ABSTRACTTREEITEM_H

#include <QObject>
#include <QVariant>
#include <QHash>

namespace DISP3DNEWLIB
{

class AbstractTreeItem : public QObject
{
    Q_OBJECT

public :
    AbstractTreeItem(int iDataRole, QString sDesc, AbstractTreeItem *parent = 0);
    virtual ~AbstractTreeItem();

    QHash<QByteArray, int> roleTypesFromName();
    AbstractTreeItem* parentItem();
    virtual void appendChild(AbstractTreeItem *child);
    virtual AbstractTreeItem *child(int row);
    virtual int childCount() const;
    virtual int row() const;
    virtual bool setData(int role, const QVariant &value);

    virtual QHash<int, QByteArray> roleNames() const = 0;
    virtual int columnCount() const = 0;
    virtual QVariant data(int column, int role) const = 0;

protected :
    QList<AbstractTreeItem*>    m_childItems;       /**< The list of all children of this tree item. */

    int                         m_iDataRole;
    QString                     m_sDesc;
    AbstractTreeItem*           m_pParentItem;
};

}

#endif // ABSTRACTTREEITEM_H
