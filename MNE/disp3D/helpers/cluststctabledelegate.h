#ifndef CLUSTSTCTABLEDELEGATE_H
#define CLUSTSTCTABLEDELEGATE_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"

#include <QAbstractItemDelegate>
#include <QSharedPointer>

class DISP3DSHARED_EXPORT ClustStcTableDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    typedef QSharedPointer<ClustStcTableDelegate> SPtr;            /**< Shared pointer type for ClustStcTableDelegate class. */
    typedef QSharedPointer<const ClustStcTableDelegate> ConstSPtr; /**< Const shared pointer type for ClustStcTableDelegate class. */

    ClustStcTableDelegate(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Use the painter and style option to render the item specified by the item index.
    *
    * (sizeHint() must be implemented also)
    *
    * @param[in] painter    Low-level painting on widgets and other paint devices
    * @param[in] option     Describes the parameters used to draw an item in a view widget
    * @param[in] index      Used to locate data in a data model.
    */
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    //=========================================================================================================
    /**
    * Item size
    *
    * @param[in] option     Describes the parameters used to draw an item in a view widget
    * @param[in] index      Used to locate data in a data model.
    */
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:

};

#endif // CLUSTSTCTABLEDELEGATE_H
