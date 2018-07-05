//=============================================================================================================
/**
* @file     evokedmodel.cpp
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
* @brief    Definition of the EvokedModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "evokedmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QBrush>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EvokedModel::EvokedModel(QObject *parent)
: QAbstractTableModel(parent)
, m_matData(MatrixXd(0,0))
, m_matDataFreeze(MatrixXd(0,0))
, m_fSps(1024.0f)
, m_bIsFreezed(false)
, m_bProjActivated(false)
, m_bCompActivated(false)
, m_bIsInit(false)
, m_iMaxFilterLength(100)
{

}


//*************************************************************************************************************

EvokedModel::~EvokedModel()
{
    std::cout<<"EvokedModel::~EvokedModel"<<std::endl;
}


//*************************************************************************************************************
//virtual functions
int EvokedModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qMapIdxRowSelection.empty()) {
        return m_qMapIdxRowSelection.size();
    }

    return 0;
}


//*************************************************************************************************************

int EvokedModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant EvokedModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != EvokedModelRoles::GetAverageData) {
        return QVariant();
    }

    if(index.isValid()) {
        qint32 row = m_qMapIdxRowSelection[index.row()];

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole) {
            return QVariant(m_pRTE->info()->ch_names[row]);
        }

        //******** second column (butterfly data plot) ********
        //TODO: Bring this into a better structure see colum three
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList
                    RowVectorXd rowVec;

                    if(m_bIsFreezed) {
                        // data freeze
                        if(m_filterData.isEmpty()) {
                            rowVec = m_matDataFreeze.row(row);
                        } else {
                            rowVec = m_matDataFilteredFreeze.row(row);
                        }
                    } else {
                        // data stream
                        if(m_filterData.isEmpty()) {
                            rowVec = m_matData.row(row);
                        } else {
                            rowVec = m_matDataFiltered.row(row);
                        }
                    }

                    v.setValue(rowVec);

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

        //******** third column (2D layout data plot, this third column is needed because the te data needs to be in another strucutre for the 2D layout to work) ********
        if(index.column()==2) {
            QVariant v;
            RowVectorPair averagedData;

            switch(role) {
                case EvokedModelRoles::GetAverageData: {
                    if(m_bIsFreezed){
                        // data freeze
                        if(m_filterData.isEmpty()) {
                            averagedData.first = m_matDataFreeze.data();
                            averagedData.second = m_matDataFreeze.cols();
                        } else {
                            averagedData.first = m_matDataFilteredFreeze.data();
                            averagedData.second = m_matDataFilteredFreeze.cols();
                        }

                        v.setValue(averagedData);
                    }
                    else {
                        // data
                        if(m_filterData.isEmpty()) {
                            averagedData.first = m_matData.data();
                            averagedData.second = m_matData.cols();
                        } else {
                            averagedData.first = m_matDataFiltered.data();
                            averagedData.second = m_matDataFiltered.cols();
                        }

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

QVariant EvokedModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
        return QVariant();
    }

    if(orientation == Qt::Horizontal) {
        switch(section) {
        case 0: //chname column
            return QVariant();
        case 1: //data plot column
            switch(role) {
            case Qt::DisplayRole:
                return QVariant("data plot");
            case Qt::TextAlignmentRole:
                return QVariant(Qt::AlignLeft);
            }
            return QVariant("data plot");
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

void EvokedModel::setRTE(QSharedPointer<RealTimeEvoked> &pRTE)
{
    beginResetModel();
    m_pRTE = pRTE;

    //Generate bad channel index list
    RowVectorXi sel;// = RowVectorXi(0,0);
    QStringList emptyExclude;

    if(m_pRTE->info()->bads.size() > 0) {
        sel = FiffInfoBase::pick_channels(m_pRTE->info()->ch_names, m_pRTE->info()->bads, emptyExclude);
    }

    m_vecBadIdcs = sel;

    m_fSps = m_pRTE->info()->sfreq;

    //Create the initial SSP projector
    updateProjection();

    //Init list of channels which are to filtered
    createFilterChannelList(m_pRTE->info()->ch_names);

    endResetModel();

    resetSelection();
}


//*************************************************************************************************************

void EvokedModel::updateData()
{
    if(m_matData.cols() != m_pRTE->getValue()->data.cols()) {
        //init data matrix
        m_matData.conservativeResize(m_pRTE->info()->chs.size(), m_pRTE->getValue()->data.cols());
        m_matData.setZero();
        m_matDataFreeze.conservativeResize(m_pRTE->info()->chs.size(), m_pRTE->getValue()->data.cols());
        m_matDataFreeze.setZero();
        m_matDataFiltered.conservativeResize(m_pRTE->info()->chs.size(), m_pRTE->getValue()->data.cols());
        m_matDataFiltered.setZero();
        m_matDataFilteredFreeze.conservativeResize(m_pRTE->info()->chs.size(), m_pRTE->getValue()->data.cols());
        m_matDataFilteredFreeze.setZero();
    }

    bool doProj = m_bProjActivated && m_matData.cols() > 0 && m_matData.rows() == m_matProj.cols() ? true : false;

    bool doComp = m_bCompActivated && m_matData.cols() > 0 && m_matData.rows() == m_matComp.cols() ? true : false;

    if(doComp) {
        if(doProj) {
            //Comp + Proj
            m_matData = m_matSparseProjCompMult * m_pRTE->getValue()->data;
        } else {
            //Comp
            m_matData = m_matSparseCompMult * m_pRTE->getValue()->data;
        }
    } else {
        if(doProj) {
            //Proj
            m_matData = m_matSparseProjMult * m_pRTE->getValue()->data;
        } else {
            //None - Raw
            m_matData = m_pRTE->getValue()->data;
        }
    }

    m_pairBaseline = m_pRTE->getValue()->baseline;

    if(!m_filterData.isEmpty()) {
        filterChannelsConcurrently();
    }

    m_bIsInit = true;

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_pRTE->info()->nchan-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

QColor EvokedModel::getColor(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->chColor()[chRow];
    }

    return QColor();
}


//*************************************************************************************************************

fiff_int_t EvokedModel::getKind(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].kind;
    }

    return 0;
}


//*************************************************************************************************************

fiff_int_t EvokedModel::getUnit(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].unit;
    }

    return FIFF_UNIT_NONE;
}


//*************************************************************************************************************

fiff_int_t EvokedModel::getCoil(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pRTE->info()->chs[chRow].chpos.coil_type;
    }

    return FIFFV_COIL_NONE;
}


//*************************************************************************************************************

void EvokedModel::selectRows(const QList<qint32> &selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i) {
        if(selection[i] < m_pRTE->info()->nchan) {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}


//*************************************************************************************************************

void EvokedModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_pRTE->info()->nchan; ++i) {
        m_qMapIdxRowSelection.insert(i,i);
    }

    endResetModel();
}


//*************************************************************************************************************

void EvokedModel::setScaling(const QMap< qint32,float >& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}


//*************************************************************************************************************

void EvokedModel::updateProjection()
{
    //
    //  Update the SSP projector
    //
    if(m_pRTE->info()->chs.size()>0) {
        m_bProjActivated = false;
        for(qint32 i = 0; i < m_pRTE->info()->projs.size(); ++i) {
            if(m_pRTE->info()->projs[i].active) {
                m_bProjActivated = true;
            }
        }

        m_pRTE->info()->make_projector(m_matProj);
        qDebug() << "updateProjection :: New projection calculated :: m_bProjActivated is "<<m_bProjActivated;

        //set columns of matrix to zero depending on bad channels indexes
        RowVectorXi sel;// = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(m_pRTE->info()->bads.size() > 0) {
            sel = FiffInfoBase::pick_channels(m_pRTE->info()->ch_names, m_pRTE->info()->bads, emptyExclude);
        }

        m_vecBadIdcs = sel;

        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j) {
            m_matProj.col(m_vecBadIdcs[j]).setZero();
        }

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
        for(i = 0; i < m_matProj.rows(); ++i) {
            for(k = 0; k < m_matProj.cols(); ++k) {
                if(m_matProj(i,k) != 0) {
                    tripletList.push_back(T(i, k, m_matProj(i,k)));
                }
            }
        }

        m_matSparseProjMult = SparseMatrix<double>(m_matProj.rows(),m_matProj.cols());
        if(tripletList.size() > 0) {
            m_matSparseProjMult.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;
    }
}


//*************************************************************************************************************

void EvokedModel::updateCompensator(int to)
{
    //
    //  Update the compensator
    //
    if(m_pRTE->info())
    {
        if(to == 0) {
            m_bCompActivated = false;
        } else {
            m_bCompActivated = true;
        }

//        qDebug()<<"to"<<to;
//        qDebug()<<"from"<<from;
//        qDebug()<<"m_bCompActivated"<<m_bCompActivated;

        FiffCtfComp newComp;
        m_pRTE->info()->make_compensator(0, to, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data

        //We do not need to call this->m_pFiffInfo->set_current_comp(to);
        //Because we will set the compensators to the coil in the same FiffInfo which is already used to write to file.
        //Note that the data is written in raw form not in compensated form.
        m_matComp = newComp.data->data;

        //
        // Make proj sparse
        //
        qint32 nchan = m_pRTE->info()->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        tripletList.clear();
        tripletList.reserve(m_matComp.rows()*m_matComp.cols());
        for(i = 0; i < m_matComp.rows(); ++i) {
            for(k = 0; k < m_matComp.cols(); ++k) {
                if(m_matComp(i,k) != 0) {
                    tripletList.push_back(T(i, k, m_matComp(i,k)));
                }
            }
        }

        m_matSparseCompMult = SparseMatrix<double>(m_matComp.rows(),m_matComp.cols());
        if(tripletList.size() > 0) {
            m_matSparseCompMult.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Create full multiplication matrix
        m_matSparseProjCompMult = m_matSparseProjMult * m_matSparseCompMult;
    }
}


//*************************************************************************************************************

void EvokedModel::toggleFreeze()
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

void EvokedModel::filterChanged(QList<FilterData> filterData)
{
    m_filterData = filterData;

    m_iMaxFilterLength = 1;
    for(int i=0; i<filterData.size(); i++) {
        if(m_iMaxFilterLength<filterData.at(i).m_iFilterOrder) {
            m_iMaxFilterLength = filterData.at(i).m_iFilterOrder;
        }
    }

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void EvokedModel::setFilterChannelType(QString channelType)
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
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            } else if(m_pRTE->info()->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            }
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

void EvokedModel::createFilterChannelList(QStringList channelNames)
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
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            } else if(m_pRTE->info()->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pRTE->info()->chs.at(i).ch_name;
            }
        }
    }

//    for(int i = 0; i<m_filterChannelList.size(); i++)
//        std::cout<<m_filterChannelList.at(i).toStdString()<<std::endl;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void doFilterPerChannelRTE(QPair<QList<FilterData>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i=0; i<channelDataTime.first.size(); i++) {
        //channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, true, FilterData::ZeroPad);
        channelDataTime.second.second = channelDataTime.first.at(i).applyFFTFilter(channelDataTime.second.second, true, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
    }
}


//*************************************************************************************************************

void EvokedModel::filterChannelsConcurrently()
{    
    //std::cout<<"START EvokedModel::filterChannelsConcurrently()"<<std::endl;

    if(m_filterData.isEmpty()) {
        return;
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;
    QList<int> notFilterChannelIndex;

    //Also append mirrored data in front and back to get rid of edge effects
    for(qint32 i=0; i<m_matData.rows(); ++i) {
        if(m_filterChannelList.contains(m_pRTE->info()->chs.at(i).ch_name)) {
            RowVectorXd datTemp(m_matData.row(i).cols() + 2 * m_iMaxFilterLength);
            datTemp << m_matData.row(i).head(m_iMaxFilterLength).reverse(), m_matData.row(i), m_matData.row(i).tail(m_iMaxFilterLength).reverse();
            timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,datTemp)));
        } else {
            notFilterChannelIndex.append(i);
        }
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTE);

        future.waitForFinished();

        for(int r = 0; r<timeData.size(); r++) {
            m_matDataFiltered.row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength+m_iMaxFilterLength/2, m_matData.cols());
        }
    }

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i<notFilterChannelIndex.size(); i++) {
        m_matDataFiltered.row(notFilterChannelIndex.at(i)) = m_matData.row(notFilterChannelIndex.at(i));
    }

    //std::cout<<"END EvokedModel::filterChannelsConcurrently()"<<std::endl;
}

