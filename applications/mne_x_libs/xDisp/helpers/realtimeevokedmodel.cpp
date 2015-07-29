//=============================================================================================================
/**
* @file     realtimeevokedmodel.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the RealTimeEvokedModel Class.
*
*/

#include "realtimeevokedmodel.h"

#include <QDebug>
#include <QBrush>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedModel::RealTimeEvokedModel(QObject *parent)
: QAbstractTableModel(parent)
, m_matData(MatrixXd(0,0))
, m_matDataFreeze(MatrixXd(0,0))
, m_fSps(1024.0f)
, m_bIsFreezed(false)
, m_bProjActivated(false)
{

}


//*************************************************************************************************************
//virtual functions
int RealTimeEvokedModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qMapIdxRowSelection.empty())
        return m_qMapIdxRowSelection.size();
    else
        return 0;
}


//*************************************************************************************************************

int RealTimeEvokedModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant RealTimeEvokedModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != RealTimeEvokedModelRoles::GetAverageData)
        return QVariant();

    if (index.isValid()) {
        qint32 row = m_qMapIdxRowSelection[index.row()];

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(m_pRTE->info()->ch_names[row]);

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList
                    RowVectorXd rowVec;

                    if(m_bIsFreezed) {
                        // data freeze
                        if(m_filterData.isEmpty())
                            rowVec = m_matDataFreeze.row(row);
                        else
                            rowVec = m_matDataFilteredFreeze.row(row);
                    }
                    else {
                        // data stream
                        if(m_filterData.isEmpty())
                            rowVec = m_matData.row(row);
                        else
                            rowVec = m_matDataFiltered.row(row);
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

        //******** third column (evoked set data types) ********
        if(index.column()==2) {
            QVariant v;
            RowVectorPair averagedData;

            switch(role) {
                case RealTimeEvokedModelRoles::GetAverageData: {
                    if(m_bIsFreezed){
                        // data freeze
                        averagedData.first = m_matDataFreeze.data();
                        averagedData.second = m_matDataFreeze.cols();
                        v.setValue(averagedData);
                    }
                    else {
                        // data
                        averagedData.first = m_matData.data();
                        averagedData.second = m_matData.cols();
                        v.setValue(averagedData);
                    }
                }
            }

            return v;
        }//end column check

    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant RealTimeEvokedModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void RealTimeEvokedModel::setRTE(QSharedPointer<RealTimeEvoked> &pRTE)
{
    beginResetModel();
    m_pRTE = pRTE;

    //Generate bad channel index list
    RowVectorXi sel;// = RowVectorXi(0,0);
    QStringList emptyExclude;

    if(m_pRTE->info()->bads.size() > 0)
        sel = FiffInfoBase::pick_channels(m_pRTE->info()->ch_names, m_pRTE->info()->bads, emptyExclude);

    m_vecBadIdcs = sel;

    //Create the initial SSP projector
    updateProjection();

    createFilterChannelList(m_pRTE->info()->ch_names);

    endResetModel();

    resetSelection();
}


//*************************************************************************************************************

void RealTimeEvokedModel::updateData()
{
    bool doProj = m_bProjActivated && m_matData.cols() > 0 && m_matData.rows() == m_matProj.cols() ? true : false;

    if(!doProj)
        m_matData = m_pRTE->getValue()->data;
    else
        m_matData = m_matSparseProj * m_pRTE->getValue()->data;

    if(!m_filterData.isEmpty())
        filterChannelsConcurrently();

    m_bIsInit = true;

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_pRTE->info()->nchan-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

QColor RealTimeEvokedModel::getColor(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->chColor()[chRow];
    }
    else
        return QColor();
}


//*************************************************************************************************************

fiff_int_t RealTimeEvokedModel::getKind(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].kind;
    }
    else
        return 0;
}


//*************************************************************************************************************

fiff_int_t RealTimeEvokedModel::getUnit(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].unit;
    }
    else
        return FIFF_UNIT_NONE;
}


//*************************************************************************************************************

fiff_int_t RealTimeEvokedModel::getCoil(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size())
    {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].coil_type;
    }
    else
        return FIFFV_COIL_NONE;
}


