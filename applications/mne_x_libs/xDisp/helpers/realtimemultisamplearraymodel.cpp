//=============================================================================================================
/**
* @file     realtimemultisamplearraymodel.cpp
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
* @brief    Implementation of the RealTimeMultiSampleArrayModel Class.
*
*/

#include "realtimemultisamplearraymodel.h"

#include <iostream>

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

RealTimeMultiSampleArrayModel::RealTimeMultiSampleArrayModel(QObject *parent)
: QAbstractTableModel(parent)
, m_bProjActivated(false)
, m_fSps(1024.0f)
, m_iT(10)
, m_iDownsampling(10)
, m_iMaxSamples(1024)
, m_iCurrentSample(0)
, m_bIsFreezed(false)
, m_sFilterChannelType("MEG")
{
    init();
}

//*************************************************************************************************************
//virtual functions
int RealTimeMultiSampleArrayModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_pFiffInfo->chs.empty())
        return m_pFiffInfo->chs.size();
    else
        return 0;

//    if(!m_qMapIdxRowSelection.empty())
//        return m_qMapIdxRowSelection.size();
//    else
//        return 0;
}


//*************************************************************************************************************

int RealTimeMultiSampleArrayModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant RealTimeMultiSampleArrayModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if (index.isValid()) {
        qint32 row = m_qMapIdxRowSelection.value(index.row(),0);

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(m_qListChInfo[row].getChannelName());

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;
            RowVectorPair rowVectorPair;

            switch(role) {
                case Qt::DisplayRole: {
                    if(m_bIsFreezed) {
                        // data freeze
                        if(m_filterData.isEmpty()) {
                            rowVectorPair.first = m_matDataRawFreeze.data() + row*m_matDataRawFreeze.cols();
                            rowVectorPair.second  = m_matDataRawFreeze.cols();
                            v.setValue(rowVectorPair);
                        }
                        else {
                            rowVectorPair.first = m_matDataFilteredFreeze.data() + row*m_matDataFilteredFreeze.cols();
                            rowVectorPair.second  = m_matDataFilteredFreeze.cols();
                            v.setValue(rowVectorPair);
                        }
                    }
                    else {
                        // data stream
                        if(m_filterData.isEmpty()) {
                            rowVectorPair.first = m_matDataRaw.data() + row*m_matDataRaw.cols();
                            rowVectorPair.second  = m_matDataRaw.cols();
                            v.setValue(rowVectorPair);
                        }
                        else {
                            rowVectorPair.first = m_matDataFiltered.data() + row*m_matDataFiltered.cols();
                            rowVectorPair.second  = m_matDataFiltered.cols();
                            v.setValue(rowVectorPair);
                        }
                    }

                    return v;
                    break;
                }
                case Qt::BackgroundRole: {
                    if(m_pFiffInfo->bads.contains(m_qListChInfo[row].getChannelName())) {
                        QBrush brush;
                        brush.setStyle(Qt::SolidPattern);
                        //qDebug() << m_qListChInfo[row].getChannelName() << "is marked as bad, index:" << row;
                        QColor color(254,74,93);
                        color.setAlpha(40);
                        brush.setColor(color);

                        return QVariant(brush);
                    }
                    else
                        return QVariant();

                    break;
                }
            } // end role switch
        } // end column check

        //******** first column (chname) ********
        if(index.column() == 2 && role == Qt::DisplayRole)
            return QVariant(m_pFiffInfo->bads.contains(m_qListChInfo[row].getChannelName()));

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

