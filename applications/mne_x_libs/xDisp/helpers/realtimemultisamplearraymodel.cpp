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
, m_fSps(1024.0f)
, m_fT(10)
, m_iDownsampling(10)
, m_iMaxSamples(1024)
, m_iCurrentSample(0)
{
}


//*************************************************************************************************************
//virtual functions
int RealTimeMultiSampleArrayModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qListChInfo.empty())
        return m_qListChInfo.size();
    else
        return 0;
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
                    //pack all adjacent (after reload) RowVectorPairs into a QList
                    QList< QVector<float> > qListVector;

                    // data
                    QVector<float> data;
                    for(qint16 i=0; i < m_dataCurrent.size(); ++i)
                        data.append(m_dataCurrent[i](index.row()));
                    qListVector.append(data);

                    // last data
                    QVector<float> lastData;
                    for(qint16 i=0; i < m_dataLast.size(); ++i)
                        lastData.append(m_dataLast[i](index.row()));
                    qListVector.append(lastData);

                    v.setValue(qListVector);
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

void RealTimeMultiSampleArrayModel::setSamplingInfo(float sps, float T, float dest_sps)
{
    beginResetModel();

    if(sps > dest_sps)
        m_iDownsampling = (qint32)ceil(sps/dest_sps);
    else
        m_iDownsampling = 1;

    float maxSamples = sps * T;
    m_iMaxSamples = (qint32)ceil(maxSamples/(sps/dest_sps)); // Max Samples / Downsampling

    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::addData(const QVector<VectorXd> &data)
{
    //Downsampling ->ToDo make this more accurate
    qint32 i;
    for(i = m_iCurrentSample; i < data.size(); i += m_iDownsampling)
        m_dataCurrent.append(data[i]);

    //store for next buffer
    m_iCurrentSample = i - data.size();

    //ToDo separate worker thread? ToDo 2000 -> size of screen
    if(m_dataCurrent.size() > m_iMaxSamples)
    {
        m_dataLast = m_dataCurrent.mid(0,m_iMaxSamples); // Store last data to keep as background in the display
        m_dataCurrent.remove(0, m_iMaxSamples);
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_qListChInfo.size()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}