//*************************************************************************************************************

void RealTimeEvokedModel::selectRows(const QList<qint32> &selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i)
    {
        if(selection[i] < m_pRTE->info()->nchan)
        {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}


//*************************************************************************************************************

void RealTimeEvokedModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_pRTE->info()->nchan; ++i)
        m_qMapIdxRowSelection.insert(i,i);

    endResetModel();
}


//*************************************************************************************************************

void RealTimeEvokedModel::setScaling(const QMap< qint32,float >& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}


//*************************************************************************************************************

void RealTimeEvokedModel::updateProjection()
{
    //
    //  Update the SSP projector
    //
    if(m_pRTE->info()->chs.size()>0)
    {
        m_bProjActivated = false;
        for(qint32 i = 0; i < m_pRTE->info()->projs.size(); ++i)
            if(m_pRTE->info()->projs[i].active)
                m_bProjActivated = true;

        m_pRTE->info()->make_projector(m_matProj);
        qDebug() << "updateProjection :: New projection calculated.";

        //set columns of matrix to zero depending on bad channels indexes
        RowVectorXi sel;// = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(m_pRTE->info()->bads.size() > 0)
            sel = FiffInfoBase::pick_channels(m_pRTE->info()->ch_names, m_pRTE->info()->bads, emptyExclude);

        m_vecBadIdcs = sel;

        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j)
            m_matProj.col(m_vecBadIdcs[j]).setZero();

//        std::cout << "Bads\n" << m_vecBadIdcs << std::endl;
//        std::cout << "Proj\n";
//        std::cout << m_matProj.block(0,0,10,10) << std::endl;

        qint32 nchan = m_pRTE->info()->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        //
        // Make proj sparse
        //
        tripletList.clear();
        tripletList.reserve(m_matProj.rows()*m_matProj.cols());
        for(i = 0; i < m_matProj.rows(); ++i)
            for(k = 0; k < m_matProj.cols(); ++k)
                if(m_matProj(i,k) != 0)
                    tripletList.push_back(T(i, k, m_matProj(i,k)));

        m_matSparseProj = SparseMatrix<double>(m_matProj.rows(),m_matProj.cols());
        if(tripletList.size() > 0)
            m_matSparseProj.setFromTriplets(tripletList.begin(), tripletList.end());
    }
}


//*************************************************************************************************************

