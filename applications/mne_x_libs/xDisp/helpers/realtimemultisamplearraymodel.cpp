#include "realtimemultisamplearraymodel.h"

#include <QDebug>
#include <QBrush>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeMultiSampleArrayModel::RealTimeMultiSampleArrayModel(QObject *parent)
: QAbstractTableModel(parent)
{
}


//*************************************************************************************************************
//virtual functions
int RealTimeMultiSampleArrayModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qListChInfo.empty())
        return m_qListChInfo.size();
    else return 0;
}


//*************************************************************************************************************

int RealTimeMultiSampleArrayModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}


//*************************************************************************************************************

QVariant RealTimeMultiSampleArrayModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();


    if (index.isValid()) {
        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(m_qListChInfo[index.row()].getChannelName());

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole: {
                    //form RowVectorPair of pointer and length of RowVector
                    //QPair<const double*,qint32> rowVectorPair;

//                    //pack all adjacent (after reload) RowVectorPairs into a QList
//                    QList<RowVectorPair> listRowVectorPair;

//                    for(qint16 i=0; i < m_dataCurrent.size(); ++i) {
//                        //if channel is not filtered or background Processing pending...
//                        if(!m_assignedOperators.contains(index.row()) || (m_bProcessing && m_bReloadBefore && i==0) || (m_bProcessing && !m_bReloadBefore && i==m_dataCurrent.size()-1)) {
//                            rowVectorPair.first = m_dataCurrent[i].data() + index.row()*m_dataCurrent[i].cols();
//                            rowVectorPair.second  = m_dataCurrent[i].cols();
//                        }
//                        else { //if channel IS filtered
//                            rowVectorPair.first = m_procData[i].data() + index.row()*m_procData[i].cols();
//                            rowVectorPair.second  = m_procData[i].cols();
//                        }

//                        listRowVectorPair.append(rowVectorPair);
//                    }

//                    v.setValue(listRowVectorPair);


                    if(m_dataCurrent.size() > 0)
                    {
//                        qDebug() << "m_dataCurrent.size()" << index.row();
                        //pack all adjacent (after reload) RowVectorPairs into a QList
                        QVector<float> qVector;

                        for(qint16 i=0; i < m_dataCurrent.size(); ++i)
                            qVector.append(m_dataCurrent[i](index.row()));

                        v.setValue(qVector);
                    }
                    return v;
                    break;
                }
                case Qt::BackgroundRole: {
//                    if(m_fiffInfo.bads.contains(m_chInfolist[index.row()].ch_name)) {
//                        QBrush brush;
//                        brush.setStyle(Qt::SolidPattern);
//    //                    qDebug() << m_chInfolist[index.row()].ch_name << "is marked as bad, index:" << index.row();
//                        brush.setColor(Qt::red);
//                        return QVariant(brush);
//                    }
//                    else
                        return QVariant();

                    break;
                }
            } // end role switch
        } // end column check

    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant RealTimeMultiSampleArrayModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: //chname column
            return QVariant();
        case 1: //data plot column
            return QVariant("data plot");
            switch(role) {
            case Qt::DisplayRole:
                return QVariant("data plot");
            case Qt::TextAlignmentRole:
                return QVariant(Qt::AlignLeft);
            }
        }
    }
    else if(orientation == Qt::Vertical) {
        QModelIndex chname = createIndex(section,0);
        switch(role) {
        case Qt::DisplayRole:
            return QVariant(data(chname).toString());
        }
    }

    return QVariant();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setChannelInfo(QList<RealTimeSampleArrayChInfo> &chInfo)
{
    beginResetModel();


    m_qListChInfo = chInfo;


    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::addData(const QVector<VectorXd> &data)
{
    beginResetModel();

//    qDebug() << "add Current thread" << QThread::currentThread() ;
//    qDebug() << "m_dataCurrent.size()" << m_dataCurrent.size();

    //Downsampling ->ToDo make this more accurate
    for(qint32 i = 0; i < data.size(); i += 10)
        m_dataCurrent.append(data[i]);

    //ToDo separate worker thread? ToDo 2000 -> size of screen
    if(m_dataCurrent.size() > 2000)
    {
        m_dataLast = m_dataCurrent.mid(0,2000); // Store last data to keep as background in the display
        m_dataCurrent.remove(0, 2000);
    }

    endResetModel();




//    qDebug() << "m_dataCurrent.size()" << m_dataCurrent.size();

//    //ToDo Down sampling
//    m_dataCurrent << data;

////    //ToDo separate worker thread? ToDo 2000 -> size of screen
////    if(m_dataCurrent.size() > 2000)
////    {
//////        m_dataLast = m_dataCurrent.mid(0,2000); // Store last data to keep as background in the display
////        m_dataCurrent.remove(0, m_dataCurrent.size() - 2000);
////    }



}
