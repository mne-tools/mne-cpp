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
, m_iMaxFilterLength(128)
, m_iCurrentBlockSize(1024)
, m_iResidual(0)
, m_bDrawFilterFront(true)
, m_bTriggerDetectionActive(false)
, m_dTriggerThreshold(0.01)
, m_iDistanceTimerSpacer(1000)
, m_iDetectedTriggers(0)
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

        m_matOverlap.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxFilterLength);

        //  Create the initial SSP projector
        updateProjection();

        //Initialize filter channel names
        int visibleInit = 20;
        QStringList filterChannels;

        if(visibleInit>m_pFiffInfo->chs.size()) {
            while(visibleInit>m_pFiffInfo->chs.size())
                visibleInit--;
        }

        for(qint32 b = 0; b < visibleInit; ++b)
            filterChannels.append(m_pFiffInfo->ch_names.at(b));

        createFilterChannelList(filterChannels);

        //Look for trigger channels and initialise detected trigger map
        QList<int> temp;
        for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
            if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH/* && m_pFiffInfo->chs[i].ch_name == "STI 001"*/)
                m_qMapDetectedTrigger.insert(i, temp);
        }
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

    if(m_iCurrentSample>m_iMaxSamples)
        m_iCurrentSample = 0;

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
            std::cout<<"incoming data does not match internal data row size. Returning..."<<std::endl;
            return;
        }

        //Reset m_iCurrentSample and start filling the data matrix from the beginning again. Also add residual  amount of data to the end of the matrix.
        if(m_iCurrentSample+data.at(b).cols() > m_matDataRaw.cols()) {
            m_iResidual = data.at(b).cols() - ((m_iCurrentSample+data.at(b).cols()) % m_matDataRaw.cols());
            if(m_iResidual == data.at(b).cols())
                m_iResidual = 0;

//            std::cout<<"incoming data exceeds internal data cols by: "<<(m_iCurrentSample+data.at(b).cols()) % m_matDataRaw.cols()<<std::endl;
//            std::cout<<"m_iCurrentSample+data.at(b).cols(): "<<m_iCurrentSample+data.at(b).cols()<<std::endl;
//            std::cout<<"m_matDataRaw.cols(): "<<m_matDataRaw.cols()<<std::endl;
//            std::cout<<"data.at(b).cols()-m_iResidual: "<<data.at(b).cols()-m_iResidual<<std::endl<<std::endl;

            if(doProj)
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), m_iResidual) = m_matSparseProj * data.at(b).block(0,0,data.at(b).rows(),m_iResidual);
            else
                m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), m_iResidual) = data.at(b).block(0,0,data.at(b).rows(),m_iResidual);

            m_iCurrentSample = 0;

            if(!m_bIsFreezed) {
                m_vecLastBlockFirstValuesFiltered = m_matDataFiltered.col(0);
                m_vecLastBlockFirstValuesRaw = m_matDataRaw.col(0);
            }

            //Store old detected triggers
            m_qMapDetectedTriggerOld = m_qMapDetectedTrigger;

            //Clear detected triggers
            if(m_bTriggerDetectionActive) {
                QMutableMapIterator<int,QList<int> > i(m_qMapDetectedTrigger);
                while (i.hasNext()) {
                    i.next();
                    i.value().clear();
                }
            }
        } else
            m_iResidual = 0;

        //std::cout<<"incoming data is ok"<<std::endl;
        if(doProj)
            m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()) = m_matSparseProj * data.at(b);
        else
            m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()) = data.at(b);

        //Filter if neccessary else set to zero
        if(!m_filterData.isEmpty())
            filterChannelsConcurrently(m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()), m_iCurrentSample);
        else
            m_matDataFiltered.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols()).setZero();// = m_matDataRaw.block(0, m_iCurrentSample, data.at(b).rows(), data.at(b).cols());

        m_iCurrentSample += data.at(b).cols();

        m_iCurrentBlockSize = data.at(b).cols();

        //detect the trigger flanks in the trigger channels
        if(m_bTriggerDetectionActive) {
            int iOldDetectedTriggers = m_qMapDetectedTrigger[m_iCurrentTriggerChIndex].size();

            DetectTrigger::detectTriggerFlanksMax(data.at(b), m_qMapDetectedTrigger, m_iCurrentSample-data.at(b).cols(), m_dTriggerThreshold, true);
            //DetectTrigger::detectTriggerFlanksGrad(data.at(b), m_qMapDetectedTrigger, m_iCurrentSample-data.at(b).cols());

            //Compute newly counted triggers
            int newTriggers = m_qMapDetectedTrigger[m_iCurrentTriggerChIndex].size() - iOldDetectedTriggers;

            std::cout<<"iOldDetectedTriggers: "<<iOldDetectedTriggers<<std::endl;
            std::cout<<"newTriggers: "<<newTriggers<<std::endl;

            if(newTriggers!=0) {
                m_iDetectedTriggers += newTriggers;
                emit triggerDetected(m_iDetectedTriggers);
            }
            std::cout<<"m_iDetectedTriggers: "<<m_iDetectedTriggers<<std::endl;
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
        m_qMapDetectedTriggerFreeze = m_qMapDetectedTrigger;
        m_qMapDetectedTriggerOldFreeze = m_qMapDetectedTriggerOld;

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

    m_iMaxFilterLength = 1;
    for(int i=0; i<filterData.size(); i++)
        if(m_iMaxFilterLength<filterData.at(i).m_iFilterOrder)
            m_iMaxFilterLength = filterData.at(i).m_iFilterOrder;

    m_matOverlap.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxFilterLength);
    m_matOverlap.setZero();

    m_bDrawFilterFront = false;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::filterActivated(bool state)
{
    //Filter all visible data channels at once
    if(state) {
 //       m_bDrawFilterFront = false;
        //filterChannelsConcurrently();
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::setFilterChannelType(QString channelType)
{
    m_sFilterChannelType = channelType;
    m_filterChannelList = m_visibleChannelList;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_filterChannelList.clear();

    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH) && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All")
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType))
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
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

