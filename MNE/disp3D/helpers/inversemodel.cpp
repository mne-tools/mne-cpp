#include "inversemodel.h"

InverseModel::InverseModel(QObject *parent)
: QAbstractTableModel(parent)
{
}


//*************************************************************************************************************
//virtual functions
int InverseModel::rowCount(const QModelIndex & /*parent*/) const
{
//    if(!m_qMapIdxRowSelection.empty())
//        return m_qMapIdxRowSelection.size();
//    else
        return 0;
}


//*************************************************************************************************************

int InverseModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}


//*************************************************************************************************************

QVariant InverseModel::data(const QModelIndex &index, int role) const
{
//    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
//        return QVariant();

//    if (index.isValid()) {
//        qint32 row = m_qMapIdxRowSelection[index.row()];

//        //******** first column (chname) ********
//        if(index.column() == 0 && role == Qt::DisplayRole)
//            return QVariant(m_qListChInfo[row].getChannelName());

//        //******** second column (data plot) ********
//        if(index.column()==1) {
//            QVariant v;

//            switch(role) {
//                case Qt::DisplayRole: {
//                    //pack all adjacent (after reload) RowVectorPairs into a QList
//                    QList< QVector<float> > qListVector;

//                    // data
//                    QVector<float> data;
//                    for(qint32 i = 0; i < m_dataCurrent.size(); ++i)
//                        data.append(m_dataCurrent[i](row));
//                    qListVector.append(data);

//                    // last data
//                    QVector<float> lastData;
//                    for(qint32 i=0; i < m_dataLast.size(); ++i)
//                        lastData.append(m_dataLast[i](row));
//                    qListVector.append(lastData);

//                    v.setValue(qListVector);
//                    return v;
//                    break;
//                }
//                case Qt::BackgroundRole: {
////                    if(m_fiffInfo.bads.contains(m_chInfolist[row].ch_name)) {
////                        QBrush brush;
////                        brush.setStyle(Qt::SolidPattern);
////    //                    qDebug() << m_chInfolist[row].ch_name << "is marked as bad, index:" << row;
////                        brush.setColor(Qt::red);
////                        return QVariant(brush);
////                    }
////                    else
//                        return QVariant();

//                    break;
//                }
//            } // end role switch
//        } // end column check

//    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant InverseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
//        return QVariant();

//    if(orientation == Qt::Horizontal) {
//        switch(section) {
//        case 0: //chname column
//            return QVariant();
//        case 1: //data plot column
//            return QVariant("data plot");
//            switch(role) {
//            case Qt::DisplayRole:
//                return QVariant("data plot");
//            case Qt::TextAlignmentRole:
//                return QVariant(Qt::AlignLeft);
//            }
//        }
//    }
//    else if(orientation == Qt::Vertical) {
//        QModelIndex chname = createIndex(section,0);
//        switch(role) {
//        case Qt::DisplayRole:
//            return QVariant(data(chname).toString());
//        }
//    }

    return QVariant();
}
