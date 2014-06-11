#include "stctabledelegate.h"
#include <QPainter>
#include <QDebug>

StcTableDelegate::StcTableDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{
}

//*************************************************************************************************************

void StcTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    float t_fPlotHeight = option.rect.height();
    switch(index.column()) {
        case 0: { //index
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 1: { //vertex
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 2: { //stc value
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 3: { //stc relative value
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 4: { //roi name
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).toString());

            painter->restore();
            break;
        }
        case 5: { //roi color
            painter->save();

            QColor c = index.model()->data(index,Qt::DisplayRole).value<QColor>();

            painter->setPen(c);
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,c.name());

            painter->restore();
            break;
        }
    }

}


//*************************************************************************************************************

QSize StcTableDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size;

    switch(index.column()) {
        case 0:
            size = QSize(20,option.rect.height());
            break;
        case 1:
            size = QSize(10,option.rect.height());
            break;
    }

    return size;
}