void RealTimeMultiSampleArrayModel::createFilterChannelList(QStringList channelNames)
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
    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH) && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All")
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType))
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
        }
    }

//    m_bDrawFilterFront = false;

//    for(int i = 0; i<m_filterChannelList.size(); i++)
//        std::cout<<m_filterChannelList.at(i).toStdString()<<std::endl;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::markChBad(QModelIndex ch, bool status)
{
    QList<FiffChInfo> chInfolist = m_pFiffInfo->chs;

    if(status) {
        if(!m_pFiffInfo->bads.contains(chInfolist[ch.row()].ch_name))
            m_pFiffInfo->bads.append(chInfolist[ch.row()].ch_name);
        qDebug() << "RawModel:" << chInfolist[ch.row()].ch_name << "marked as bad.";
    }
    else {
        if(m_pFiffInfo->bads.contains(chInfolist[ch.row()].ch_name)) {
            int index = m_pFiffInfo->bads.indexOf(chInfolist[ch.row()].ch_name);
            m_pFiffInfo->bads.removeAt(index);
            qDebug() << "RawModel:" << chInfolist[ch.row()].ch_name << "marked as good.";
        }
    }

    //Update indeices of bad channels (this vector is needed when creating new ssp operators)
    QStringList emptyExclude;
    m_vecBadIdcs = FiffInfoBase::pick_channels(m_pFiffInfo->ch_names, m_pFiffInfo->bads, emptyExclude);

    emit dataChanged(ch,ch);
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::triggerInfoChanged(const QMap<QString, QColor>& colorMap, bool active, QString triggerCh, double threshold)
{
    m_qMapTriggerColor = colorMap;
    m_bTriggerDetectionActive = active;
    m_sCurrentTriggerCh = triggerCh;
    m_dTriggerThreshold = threshold;

    //Find channel index and initialise detected trigger map
    QList<int> temp;
    m_qMapDetectedTrigger.clear();

    for(int i = 0; i<m_pFiffInfo->chs.size(); i++) {
        if(m_pFiffInfo->chs[i].ch_name == m_sCurrentTriggerCh) {
            m_iCurrentTriggerChIndex = i;
            m_qMapDetectedTrigger.insert(i, temp);
            break;
        }
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::distanceTimeSpacerChanged(int value)
{
    m_iDistanceTimerSpacer = value;
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::resetTriggerCounter()
{
    m_iDetectedTriggers = 0;
}



//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::markChBad(QModelIndexList chlist, bool status)
{
    QList<FiffChInfo> chInfolist = m_pFiffInfo->chs;

    for(int i=0; i < chlist.size(); ++i) {
        if(status) {
            if(!m_pFiffInfo->bads.contains(chInfolist[chlist[i].row()].ch_name))
                m_pFiffInfo->bads.append(chInfolist[chlist[i].row()].ch_name);
            qDebug() << "RawModel:" << chInfolist[chlist[i].row()].ch_name << "marked as bad.";
        }
        else {
            if(m_pFiffInfo->bads.contains(chInfolist[chlist[i].row()].ch_name)) {
                int index = m_pFiffInfo->bads.indexOf(chInfolist[chlist[i].row()].ch_name);
                m_pFiffInfo->bads.removeAt(index);
                qDebug() << "RawModel:" << chInfolist[chlist[i].row()].ch_name << "marked as good.";
            }
        }

        emit dataChanged(chlist[i],chlist[i]);
    }

    //Update indeices of bad channels (this vector is needed when creating new ssp operators)
    QStringList emptyExclude;
    m_vecBadIdcs = FiffInfoBase::pick_channels(m_pFiffInfo->ch_names, m_pFiffInfo->bads, emptyExclude);
}


//*************************************************************************************************************

void doFilterPerChannelRTMSA(QPair<QList<FilterData>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i=0; i<channelDataTime.first.size(); i++)
        //channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, true, FilterData::ZeroPad);
        channelDataTime.second.second = channelDataTime.first.at(i).applyFFTFilter(channelDataTime.second.second, true, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayModel::filterChannelsConcurrently()
{
    //std::cout<<"START RealTimeMultiSampleArrayModel::filterChannelsConcurrently"<<std::endl;

    if(m_filterData.isEmpty())
        return;

    //Create temporary filters with higher fft length because we are going to filter all available data at once for one time
    QList<FilterData> tempFilterList;

    int fftLength = m_matDataRaw.row(0).cols() + 4 * m_iMaxFilterLength;
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
    for(qint32 i=0; i<m_matDataRaw.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            RowVectorXd datTemp(m_matDataRaw.row(i).cols() + 2 * m_iMaxFilterLength);
            datTemp << m_matDataRaw.row(i).head(m_iMaxFilterLength).reverse(), m_matDataRaw.row(i), m_matDataRaw.row(i).tail(m_iMaxFilterLength).reverse();
            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(tempFilterList,QPair<int,RowVectorXd>(i,datTemp)));
        }
        else
            notFilterChannelIndex.append(i);
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTMSA);

        future.waitForFinished();

        for(int r = 0; r<timeData.size(); r++) {
            m_matDataFiltered.row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength+m_iMaxFilterLength/2, m_matDataRaw.cols());
            m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
        }
    }

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i<notFilterChannelIndex.size(); i++)
        m_matDataFiltered.row(notFilterChannelIndex.at(i)) = m_matDataRaw.row(notFilterChannelIndex.at(i));

    if(!m_bIsFreezed) {
        m_vecLastBlockFirstValuesFiltered = m_matDataFiltered.col(0);
    }

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
    QList<int> notFilterChannelIndex;

    for(qint32 i=0; i<data.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name))
            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,data.row(i))));
        else
            notFilterChannelIndex.append(i);
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTMSA);

        future.waitForFinished();

        //Do the overlap add method and store in m_matDataFiltered
        int iFilterDelay = m_iMaxFilterLength/2;
        int iFilteredNumberCols = timeData.at(0).second.second.cols();

        for(int r = 0; r<timeData.size(); r++) {
            if(m_iCurrentSample+2*data.cols() > m_matDataRaw.cols()) {
                //Handle last data block
                //std::cout<<"Handle last data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);

                    //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
                    int start = dataIndex-iFilterDelay < 0 ? 0 : dataIndex-iFilterDelay;
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(start,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            } else if(m_iCurrentSample == 0) {
                //Handle first data block
                //std::cout<<"Handle first data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Add newly calculate data to the tail of the current filter data matrix
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(m_matDataFiltered.cols()-iFilterDelay-m_iResidual, iFilterDelay) = tempData.head(iFilterDelay) + m_matOverlap.row(timeData.at(r).second.first).head(iFilterDelay);

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);
                    m_matDataFiltered.row(timeData.at(r).second.first).head(iFilteredNumberCols-m_iMaxFilterLength-iFilterDelay) = tempData.segment(iFilterDelay,iFilteredNumberCols-m_iMaxFilterLength-iFilterDelay);

                    //Copy residual data from the front to the back. The residual is != 0 if the chosen block size cannot be evenly fit into the matrix size
                    m_matDataFiltered.row(timeData.at(r).second.first).tail(m_iResidual) = m_matDataFiltered.row(timeData.at(r).second.first).head(m_iResidual);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).head(m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            } else {
                //Handle middle data blocks
                //std::cout<<"Handle middle data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);

                    //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex-iFilterDelay,m_iMaxFilterLength).setZero();// = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(dataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            }
        }
    }

    m_bDrawFilterFront = true;

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i<notFilterChannelIndex.size(); i++)
        m_matDataFiltered.row(notFilterChannelIndex.at(i)).segment(dataIndex,data.row(notFilterChannelIndex.at(i)).cols()) = data.row(notFilterChannelIndex.at(i));

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
    m_matOverlap.setZero();

    endResetModel();

    qDebug("RealTimeMultiSampleArrayModel cleared.");

}
