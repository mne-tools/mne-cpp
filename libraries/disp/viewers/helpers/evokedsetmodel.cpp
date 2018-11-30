//=============================================================================================================
/**
* @file     evokedsetmodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the EvokedSetModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "evokedsetmodel.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtConcurrent>
#include <QFuture>
#include <QPair>
#include <QColor>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EvokedSetModel::EvokedSetModel(QObject *parent)
: QAbstractTableModel(parent)
, m_fSps(1024.0f)
, m_bIsFreezed(false)
, m_bProjActivated(false)
, m_bCompActivated(false)
, m_bIsInit(false)
, m_iMaxFilterLength(100)
, m_qMapAverageColor(QSharedPointer<QMap<QString, QColor> >::create())
, m_qMapAverageActivation(QSharedPointer<QMap<QString, bool> >::create())
, m_qMapAverageColorOld(QSharedPointer<QMap<QString, QColor> >::create())
, m_qMapAverageActivationOld(QSharedPointer<QMap<QString, bool> >::create())
{
}


//*************************************************************************************************************

EvokedSetModel::~EvokedSetModel()
{
}


//*************************************************************************************************************
//virtual functions
int EvokedSetModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qMapIdxRowSelection.empty()) {
        return m_qMapIdxRowSelection.size();
    }

    return 0;
}


//*************************************************************************************************************

int EvokedSetModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant EvokedSetModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole && role != EvokedSetModelRoles::GetAverageData) {
        return QVariant();
    }

    if(index.isValid()) {
        qint32 row = m_qMapIdxRowSelection[index.row()];

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole) {
            return QVariant(m_pEvokedSet->info.ch_names);
        }

        //******** second column (butterfly data plot) ********
        //TODO: Bring this into a better structure see colum three
        if(index.column()==1) {
            QVariant v;

            QList<DISPLIB::AvrTypeRowVector> lRowDataPerTrigType;
            DISPLIB::AvrTypeRowVector pairItem;

            switch(role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList

                    if(m_bIsFreezed) {
                        // data freeze
                        if(m_filterData.isEmpty()) {
                            for(int i = 0; i < m_matDataFreeze.size(); ++i) {
                                pairItem.first = m_lAvrTypes.at(i);
                                pairItem.second = m_matDataFreeze.at(i).row(row);
                                lRowDataPerTrigType.append(pairItem);
                            }
                        } else {
                            for(int i = 0; i < m_matDataFilteredFreeze.size(); ++i) {
                                pairItem.first = m_lAvrTypes.at(i);
                                pairItem.second = m_matDataFilteredFreeze.at(i).row(row);
                                lRowDataPerTrigType.append(pairItem);
                            }
                        }
                    } else {
                        // data stream
                        if(m_filterData.isEmpty()) {
                            for(int i = 0; i < m_matData.size(); ++i) {
                                pairItem.first = m_lAvrTypes.at(i);
                                pairItem.second = m_matData.at(i).row(row);
                                lRowDataPerTrigType.append(pairItem);
                            }
                        } else {
                            for(int i = 0; i < m_matDataFiltered.size(); ++i) {
                                pairItem.first = m_lAvrTypes.at(i);
                                pairItem.second = m_matDataFiltered.at(i).row(row);
                                lRowDataPerTrigType.append(pairItem);
                            }
                        }
                    }

                    v.setValue(lRowDataPerTrigType);

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

        //******** third column (2D layout data plot, this third column is needed because the te data needs to be in another structure for the 2D layout to work) ********
        if(index.column()==2) {
            QVariant v;
            QList<DISPLIB::AvrTypeRowVectorPair> lRowDataPerTrigType;
            DISPLIB::AvrTypeRowVectorPair averagedData;

            switch(role) {
                case EvokedSetModelRoles::GetAverageData: {
                    if(m_bIsFreezed){
                        // data freeze
                        if(m_filterData.isEmpty()) {
                            for(int i = 0; i < m_matDataFreeze.size(); ++i) {
                                averagedData.first = m_lAvrTypes.at(i);
                                averagedData.second.first = m_matDataFreeze.at(i).data();
                                averagedData.second.second = m_matDataFreeze.at(i).cols();

                                lRowDataPerTrigType.append(averagedData);
                            }
                        } else {
                            for(int i = 0; i < m_matDataFilteredFreeze.size(); ++i) {
                                averagedData.first = m_lAvrTypes.at(i);
                                averagedData.second.first = m_matDataFilteredFreeze.at(i).data();
                                averagedData.second.second = m_matDataFilteredFreeze.at(i).cols();

                                lRowDataPerTrigType.append(averagedData);
                            }
                        }
                    } else {
                        // data
                        if(m_filterData.isEmpty()) {
                            for(int i = 0; i < m_matData.size(); ++i) {
                                averagedData.first = m_lAvrTypes.at(i);
                                averagedData.second.first = m_matData.at(i).data();
                                averagedData.second.second = m_matData.at(i).cols();

                                lRowDataPerTrigType.append(averagedData);
                            }
                        } else {
                            for(int i = 0; i < m_matDataFiltered.size(); ++i) {
                                averagedData.first = m_lAvrTypes.at(i);
                                averagedData.second.first = m_matDataFiltered.at(i).data();
                                averagedData.second.second = m_matDataFiltered.at(i).cols();

                                lRowDataPerTrigType.append(averagedData);
                            }
                        }
                    }

                    v.setValue(lRowDataPerTrigType);
                }
            }

            return v;
        }//end column check

    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

QVariant EvokedSetModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void EvokedSetModel::setEvokedSet(QSharedPointer<FiffEvokedSet> pEvokedSet)
{
    m_pEvokedSet = pEvokedSet;

    if(!m_bIsInit) {
        init();
    }

    updateData();
}


//*************************************************************************************************************

void EvokedSetModel::init()
{
    if(!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    //Generate bad channel index list
    RowVectorXi sel;// = RowVectorXi(0,0);
    QStringList emptyExclude;

    if(m_pEvokedSet->info.bads.size() > 0) {
        sel = FiffInfoBase::pick_channels(m_pEvokedSet->info.ch_names, m_pEvokedSet->info.bads, emptyExclude);
    }

    m_vecBadIdcs = sel;

    m_fSps = m_pEvokedSet->info.sfreq;

    m_matSparseProjMult = SparseMatrix<double>(m_pEvokedSet->info.chs.size(),m_pEvokedSet->info.chs.size());
    m_matSparseCompMult = SparseMatrix<double>(m_pEvokedSet->info.chs.size(),m_pEvokedSet->info.chs.size());
    m_matSparseProjCompMult = SparseMatrix<double>(m_pEvokedSet->info.chs.size(),m_pEvokedSet->info.chs.size());

    m_matSparseProjMult.setIdentity();
    m_matSparseCompMult.setIdentity();
    m_matSparseProjCompMult.setIdentity();

    m_qMapAverageActivation->clear();
    m_qMapAverageColor->clear();
    m_qMapAverageActivationOld->clear();
    m_qMapAverageColorOld->clear();

    //Create the initial SSP projector
    updateProjection(m_pEvokedSet->info.projs);

    //Create the initial Compensator projector
    updateCompensator(0);

    //Init list of channels which are to filtered
    createFilterChannelList(m_pEvokedSet->info.ch_names);

    endResetModel();

    resetSelection();

    m_bIsInit = true;
}


//*************************************************************************************************************

void EvokedSetModel::updateData()
{
    if(!m_pEvokedSet) {
        return;
    }

    m_matData.clear();
    m_matDataFiltered.clear();
    m_lAvrTypes.clear();

    for(int i = 0; i < m_pEvokedSet->evoked.size(); ++i) {
        bool doProj = m_bProjActivated && m_pEvokedSet->evoked.at(i).data.cols() > 0 && m_pEvokedSet->evoked.at(i).data.rows() == m_matProj.cols() ? true : false;

        bool doComp = m_bCompActivated && m_pEvokedSet->evoked.at(i).data.cols() > 0 && m_pEvokedSet->evoked.at(i).data.rows() == m_matComp.cols() ? true : false;

        if(doComp) {
            if(doProj) {
                //Comp + Proj
                m_matData.append(m_matSparseProjCompMult * m_pEvokedSet->evoked.at(i).data);
            } else {
                //Comp
                m_matData.append(m_matSparseCompMult * m_pEvokedSet->evoked.at(i).data);
            }
        } else {
            if(doProj) {
                //Proj
                m_matData.append(m_matSparseProjMult * m_pEvokedSet->evoked.at(i).data);
            } else {
                //None - Raw
                m_matData.append(m_pEvokedSet->evoked.at(i).data);
            }
        }

        m_matDataFiltered.append(MatrixXd::Zero(m_pEvokedSet->evoked.at(i).data.rows(), m_pEvokedSet->evoked.at(i).data.cols()));

        m_pairBaseline = m_pEvokedSet->evoked.at(i).baseline;

        m_lAvrTypes.append(m_pEvokedSet->evoked.at(i).comment);
    }

    if(!m_filterData.isEmpty()) {
        filterChannelsConcurrently();
    }

    // Update average selection information map. Use old colors if existing.
    QStringList slCurrentAvrComments;
    int iSizeAvrActivation = m_qMapAverageActivation->size();
    int iSizeAvrColor = m_qMapAverageColor->size();

    for(int i = 0; i < m_pEvokedSet->evoked.size(); ++i) {
        slCurrentAvrComments << m_pEvokedSet->evoked.at(i).comment;

        if(!m_qMapAverageActivation->contains(m_pEvokedSet->evoked.at(i).comment)) {
            if(m_qMapAverageActivationOld->contains(m_pEvokedSet->evoked.at(i).comment)) {
                qDebug() << "EvokedSetModel::updateData m_qMapAverageActivationOld exists for" << m_pEvokedSet->evoked.at(i).comment << m_qMapAverageActivationOld->value(m_pEvokedSet->evoked.at(i).comment);
                m_qMapAverageActivation->insert(m_pEvokedSet->evoked.at(i).comment, m_qMapAverageActivationOld->value(m_pEvokedSet->evoked.at(i).comment));
            } else {
                qDebug() << "EvokedSetModel::updateData m_qMapAverageActivationOld does not exists for" << m_pEvokedSet->evoked.at(i).comment;
                m_qMapAverageActivation->insert(m_pEvokedSet->evoked.at(i).comment, true);
            }
        }

        if(!m_qMapAverageColor->contains(m_pEvokedSet->evoked.at(i).comment)) {
            if(m_qMapAverageColorOld->contains(m_pEvokedSet->evoked.at(i).comment)) {
                m_qMapAverageColor->insert(m_pEvokedSet->evoked.at(i).comment, m_qMapAverageColorOld->value(m_pEvokedSet->evoked.at(i).comment));
            } else {
                m_qMapAverageColor->insert(m_pEvokedSet->evoked.at(i).comment, Qt::yellow);
            }
        }
    }

    // Delete average color and activation if they are no longer present in the evoked set
    QMutableMapIterator<QString, bool> itrActivation(*m_qMapAverageActivation);
    while(itrActivation.hasNext()) {
        itrActivation.next();
        if(!slCurrentAvrComments.contains(itrActivation.key())) {
            qDebug() << "EvokedSetModel::updateData Adding to m_qMapAverageActivationOld" << itrActivation.key();
            m_qMapAverageActivationOld->insert(itrActivation.key(),itrActivation.value());
            itrActivation.remove();
        }
    }

    QMutableMapIterator<QString, QColor> itrColor(*m_qMapAverageColor);
    while(itrColor.hasNext()) {
        itrColor.next();
        if(!slCurrentAvrComments.contains(itrColor.key())) {
            qDebug() << "EvokedSetModel::updateData Adding to m_qMapAverageColorOld" << itrColor.key();
            m_qMapAverageColorOld->insert(itrColor.key(),itrColor.value());
            itrColor.remove();
        }
    }

    // Only emit new colors and activations if evoked types were added or deleted
    if(iSizeAvrColor != m_qMapAverageColor->size()) {
        emit newAverageColorMap(m_qMapAverageColor);
    }

    if(iSizeAvrActivation != m_qMapAverageActivation->size()) {
        emit newAverageActivationMap(m_qMapAverageActivation);
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_pEvokedSet->info.nchan-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;

    emit dataChanged(topLeft, bottomRight, roles);
}


//*************************************************************************************************************

QSharedPointer<QMap<QString, QColor> > EvokedSetModel::getAverageColor() const
{
    return m_qMapAverageColor;
}


//*************************************************************************************************************

QSharedPointer<QMap<QString, bool> > EvokedSetModel::getAverageActivation() const
{
    return m_qMapAverageActivation;
}


//*************************************************************************************************************

void EvokedSetModel::setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
}


//*************************************************************************************************************

void EvokedSetModel::setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
}


//*************************************************************************************************************

fiff_int_t EvokedSetModel::getKind(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pEvokedSet->info.chs[chRow].kind;
    }

    return 0;
}


//*************************************************************************************************************

fiff_int_t EvokedSetModel::getUnit(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pEvokedSet->info.chs[chRow].unit;
    }

    return FIFF_UNIT_NONE;
}


//*************************************************************************************************************

fiff_int_t EvokedSetModel::getCoil(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pEvokedSet->info.chs[chRow].chpos.coil_type;
    }

    return FIFFV_COIL_NONE;
}


//*************************************************************************************************************

bool EvokedSetModel::isInit() const
{
    return m_bIsInit;
}


//*************************************************************************************************************

qint32 EvokedSetModel::getNumSamples() const
{
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return m_bIsInit ? iNumSamples : 0;
}


//*************************************************************************************************************

QVariant EvokedSetModel::data(int row, int column, int role) const
{
    return data(index(row, column), role);
}


//*************************************************************************************************************

const QMap<qint32,qint32>& EvokedSetModel::getIdxSelMap() const
{
    return m_qMapIdxRowSelection;
}


//*************************************************************************************************************

qint32 EvokedSetModel::numVLines() const
{
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return (qint32)(iNumSamples/m_fSps) - 1;
}


//*************************************************************************************************************

qint32 EvokedSetModel::getNumPreStimSamples() const
{
    int iPreSamples = 0;

    if(!m_pEvokedSet) {
        return iPreSamples;
    }

    if (!m_pEvokedSet->evoked.isEmpty()) {
        RowVectorXf times = m_pEvokedSet->evoked.first().times;

        // Search for stim onset via times
        for(int i = 0; i < times.cols(); i++) {
            if(times(i) == 0.0f) {
                break;
            }

            iPreSamples++;
        }
    }

    return iPreSamples;
}


//*************************************************************************************************************

float EvokedSetModel::getSamplingFrequency() const
{
    return m_fSps;
}


//*************************************************************************************************************

bool EvokedSetModel::isFreezed() const
{
    return m_bIsFreezed;
}


//*************************************************************************************************************

int EvokedSetModel::getNumberOfTimeSpacers() const
{
    //std::cout<<floor((m_matData.cols()/m_fSps)*10)<<std::endl;
    qint32 iNumSamples = 0;

    if(!m_matData.isEmpty()) {
        iNumSamples = m_matData.first().cols();
    }

    return floor((iNumSamples/m_fSps)*10);
}


//*************************************************************************************************************

QPair<QVariant,QVariant> EvokedSetModel::getBaselineInfo() const
{
    //qDebug()<<floor((m_matData.cols()/m_fSps)*10);
    return m_pairBaseline;
}


//*************************************************************************************************************

int EvokedSetModel::getNumAverages() const
{
    return m_matData.size();
}


//*************************************************************************************************************

void EvokedSetModel::selectRows(const QList<qint32> &selection)
{
    if(!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i) {
        if(selection[i] < m_pEvokedSet->info.nchan) {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}


//*************************************************************************************************************

void EvokedSetModel::resetSelection()
{
    if(!m_pEvokedSet) {
        return;
    }

    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_pEvokedSet->info.nchan; ++i) {
        m_qMapIdxRowSelection.insert(i,i);
    }

    endResetModel();
}


//*************************************************************************************************************

void EvokedSetModel::updateProjection(const QList<FiffProj>& projs)
{
    if(!m_pEvokedSet) {
        return;
    }

    // Update the SSP projector
    if(m_pEvokedSet->info.chs.size() > 0) {
        m_pEvokedSet->info.projs = projs;
        m_bProjActivated = false;
        for(qint32 i = 0; i < projs.size(); ++i) {
            if(m_pEvokedSet->info.projs[i].active) {
                m_bProjActivated = true;
            }
        }

        m_pEvokedSet->info.make_projector(m_matProj);
        //qDebug() << "EvokedSetModel::updateProjection - New projection calculated. m_bProjActivated is "<<m_bProjActivated;

        //set columns of matrix to zero depending on bad channels indexes
        RowVectorXi sel;// = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(m_pEvokedSet->info.bads.size() > 0) {
            sel = FiffInfoBase::pick_channels(m_pEvokedSet->info.ch_names, m_pEvokedSet->info.bads, emptyExclude);
        }

        m_vecBadIdcs = sel;

        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j) {
            m_matProj.col(m_vecBadIdcs[j]).setZero();
        }

//        qDebug() << "Bads\n" << m_vecBadIdcs;
//        qDebug() << "Proj\n";
//        qDebug() << m_matProj.block(0,0,10,10);

        qint32 nchan = m_pEvokedSet->info.nchan;
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

void EvokedSetModel::updateCompensator(int to)
{
    if(!m_pEvokedSet) {
        return;
    }

    // Update the compensator
    if(m_pEvokedSet->info.chs.size() > 0)
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
        m_pEvokedSet->info.make_compensator(0, to, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data

        //We do not need to call this->m_pFiffInfo->set_current_comp(to);
        //Because we will set the compensators to the coil in the same FiffInfo which is already used to write to file.
        //Note that the data is written in raw form not in compensated form.
        m_matComp = newComp.data->data;

        //
        // Make proj sparse
        //
        qint32 nchan = m_pEvokedSet->info.nchan;
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

void EvokedSetModel::toggleFreeze()
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

void EvokedSetModel::filterChanged(QList<FilterData> filterData)
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

void EvokedSetModel::setFilterChannelType(QString channelType)
{
    if(!m_pEvokedSet) {
        return;
    }

    m_sFilterChannelType = channelType;
    m_filterChannelList = m_visibleChannelList;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_filterChannelList.clear();

    for(int i = 0; i<m_pEvokedSet->info.chs.size(); i++) {
        if((m_pEvokedSet->info.chs.at(i).kind == FIFFV_MEG_CH || m_pEvokedSet->info.chs.at(i).kind == FIFFV_EEG_CH ||
            m_pEvokedSet->info.chs.at(i).kind == FIFFV_EOG_CH || m_pEvokedSet->info.chs.at(i).kind == FIFFV_ECG_CH ||
            m_pEvokedSet->info.chs.at(i).kind == FIFFV_EMG_CH) && !m_pEvokedSet->info.bads.contains(m_pEvokedSet->info.chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pEvokedSet->info.chs.at(i).ch_name;
            } else if(m_pEvokedSet->info.chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pEvokedSet->info.chs.at(i).ch_name;
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

void EvokedSetModel::createFilterChannelList(QStringList channelNames)
{
    if(!m_pEvokedSet) {
        return;
    }

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
    for(int i = 0; i<m_pEvokedSet->info.chs.size(); i++) {
        if((m_pEvokedSet->info.chs.at(i).kind == FIFFV_MEG_CH || m_pEvokedSet->info.chs.at(i).kind == FIFFV_EEG_CH ||
            m_pEvokedSet->info.chs.at(i).kind == FIFFV_EOG_CH || m_pEvokedSet->info.chs.at(i).kind == FIFFV_ECG_CH ||
            m_pEvokedSet->info.chs.at(i).kind == FIFFV_EMG_CH) && !m_pEvokedSet->info.bads.contains(m_pEvokedSet->info.chs.at(i).ch_name)) {
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pEvokedSet->info.chs.at(i).ch_name;
            } else if(m_pEvokedSet->info.chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pEvokedSet->info.chs.at(i).ch_name;
            }
        }
    }

//    for(int i = 0; i<m_filterChannelList.size(); i++)
//        std::cout<<m_filterChannelList.at(i).toStdString()<<std::endl;

    //Filter all visible data channels at once
    //filterChannelsConcurrently();
}


//*************************************************************************************************************

void doFilterPerChannelRTESet(QPair<QList<FilterData>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i=0; i < channelDataTime.first.size(); ++i) {
        //channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, true, FilterData::ZeroPad);
        channelDataTime.second.second = channelDataTime.first.at(i).applyFFTFilter(channelDataTime.second.second, true, FilterData::ZeroPad); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
    }
}


//*************************************************************************************************************

void EvokedSetModel::filterChannelsConcurrently()
{    
    //qDebug()<<"START EvokedSetModel::filterChannelsConcurrently()";

    if(m_filterData.isEmpty()) {
        //qDebug()<<"data filter empty";
        return;
    }

    //Generate QList structure which can be handled by the QConcurrent framework for each average in set
    for(int j = 0; j < m_matData.size(); ++j) {
        QList<QPair<QList<FilterData>,QPair<int,RowVectorXd> > > timeData;
        QList<int> notFilterChannelIndex;

        //Also append mirrored data in front and back to get rid of edge effects
        for(qint32 i = 0; i < m_matData.at(j).rows(); ++i) {
            if(m_filterChannelList.contains(m_pEvokedSet->info.chs.at(i).ch_name)) {
                RowVectorXd datTemp(m_matData.at(j).row(i).cols() + 2 * m_iMaxFilterLength);
                datTemp << m_matData.at(j).row(i).head(m_iMaxFilterLength).reverse(), m_matData.at(j).row(i), m_matData.at(j).row(i).tail(m_iMaxFilterLength).reverse();
                timeData.append(QPair<QList<FilterData>,QPair<int,RowVectorXd> >(m_filterData,QPair<int,RowVectorXd>(i,datTemp)));
            } else {
                notFilterChannelIndex.append(i);
            }
        }

        //Do the concurrent filtering
        if(!timeData.isEmpty()) {
            QFuture<void> future = QtConcurrent::map(timeData,
                                                 doFilterPerChannelRTESet);

            future.waitForFinished();

            for(int r = 0; r < timeData.size(); ++r) {
                m_matDataFiltered[j].row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength+m_iMaxFilterLength/2, m_matData.at(j).cols());
            }
        }

        //Fill filtered data with raw data if the channel was not filtered
        for(int i = 0; i<notFilterChannelIndex.size(); i++) {
            m_matDataFiltered[j].row(notFilterChannelIndex.at(i)) = m_matData.at(j).row(notFilterChannelIndex.at(i));
        }
    }

    //qDebug()<<"END EvokedSetModel::filterChannelsConcurrently()";
}

