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

void RealTimeMultiSampleArrayModel::setFiffInfo(FiffInfo::SPtr& p_pFiffInfo)
{
    if(p_pFiffInfo)
    {
        RowVectorXi sel = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(p_pFiffInfo->bads.size() > 0)
            sel = FiffInfoBase::pick_channels(p_pFiffInfo->ch_names, p_pFiffInfo->bads, emptyExclude);

        m_vecBadIdcs = sel;

        this->m_pFiffInfo = p_pFiffInfo;

        //
        //  Create the initial SSP projector
        //
        updateProjection();
    }
    else
    {
        m_vecBadIdcs = RowVectorXi(0,0);
        m_matProj = MatrixXd(0,0);
    }
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

void RealTimeMultiSampleArrayModel::addData(const QList<MatrixXd> &data)
{
    //Downsampling ->ToDo make this more accurate

    for(qint32 b = 0; b < data.size(); ++b)
    {        
        qint32 i;
        qint32 count = 0;
        //Downsample the data
        for(i = m_iCurrentSample; i < data[b].cols(); i += m_iDownsampling)
            ++count;

        MatrixXd dsData(data[b].rows(),count);

        count = 0;
        for(i = m_iCurrentSample; i < data[b].cols(); i += m_iDownsampling)
        {
            dsData.col(count) = data[b].col(i);
            ++count;
        }


//        qDebug() << "nchan" << dsData.rows() << "proj rows" << m_matProj.rows();


        //store for next buffer
        m_iCurrentSample = i - data[b].cols();

//            m_dataCurrent.append(data[b].col(i));

        bool doProj = false;

        if(m_bProjActivated && dsData.cols() > 0 && dsData.rows() == m_matProj.cols())
            doProj = true;

        //SSP
        if(doProj)
        {
            //set bad channels to zero
            for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j)
                dsData.row(m_vecBadIdcs[j]).setZero();

            //Do SSP Projection
            MatrixXd projDsData = m_matSparseProj * dsData;
            for(i = 0; i < projDsData.cols(); ++i)
                m_dataCurrent.append(projDsData.col(i));
        }
        else
        {
            //store data
            for(i = 0; i < dsData.cols(); ++i)
                m_dataCurrent.append(dsData.col(i));
        }

        // - old -
//        //SSP
//        //set bad channels to zero
//        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j)
//            m_dataCurrent.last()[m_vecBadIdcs[j]] = 0;

//        //apply projector
//        if(doProj)
//            m_dataCurrent.last() = m_matProj * m_dataCurrent.last();

//        //Downsampling
//        for(i = m_iCurrentSample; i < data[b].cols(); i += m_iDownsampling)
//            m_dataCurrent.append(data[b].col(i));

//        //store for next buffer
//        m_iCurrentSample = i - data[b].cols();
    }

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

//        std::cout << "Bads\n" << m_vecBadIdcs << std::endl;
//        std::cout << "Proj\n";
//        std::cout << m_matProj.block(0,0,10,10) << std::endl;

        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        qDebug() << "Channel names" << this->m_pFiffInfo->ch_names;

        qDebug() << "Proj row names" << this->m_pFiffInfo->projs[0].data->row_names;

        qDebug() << "Proj col names" << this->m_pFiffInfo->projs[0].data->col_names;
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