void RealTimeMultiSampleArrayModel::init()
{
    m_pFiffInfo = FiffInfo::SPtr(new FiffInfo());
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

void RealTimeMultiSampleArrayModel::setFiffInfo(FiffInfo::SPtr& p_pFiffInfo)
{
    if(p_pFiffInfo)
    {
        RowVectorXi sel;// = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(p_pFiffInfo->bads.size() > 0)
            sel = FiffInfoBase::pick_channels(p_pFiffInfo->ch_names, p_pFiffInfo->bads, emptyExclude);

        m_vecBadIdcs = sel;

        m_pFiffInfo = p_pFiffInfo;

        //Resize data matrix without touching the stored values
        m_matDataRaw.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
        m_matDataRaw.setZero();

        m_matDataFiltered.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
        m_matDataFiltered.setZero();

        m_vecLastBlockFirstValuesFiltered.conservativeResize(m_pFiffInfo->chs.size());
        m_vecLastBlockFirstValuesFiltered.setZero();

        m_vecLastBlockFirstValuesRaw.conservativeResize(m_pFiffInfo->chs.size());
        m_vecLastBlockFirstValuesRaw.setZero();

        //  Create the initial SSP projector
        updateProjection();

        //Initialise filter channel names
        int visibleInit = 20;

        if(visibleInit>m_pFiffInfo->chs.size()) {
            while(visibleInit>m_pFiffInfo->chs.size())
                visibleInit--;
        }

        for(qint32 b = 0; b < visibleInit; ++b)
            m_filterChannelList.append(m_pFiffInfo->ch_names.at(b));
    }
    else {
        m_vecBadIdcs = RowVectorXi(0,0);
        m_matProj = MatrixXd(0,0);
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setSamplingInfo(float sps, int T)
{
    beginResetModel();

    m_iT = T;

    m_iMaxSamples = (qint32)ceil(sps * T);

    //Resize data matrix without touching the stored values
    m_matDataRaw.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
    m_matDataFiltered.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
    m_vecLastBlockFirstValuesRaw.conservativeResize(m_pFiffInfo->chs.size());
    m_vecLastBlockFirstValuesFiltered.conservativeResize(m_pFiffInfo->chs.size());

    emit windowSizeChanged(m_iMaxSamples);

    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::addData(const QList<MatrixXd> &data)
{
    //SSP
    bool doProj = m_bProjActivated && m_matDataRaw.cols() > 0 && m_matDataRaw.rows() == m_matProj.cols() ? true : false;

    //Copy new data into the global data matrix
    for(qint32 b = 0; b < data.size(); ++b) {
        if(data.at(b).rows() != m_matDataRaw.rows()) {
//            std::cout<<"incoming data does not match internal data row size. Returning..."<<std::endl;
            return;
        }

        if(m_iCurrentSample+data.at(b).cols() > m_matDataRaw.cols()) {
            int residual = (m_iCurrentSample+data.at(b).cols()) % m_matDataRaw.cols();
            if(doProj)
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()-residual) = m_matSparseProj * data.at(b).block(0,0,data.at(b).rows(),data.at(b).cols()-residual);
            else
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()-residual) = data.at(b).block(0,0,data.at(b).rows(),data.at(b).cols()-residual);

            //Filter if neccessary
            if(!m_filterData.isEmpty())
                filterChannelsConcurrently(m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()-residual), m_iCurrentSample);

            m_iCurrentSample += data.at(b).cols()-residual;

//            std::cout<<"incoming data exceeds internal data cols by: "<<residual<<std::endl;
//            std::cout<<"m_iCurrentSample+data.at(b).cols(): "<<m_iCurrentSample+data.at(b).cols()<<std::endl;
//            std::cout<<"m_matDataRaw.cols(): "<<m_matDataRaw.cols()<<std::endl;
//            std::cout<<"data.at(b).cols()-residual: "<<data.at(b).cols()-residual<<std::endl<<std::endl;
        } else {
            //std::cout<<"incoming data is ok"<<std::endl;
            if(doProj)
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()) = m_matSparseProj * data.at(b);
            else
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()) = data.at(b);

            //Filter if neccessary
            if(!m_filterData.isEmpty())
                filterChannelsConcurrently(m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()), m_iCurrentSample);

            m_iCurrentSample += data.at(b).cols();
        }
    }

    //Reset m_iCurrentSample and start filling the data matrix from the beginning again
    if(m_iCurrentSample>=m_iMaxSamples) {
        m_iCurrentSample = 0;

        if(!m_bIsFreezed) {
            m_vecLastBlockFirstValuesFiltered = m_matDataFiltered.col(0);
            m_vecLastBlockFirstValuesRaw = m_matDataRaw.col(0);
        }
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
    for(qint32 i = 0; i < selection.size(); ++i) {
        if(selection[i] < m_qListChInfo.size()) {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::hideRows(const QList<qint32> &selection)
{
    beginResetModel();

    for(qint32 i = 0; i < selection.size(); ++i) {
        if(m_qMapIdxRowSelection.contains(selection.at(i)))
            m_qMapIdxRowSelection.remove(selection.at(i));
    }

    emit newSelection(selection);

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

    if(m_bIsFreezed) {
        m_matDataRawFreeze = m_matDataRaw;
        m_matDataFilteredFreeze = m_matDataFiltered;

        m_iCurrentSampleFreeze = m_iCurrentSample;
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_qListChInfo.size()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setScaling(const QMap< qint32,float >& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::updateProjection()
{
    //
    //  Update the SSP projector
    //
    if(m_pFiffInfo)
    {
        m_bProjActivated = false;
        for(qint32 i = 0; i < this->m_pFiffInfo->projs.size(); ++i)
            if(this->m_pFiffInfo->projs[i].active)
                m_bProjActivated = true;

        this->m_pFiffInfo->make_projector(m_matProj);
        qDebug() << "updateProjection :: New projection calculated.";

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j)
            m_matProj.col(m_vecBadIdcs[j]).setZero();

//        std::cout << "Bads\n" << m_vecBadIdcs << std::endl;
//        std::cout << "Proj\n";
//        std::cout << m_matProj.block(0,0,10,10) << std::endl;

        qint32 nchan = this->m_pFiffInfo->nchan;
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

void RealTimeMultiSampleArrayModel::filterChanged(QList<FilterData> filterData)
{
    m_filterData = filterData;

    m_iMaxFilterLength = 0;
    for(int i=0; i<filterData.size(); i++)
        if(m_iMaxFilterLength<filterData.at(i).m_iFilterOrder)
            m_iMaxFilterLength = filterData.at(i).m_iFilterOrder;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setFilterChannelType(QString channelType)
{
    m_sFilterChannelType = channelType;

    createFilterChannelList(m_filterChannelList);
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::createFilterChannelList(QStringList channelNames)
{
    m_filterChannelList.clear();

    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH) && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All" && channelNames.contains(m_pFiffInfo->chs.at(i).ch_name))
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType) && channelNames.contains(m_pFiffInfo->chs.at(i).ch_name))
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
        }
    }
}


//*************************************************************************************************************

void doFilterPerChannel(QPair<QList<FilterData>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i=0; i<channelDataTime.first.size(); i++)
        channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, false, FilterData::MirrorData);
        //channelDataTime.second.second = channelDataTime.first.at(i).applyFFTFilter(channelDataTime.second.second, false, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::filterChannelsConcurrently()
{
    //std::cout<<"START RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;

    for(qint32 i=0; i<m_matDataRaw.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            RowVectorXd data(m_matDataRaw.cols()+2*m_iMaxFilterLength);

            //Only append needed amount (filterLength) to the data
            data << m_matDataRaw.row(i).tail(m_iMaxFilterLength), m_matDataRaw.row(i), m_matDataRaw.row(i).tail(m_iMaxFilterLength).reverse();

            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,data)));
        }
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannel);

        future.waitForFinished();