void RealTimeEvokedModel::toggleFreeze()
{
    m_bIsFreezed = !m_bIsFreezed;

    if(m_bIsFreezed) {
        m_matDataFilteredFreeze = m_matDataFiltered;
        m_matDataFreeze = m_matData;
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(this->rowCount(),1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

void RealTimeEvokedModel::filterChanged(QList<FilterData> filterData)
{
    m_filterData = filterData;

    m_iMaxFilterLength = 1;
    for(int i=0; i<filterData.size(); i++)
        if(m_iMaxFilterLength<filterData.at(i).m_iFilterOrder)
            m_iMaxFilterLength = filterData.at(i).m_iFilterOrder;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void RealTimeEvokedModel::setFilterChannelType(QString channelType)
{
    m_sFilterChannelType = channelType;
    m_filterChannelList = m_visibleChannelList;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_filterChannelList.clear();

    for(int i = 0; i<m_pRTE->info()->chs.size(); i++) {
        if((m_pRTE->info()->chs.at(i).kind == FIFFV_MEG_CH || m_pRTE->info()->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pRTE->info()->chs.at(i).kind == FIFFV_EOG_CH || m_pRTE->info()->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pRTE->info()->chs.at(i).kind == FIFFV_EMG_CH) && !m_pRTE->info()->bads.contains(m_pRTE->info()->chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All")
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            else if(m_pRTE->info()->chs.at(i).ch_name.contains(m_sFilterChannelType))
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
        }
    }

//    if(channelType != "All") {
//        QMutableListIterator<QString> i(m_filterChannelList);
//        while(i.hasNext()) {
//            QString val = i.next();
//            if(!val.contains(channelType, Qt::CaseInsensitive)) {
//                i.remove();
//            }
//        }
//    }

//    m_bDrawFilterFront = false;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void RealTimeEvokedModel::createFilterChannelList(QStringList channelNames)
{
    m_filterChannelList.clear();
    m_visibleChannelList = channelNames;

    //    //Create channel fiter list based on channelNames
    //    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
    //        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
    //            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
    //            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH) && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
    //            if(m_sFilterChannelType == "All" && channelNames.contains(m_pFiffInfo->chs.at(i).ch_name))
    //                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
    //            else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType) && channelNames.contains(m_pFiffInfo->chs.at(i).ch_name))
    //                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
    //        }
    //    }

    //Create channel filter list independent from channelNames
    for(int i = 0; i<m_pRTE->info()->chs.size(); i++) {
        if((m_pRTE->info()->chs.at(i).kind == FIFFV_MEG_CH || m_pRTE->info()->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pRTE->info()->chs.at(i).kind == FIFFV_EOG_CH || m_pRTE->info()->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pRTE->info()->chs.at(i).kind == FIFFV_EMG_CH) && !m_pRTE->info()->bads.contains(m_pRTE->info()->chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All")
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            else if(m_pRTE->info()->chs.at(i).ch_name.contains(m_sFilterChannelType))
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
        }
    }

//    m_bDrawFilterFront = false;

//    for(int i = 0; i<m_filterChannelList.size(); i++)
//        std::cout<<m_filterChannelList.at(i).toStdString()<<std::endl;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void doFilterPerChannelRTE(QPair<QList<FilterData>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i=0; i<channelDataTime.first.size(); i++)
        //channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, true, FilterData::ZeroPad);
        channelDataTime.second.second = channelDataTime.first.at(i).applyFFTFilter(channelDataTime.second.second, true, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
}


//*************************************************************************************************************

void RealTimeEvokedModel::filterChannelsConcurrently()
{
    std::cout<<"START RealTimeEvokedModel::filterChannelsConcurrently()"<<std::endl;

    if(m_filterData.isEmpty())
        return;

    //Create temporary filters with higher fft length because we are going to filter all available data at once for one time
    QList<FilterData> tempFilterList;

    int fftLength = m_matData.row(0).cols() + 2 * m_iMaxFilterLength;
    int exp = ceil(MNEMath::log2(fftLength));
    fftLength = pow(2, exp) <512 ? 512 : pow(2, exp);

    for(int i = 0; i<m_filterData.size(); i++) {
        FilterData tempFilter(m_filterData.at(i).m_sName,
                              m_filterData.at(i).m_Type,
                              m_filterData.at(i).m_iFilterOrder,
                              m_filterData.at(i).m_dCenterFreq,
                              m_filterData.at(i).m_dBandwidth,
                              m_filterData.at(i).m_dParksWidth,
                              m_filterData.at(i).m_sFreq,
                              fftLength,
                              m_filterData.at(i).m_designMethod);

        tempFilterList.append(tempFilter);
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;
    QList<int> notFilterChannelIndex;

    //Also append mirrored data in front and back to get rid of edge effects
    for(qint32 i=0; i<m_matData.rows(); ++i) {
        if(m_filterChannelList.contains(m_pRTE->info()->chs.at(i).ch_name)) {
            RowVectorXd datTemp(m_matData.row(i).cols() + 2 * m_iMaxFilterLength);
            datTemp << m_matData.row(i).head(m_iMaxFilterLength).reverse(), m_matData.row(i), m_matData.row(i).tail(m_iMaxFilterLength).reverse();
            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(tempFilterList,QPair<int,RowVectorXd>(i,datTemp)));
        }
        else
            notFilterChannelIndex.append(i);
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTE);

        future.waitForFinished();

        for(int r = 0; r<timeData.size(); r++)
            m_matDataFiltered.row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength+m_iMaxFilterLength/2, m_matData.cols());
    }

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i<notFilterChannelIndex.size(); i++)
        m_matDataFiltered.row(notFilterChannelIndex.at(i)) = m_matData.row(notFilterChannelIndex.at(i));

    std::cout<<"END RealTimeEvokedModel::filterChannelsConcurrently()"<<std::endl;
}

