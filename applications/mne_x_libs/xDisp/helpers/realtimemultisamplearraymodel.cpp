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
, m_iT(10)
, m_iDownsampling(10)
, m_iMaxSamples(1024)
, m_iCurrentSample(0)
, m_bIsFreezed(false)
{
}


//*************************************************************************************************************
//virtual functions
int RealTimeMultiSampleArrayModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qMapIdxRowSelection.empty())
        return m_qMapIdxRowSelection.size();
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
        qint32 row = m_qMapIdxRowSelection[index.row()];

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(m_qListChInfo[row].getChannelName());

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList
                    QList< QVector<float> > qListVector;

                    if(m_bIsFreezed)
                    {
                        // data freeze
                        QVector<float> data;
                        for(qint32 i = 0; i < m_dataCurrentFreeze.size(); ++i)
                            data.append(m_dataCurrentFreeze[i](row));
                        qListVector.append(data);

                        // last data freeze
                        QVector<float> lastData;
                        for(qint32 i=0; i < m_dataLastFreeze.size(); ++i)
                            lastData.append(m_dataLastFreeze[i](row));
                        qListVector.append(lastData);

                        v.setValue(qListVector);
                    }
                    else
                    {
                        // data
                        QVector<float> data;
                        for(qint32 i = 0; i < m_dataCurrent.size(); ++i)
                            data.append(m_dataCurrent[i](row));
                        qListVector.append(data);

                        // last data
                        QVector<float> lastData;
                        for(qint32 i=0; i < m_dataLast.size(); ++i)
                            lastData.append(m_dataLast[i](row));
                        qListVector.append(lastData);

                        v.setValue(qListVector);
                    }
                    return v;
                    break;
                }
                case Qt::BackgroundRole: {
//                    if(m_fiffInfo.bads.contains(m_chInfolist[row].ch_name)) {
//                        QBrush brush;
//                        brush.setStyle(Qt::SolidPattern);
//    //                    qDebug() << m_chInfolist[row].ch_name << "is marked as bad, index:" << row;
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

    resetSelection();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setSamplingInfo(float sps, int T, float dest_sps)
{
    beginResetModel();

    if(sps > dest_sps)
        m_iDownsampling = (qint32)ceil(sps/dest_sps);
    else
        m_iDownsampling = 1;

    m_iT = T;

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


//*************************************************************************************************************

fiff_int_t RealTimeMultiSampleArrayModel::getKind(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_qListChInfo[chRow].getKind();;
    }
    else
        return 0;

}


//*************************************************************************************************************

fiff_int_t RealTimeMultiSampleArrayModel::getUnit(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_qListChInfo[chRow].getUnit();;
    }
    else
        return FIFF_UNIT_NONE;
}


//*************************************************************************************************************

fiff_int_t RealTimeMultiSampleArrayModel::getCoil(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_qListChInfo[chRow].getCoil();;
    }
    else
        return FIFFV_COIL_NONE;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::selectRows(const QList<qint32> &selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i)
    {
        if(selection[i] < m_qListChInfo.size())
        {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_qListChInfo.size(); ++i)
        m_qMapIdxRowSelection.insert(i,i);

    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::toggleFreeze(const QModelIndex &)
{
    m_bIsFreezed = !m_bIsFreezed;

    if(m_bIsFreezed)
    {
        m_dataCurrentFreeze = m_dataCurrent;
        m_dataLastFreeze = m_dataLast;
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_qListChInfo.size()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}