//        int r = m_iMaxFilterLength;
//        if(matDataLast.rows() == 0)
//            r = 0;

        for(int r = 0; r<timeData.size(); r++)
            m_matDataFiltered.row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength, m_matDataRaw.cols());
    }

//    std::cout<<"m_dataCurrent.size(): "<<m_dataCurrent.size()<<std::endl;
//    std::cout<<"m_dataLast.size(): "<<m_dataLast.size()<<std::endl;

    //std::cout<<"END RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::filterChannelsConcurrently(const MatrixXd &data, int dataIndex)
{
    //std::cout<<"START RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;

    if(dataIndex >= m_matDataFiltered.cols() || data.cols()<m_iMaxFilterLength)
        return;

    if(data.rows() != m_matDataFiltered.rows()) {
        m_matDataFiltered = m_matDataRaw;
        return;
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;

    for(qint32 i=0; i<data.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            RowVectorXd dataTemp(m_matDataRaw.cols()+2*m_iMaxFilterLength);

            //Only prepend and append the needed amount (filterLength/number of filter taps) to the raw data
            dataTemp << data.row(i).tail(m_iMaxFilterLength), data.row(i), data.row(i).tail(m_iMaxFilterLength).reverse();

            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,dataTemp)));
        }
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannel);

        future.waitForFinished();

//        int r = m_iMaxFilterLength;
//        if(matDataLast.rows() == 0)
//            r = 0;

        //TODO: Implement overlap add method
        for(int r = 0; r<timeData.size(); r++)
            m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex,data.cols()) = timeData.at(r).second.second.segment(m_iMaxFilterLength, data.cols());
    }

//    std::cout<<"m_dataCurrent.size(): "<<m_dataCurrent.size()<<std::endl;
//    std::cout<<"m_dataLast.size(): "<<m_dataLast.size()<<std::endl;

    //std::cout<<"END RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::clearModel()
{
    beginResetModel();

    m_matDataRaw.setZero();
    m_matDataFiltered.setZero();
    m_matDataRawFreeze.setZero();
    m_matDataFilteredFreeze.setZero();
    m_vecLastBlockFirstValuesFiltered.setZero();
    m_vecLastBlockFirstValuesRaw.setZero();

    endResetModel();

    qDebug("RealTimeMultiSampleArrayModel cleared.");

}
