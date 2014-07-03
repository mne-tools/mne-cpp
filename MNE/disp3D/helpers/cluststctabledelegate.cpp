#include "cluststctabledelegate.h"
#include "cluststcmodel.h" //Declar MetaType Eigen::Matrix3Xf; FSLIB::Label
#include <QPainter>
#include <QDebug>

using namespace Eigen;

ClustStcTableDelegate::ClustStcTableDelegate(QObject *parent)
: QAbstractItemDelegate(parent)
{
}

//*************************************************************************************************************

void ClustStcTableDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
//    float t_fPlotHeight = option.rect.height();
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

            qint32 val = index.model()->data(index,Qt::DisplayRole).value<VectorXd>().size();
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(val));

            painter->restore();
            break;
        }
        case 3: { //stc relative value
            painter->save();

            qint32 val = index.model()->data(index,Qt::DisplayRole).value<VectorXd>().size();
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(val));

            painter->restore();
            break;
        }
        case 4: { //Label
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,index.model()->data(index,Qt::DisplayRole).value<Label>().name);

            painter->restore();
            break;
        }
        case 5: { //Color
            painter->save();

            QColor c = index.model()->data(index,Qt::DisplayRole).value<QColor>();

            painter->setPen(c);
            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,c.name());

            painter->restore();
            break;
        }
        case 6: { //Tri Coords
            painter->save();

            painter->drawText(option.rect,Qt::AlignVCenter|Qt::AlignLeft,QString("%1").arg(index.model()->data(index,Qt::DisplayRole).value<Matrix3Xf>().cols()));

            painter->restore();
            break;
        }
    }

}


//*************************************************************************************************************

QSize ClustStcTableDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
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
