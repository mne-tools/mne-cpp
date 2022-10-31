//=============================================================================================================
/**
 * @file     rtfiffrawviewmodel.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.0
 * @date     May, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Definition of the RtFiffRawViewModel Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfiffrawviewmodel.h"

#include <fiff/fiff_types.h>
#include <fiff/fiff_info.h>

#include <utils/mnemath.h>
#include <utils/ioutils.h>

#include <rtprocessing/sphara.h>
#include <rtprocessing/detecttrigger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QBrush>
#include <QCoreApplication>
#include <QtConcurrent>
#include <QFuture>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EVENTSLIB::EventManager RtFiffRawViewModel::m_EventManager;

RtFiffRawViewModel::RtFiffRawViewModel(QObject *parent)
: QAbstractTableModel(parent)
, m_bProjActivated(false)
, m_bCompActivated(false)
, m_bSpharaActivated(false)
, m_bIsFreezed(false)
, m_bDrawFilterFront(true)
, m_bPerformFiltering(false)
, m_bTriggerDetectionActive(false)
, m_fSps(1024.0f)
, m_dTriggerThreshold(0.01)
, m_iT(10)
, m_iDownsampling(10)
, m_iMaxSamples(1024)
, m_iCurrentSample(0)
, m_iCurrentStartingSample(0)
, m_iCurrentSampleFreeze(0)
, m_iMaxFilterLength(128)
, m_iCurrentBlockSize(1024)
, m_iResidual(0)
, m_iCurrentTriggerChIndex(0)
, m_iDistanceTimerSpacer(1000)
, m_iDetectedTriggers(0)
, m_sFilterChannelType("MEG")
, m_pFiffInfo(FiffInfo::SPtr::create())
, m_colBackground(Qt::white)
{
    m_EventManager.initSharedMemory(EVENTSLIB::SharedMemoryMode::READWRITE);
}

//=============================================================================================================

RtFiffRawViewModel::~RtFiffRawViewModel()
{
    m_EventManager.stopSharedMemory();
}

//=============================================================================================================
//virtual functions
int RtFiffRawViewModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_pFiffInfo->chs.isEmpty()) {
        return m_pFiffInfo->chs.size();
    } else {
        return 0;
    }
}

//=============================================================================================================

int RtFiffRawViewModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}

//=============================================================================================================

QVariant RtFiffRawViewModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole) {
        return QVariant();
    }

    if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(m_colBackground));
    }

    if (index.isValid()) {
        qint32 row = m_qMapIdxRowSelection.value(index.row(),0);

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            return QVariant(m_pFiffInfo->ch_names[row]);

        //******** second column (data plot) ********
        if(index.column() == 1) {
            QVariant v;
            RowVectorPair rowVectorPair;

            switch(role) {
                case Qt::DisplayRole: {
                    if(m_bIsFreezed) {
                        // data freeze
                        if(!m_filterKernel.isEmpty() && m_bPerformFiltering) {
                            rowVectorPair.first = m_matDataFilteredFreeze.data() + row*m_matDataFilteredFreeze.cols();
                            rowVectorPair.second  = m_matDataFilteredFreeze.cols();
                            v.setValue(rowVectorPair);
                        } else {
                            rowVectorPair.first = m_matDataRawFreeze.data() + row*m_matDataRawFreeze.cols();
                            rowVectorPair.second  = m_matDataRawFreeze.cols();
                            v.setValue(rowVectorPair);
                        }
                    }
                    else {
                        // data stream
                        if(!m_filterKernel.isEmpty() && m_bPerformFiltering) {
                            rowVectorPair.first = m_matDataFiltered.data() + row*m_matDataFiltered.cols();
                            rowVectorPair.second  = m_matDataFiltered.cols();
                            v.setValue(rowVectorPair);
                        } else {
                            rowVectorPair.first = m_matDataRaw.data() + row*m_matDataRaw.cols();
                            rowVectorPair.second  = m_matDataRaw.cols();
                            v.setValue(rowVectorPair);
                        }
                    }

                    return v;
                }
            } // end role switch
        } // end column check

        //******** third column (bad channel) ********
        if(index.column() == 2 && role == Qt::DisplayRole) {
            return QVariant(m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names[row]));
        } // end column check

    } // end index.valid() check

    return QVariant();
}

//=============================================================================================================

QVariant RtFiffRawViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

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

//=============================================================================================================

void RtFiffRawViewModel::initSphara()
{
    //Load SPHARA matrices for babymeg and vectorview
    IOUtils::read_eigen_matrix(m_matSpharaVVGradLoaded, QCoreApplication::applicationDirPath() + QString("/resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Grad.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaVVMagLoaded, QCoreApplication::applicationDirPath() + QString("/resources/mne_scan/plugins/noisereduction/SPHARA/Vectorview_SPHARA_InvEuclidean_Mag.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGInnerLoaded, QCoreApplication::applicationDirPath() + QString("/resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Inner.txt"));
    IOUtils::read_eigen_matrix(m_matSpharaBabyMEGOuterLoaded, QCoreApplication::applicationDirPath() + QString("/resources/mne_scan/plugins/noisereduction/SPHARA/BabyMEG_SPHARA_InvEuclidean_Outer.txt"));

    IOUtils::read_eigen_matrix(m_matSpharaEEGLoaded, QCoreApplication::applicationDirPath() + QString("/resources/mne_scan/plugins/noisereduction/SPHARA/Current_SPHARA_EEG.txt"));

    //Generate indices used to create the SPHARA operators for VectorView
    m_vecIndicesFirstVV.resize(0);
    m_vecIndicesSecondVV.resize(0);

    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find GRADIOMETERS
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3012) {
            m_vecIndicesFirstVV.conservativeResize(m_vecIndicesFirstVV.rows()+1);
            m_vecIndicesFirstVV(m_vecIndicesFirstVV.rows()-1) = r;
        }

        //Find Magnetometers
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 3024) {
            m_vecIndicesSecondVV.conservativeResize(m_vecIndicesSecondVV.rows()+1);
            m_vecIndicesSecondVV(m_vecIndicesSecondVV.rows()-1) = r;
        }
    }

    //Generate indices used to create the SPHARA operators for babyMEG
    m_vecIndicesFirstBabyMEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find INNER LAYER
        if(m_pFiffInfo->chs.at(r).chpos.coil_type == 7002) {
            m_vecIndicesFirstBabyMEG.conservativeResize(m_vecIndicesFirstBabyMEG.rows()+1);
            m_vecIndicesFirstBabyMEG(m_vecIndicesFirstBabyMEG.rows()-1) = r;
        }

        //TODO: Find outer layer
    }

    //Generate indices used to create the SPHARA operators for EEG layouts
    m_vecIndicesFirstEEG.resize(0);
    for(int r = 0; r < m_pFiffInfo->chs.size(); ++r) {
        //Find EEG
        if(m_pFiffInfo->chs.at(r).kind == FIFFV_EEG_CH) {
            m_vecIndicesFirstEEG.conservativeResize(m_vecIndicesFirstEEG.rows()+1);
            m_vecIndicesFirstEEG(m_vecIndicesFirstEEG.rows()-1) = r;
        }
    }

    //Create Sphara operator for the first time
    updateSpharaOptions("BabyMEG", 270, 105);

    qDebug()<<"RtFiffRawViewModel::initSphara - Read VectorView mag matrix "<<m_matSpharaVVMagLoaded.rows()<<m_matSpharaVVMagLoaded.cols()<<"and grad matrix"<<m_matSpharaVVGradLoaded.rows()<<m_matSpharaVVGradLoaded.cols();
    qDebug()<<"RtFiffRawViewModel::initSphara - Read BabyMEG inner layer matrix "<<m_matSpharaBabyMEGInnerLoaded.rows()<<m_matSpharaBabyMEGInnerLoaded.cols()<<"and outer layer matrix"<<m_matSpharaBabyMEGOuterLoaded.rows()<<m_matSpharaBabyMEGOuterLoaded.cols();
}

//=============================================================================================================

void RtFiffRawViewModel::setFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> &p_pFiffInfo)
{
    if(p_pFiffInfo) {
        RowVectorXi sel;// = RowVectorXi(0,0);
        QStringList emptyExclude;

        if(p_pFiffInfo->bads.size() > 0) {
            sel = FiffInfoBase::pick_channels(p_pFiffInfo->ch_names, p_pFiffInfo->bads, emptyExclude);
        }

        m_vecBadIdcs = sel;

        m_pFiffInfo = p_pFiffInfo;

        resetSelection();

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

        m_matSparseProjMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
        m_matSparseCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
        m_matSparseSpharaMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());
        m_matSparseProjCompMult = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

        m_matSparseProjMult.setIdentity();
        m_matSparseCompMult.setIdentity();
        m_matSparseSpharaMult.setIdentity();
        m_matSparseProjCompMult.setIdentity();

        //Create the initial Compensator projector
        updateCompensator(0);

        //Initialize filter channel names
        int visibleInit = 20;
        QStringList filterChannels;

        if(visibleInit > m_pFiffInfo->chs.size()) {
            while(visibleInit>m_pFiffInfo->chs.size()) {
                visibleInit--;
            }
        }

        for(qint32 b = 0; b < visibleInit; ++b) {
            filterChannels.append(m_pFiffInfo->ch_names.at(b));
        }

        createFilterChannelList(filterChannels);

//        //Look for trigger channels and initialise detected trigger map
//        for(int i = 0; i<m_pFiffInfo->chs.size(); ++i) {
//            if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH/* && m_pFiffInfo->chs[i].ch_name == "STI 001"*/)
//                m_lTriggerChannelIndices.append(i);
//        }

        //Init the sphara operators
        initSphara();
    } else {
        m_vecBadIdcs = RowVectorXi(0,0);
        m_matProj = MatrixXd(0,0);
        m_matComp = MatrixXd(0,0);
    }
}

//=============================================================================================================

void RtFiffRawViewModel::setSamplingInfo(float sps, int T, bool bSetZero)
{
    beginResetModel();

    m_iT = T;

    m_iMaxSamples = (qint32) ceil(sps * T);

    //Resize data matrix without touching the stored values
    m_matDataRaw.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
    m_matDataFiltered.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxSamples);
    m_vecLastBlockFirstValuesRaw.conservativeResize(m_pFiffInfo->chs.size());
    m_vecLastBlockFirstValuesFiltered.conservativeResize(m_pFiffInfo->chs.size());

    if(bSetZero) {
        m_matDataRaw.setZero();
        m_matDataFiltered.setZero();
        m_vecLastBlockFirstValuesRaw.setZero();
        m_vecLastBlockFirstValuesFiltered.setZero();
    }

    if(m_iCurrentSample>m_iMaxSamples) {
        m_iCurrentStartingSample += m_iCurrentSample;
        m_iCurrentSample = 0;
    }

    endResetModel();
}

//=============================================================================================================

MatrixXd RtFiffRawViewModel::getLastBlock()
{
    if(!m_filterKernel.isEmpty() && m_bPerformFiltering) {
        return m_matDataFiltered.block(0, m_iCurrentSample-m_iCurrentBlockSize, m_matDataFiltered.rows(), m_iCurrentBlockSize);
    }

    return m_matDataRaw.block(0, m_iCurrentSample-m_iCurrentBlockSize, m_matDataRaw.rows(), m_iCurrentBlockSize);
}

//=============================================================================================================

void RtFiffRawViewModel::addData(const QList<MatrixXd> &data)
{
    //SSP
    bool doProj = m_bProjActivated && m_matDataRaw.cols() > 0 && m_matDataRaw.rows() == m_matProj.cols() ? true : false;

    //Compensator
    bool doComp = m_bCompActivated && m_matDataRaw.cols() > 0 && m_matDataRaw.rows() == m_matComp.cols() ? true : false;

    //SPHARA
    bool doSphara = m_bSpharaActivated && m_matSparseSpharaMult.cols() > 0 && m_matDataRaw.rows() == m_matSparseSpharaMult.cols() ? true : false;

    //Copy new data into the global data matrix
    for(qint32 b = 0; b < data.size(); ++b) {
        int nCol = data.at(b).cols();
        int nRow = data.at(b).rows();

        if(nRow != m_matDataRaw.rows()) {
            qDebug()<<"incoming data does not match internal data row size. Returning...";
            return;
        }

        //Reset m_iCurrentSample and start filling the data matrix from the beginning again. Also add residual amount of data to the end of the matrix.
        if(m_iCurrentSample+nCol > m_matDataRaw.cols()) {
            m_iResidual = nCol - ((m_iCurrentSample+nCol) % m_matDataRaw.cols());

            if(m_iResidual == nCol) {
                m_iResidual = 0;
            }

//            std::cout<<"incoming data exceeds internal data cols by: "<<(m_iCurrentSample+nCol) % m_matDataRaw.cols()<<std::endl;
//            std::cout<<"m_iCurrentSample+nCol: "<<m_iCurrentSample+nCol<<std::endl;
//            std::cout<<"m_matDataRaw.cols(): "<<m_matDataRaw.cols()<<std::endl;
//            std::cout<<"nCol-m_iResidual: "<<nCol-m_iResidual<<std::endl<<std::endl;

            if(doComp) {
                if(doProj) {
                    //Comp + Proj
                    m_matDataRaw.block(0, m_iCurrentSample, nRow, m_iResidual) = m_matSparseProjCompMult * data.at(b).block(0,0,nRow,m_iResidual);
                } else {
                    //Comp
                    m_matDataRaw.block(0, m_iCurrentSample, nRow, m_iResidual) = m_matSparseCompMult * data.at(b).block(0,0,nRow,m_iResidual);
                }
            } else {
                if(doProj)
                {
                    //Proj
                    m_matDataRaw.block(0, m_iCurrentSample, nRow, m_iResidual) = m_matSparseProjMult * data.at(b).block(0,0,nRow,m_iResidual);
                } else {
                    //None - Raw
                    m_matDataRaw.block(0, m_iCurrentSample, nRow, m_iResidual) = data.at(b).block(0,0,nRow,m_iResidual);
                }
            }

            m_iCurrentStartingSample += m_iCurrentSample;
            m_iCurrentStartingSample += m_iResidual;

            m_iCurrentSample = 0;

            if(!m_bIsFreezed) {
                m_vecLastBlockFirstValuesFiltered = m_matDataFiltered.col(0);
                m_vecLastBlockFirstValuesRaw = m_matDataRaw.col(0);
            }

            //Store old detected triggers
            m_qMapDetectedTriggerOld = m_qMapDetectedTrigger;

            //Clear detected triggers
            if(m_bTriggerDetectionActive) {
                QMutableMapIterator<int,QList<QPair<int,double> > > i(m_qMapDetectedTrigger);
                while (i.hasNext()) {
                    i.next();
                    i.value().clear();
                }
            }
        } else {
            m_iResidual = 0;
        }

        //std::cout<<"incoming data is ok"<<std::endl;

        if(doComp) {
            if(doProj) {
                //Comp + Proj
                m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol) = m_matSparseProjCompMult * data.at(b);
            } else {
                //Comp
                m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol) = m_matSparseCompMult * data.at(b);
            }
        } else {
            if(doProj) {
                //Proj
                m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol) = m_matSparseProjMult * data.at(b);
            } else {
                //None - Raw
                m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol) = data.at(b);
            }
        }

        //Filter if neccessary else set filtered data matrix to zero
        if(!m_filterKernel.isEmpty() && m_bPerformFiltering) {
            filterDataBlock(m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol), m_iCurrentSample);

            //Perform SPHARA on filtered data after actual filtering - SPHARA should be applied on the best possible data
            if(doSphara) {
                if(m_iCurrentSample-m_iMaxFilterLength/2 >= 0) {
                    m_matDataFiltered.block(0, m_iCurrentSample-m_iMaxFilterLength/2, nRow, nCol) = m_matSparseSpharaMult * m_matDataFiltered.block(0, m_iCurrentSample-m_iMaxFilterLength/2, nRow, nCol);
                }
                else {
                    if(m_iCurrentSample-m_iMaxFilterLength/2 < 0) {
                        m_matDataFiltered.block(0, 0, nRow, nCol) = m_matSparseSpharaMult * m_matDataFiltered.block(0, 0, nRow, nCol);
                        int iResidual = m_iResidual+m_iMaxFilterLength/2;
                        m_matDataFiltered.block(0, m_matDataFiltered.cols()-iResidual, nRow, iResidual) = m_matSparseSpharaMult * m_matDataFiltered.block(0, m_matDataFiltered.cols()-iResidual, nRow, iResidual);
                    }
                }
            }
        } else {
            m_matDataFiltered.block(0, m_iCurrentSample, nRow, nCol).setZero();// = m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol);

            //Perform SPHARA on raw data data
            if(doSphara) {
                m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol) = m_matSparseSpharaMult * m_matDataRaw.block(0, m_iCurrentSample, nRow, nCol);
            }
        }

        m_iCurrentSample += nCol;
        m_iCurrentBlockSize = nCol;

        //detect the trigger flanks in the trigger channels
        if(m_bTriggerDetectionActive) {
            int iOldDetectedTriggers = m_qMapDetectedTrigger[m_iCurrentTriggerChIndex].size();

            QList<QPair<int,double> > qMapDetectedTrigger = RTPROCESSINGLIB::detectTriggerFlanksMax(data.at(b), m_iCurrentTriggerChIndex, m_iCurrentSample-nCol, m_dTriggerThreshold, true, 500);
            //QList<QPair<int,double> > qMapDetectedTrigger = RTPROCESSINGLIB::detectTriggerFlanksGrad(data.at(b), m_iCurrentTriggerChIndex, m_iCurrentSample-nCol, m_dTriggerThreshold, false, "Rising");

            //Append results to already found triggers
            m_qMapDetectedTrigger[m_iCurrentTriggerChIndex].append(qMapDetectedTrigger);

            //Compute newly counted triggers
            int newTriggers = m_qMapDetectedTrigger[m_iCurrentTriggerChIndex].size() - iOldDetectedTriggers;

            if(newTriggers!=0) {
                m_iDetectedTriggers += newTriggers;
                emit triggerDetected(m_iDetectedTriggers, m_qMapDetectedTrigger);
            }
        }
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_pFiffInfo->ch_names.size()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;

    emit dataChanged(topLeft, bottomRight, roles);
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getKind(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pFiffInfo->chs.at(chRow).kind;
    }

    return 0;
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getUnit(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pFiffInfo->chs.at(chRow).unit;
    }

    return FIFF_UNIT_NONE;
}

//=============================================================================================================

fiff_int_t RtFiffRawViewModel::getCoil(qint32 row) const
{
    if(row < m_qMapIdxRowSelection.size()) {
        qint32 chRow = m_qMapIdxRowSelection[row];
        return m_pFiffInfo->chs.at(chRow).chpos.coil_type;
    }

    return FIFFV_COIL_NONE;
}

//=============================================================================================================

void RtFiffRawViewModel::selectRows(const QList<qint32> &selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i) {
        if(selection[i] < m_pFiffInfo->chs.size()) {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::hideRows(const QList<qint32> &selection)
{
    beginResetModel();

    for(qint32 i = 0; i < selection.size(); ++i) {
        if(m_qMapIdxRowSelection.contains(selection.at(i))) {
            m_qMapIdxRowSelection.remove(selection.at(i));
        }
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        m_qMapIdxRowSelection.insert(i,i);
    }

    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::toggleFreeze(const QModelIndex &)
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
    QModelIndex bottomRight = this->index(m_pFiffInfo->chs.size()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;

    emit dataChanged(topLeft, bottomRight, roles);
}

//=============================================================================================================

void RtFiffRawViewModel::setScaling(const QMap< qint32,float >& p_qMapChScaling)
{
    beginResetModel();
    m_qMapChScaling = p_qMapChScaling;
    endResetModel();
}

//=============================================================================================================

void RtFiffRawViewModel::updateProjection(const QList<FIFFLIB::FiffProj>& projs)
{
    //  Update the SSP projector
    if(m_pFiffInfo) {
        //If a minimum of one projector is active set m_bProjActivated to true so that this model applies the ssp to the incoming data
        m_bProjActivated = false;
        m_pFiffInfo->projs = projs;

        for(qint32 i = 0; i < this->m_pFiffInfo->projs.size(); ++i) {
            if(this->m_pFiffInfo->projs[i].active) {
                m_bProjActivated = true;
                break;
            }
        }

        this->m_pFiffInfo->make_projector(m_matProj);

        qDebug() << "RtFiffRawViewModel::updateProjection - New projection calculated.";

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_vecBadIdcs.cols(); ++j) {
            m_matProj.col(m_vecBadIdcs[j]).setZero();
        }

//        std::cout << "Bads\n" << m_vecBadIdcs << std::endl;
//        std::cout << "Proj\n";
//        std::cout << m_matProj.block(0,0,10,10) << std::endl;

        //
        // Make proj sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

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

//=============================================================================================================

void RtFiffRawViewModel::updateCompensator(int to)
{
    //  Update the compensator
    if(m_pFiffInfo) {
        if(to == 0) {
            m_bCompActivated = false;
        } else {
            m_bCompActivated = true;
        }

//        qDebug()<<"to"<<to;
//        qDebug()<<"from"<<from;
//        qDebug()<<"m_bCompActivated"<<m_bCompActivated;

        FiffCtfComp newComp;
        this->m_pFiffInfo->make_compensator(0, to, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data

        //We do not need to call this->m_pFiffInfo->set_current_comp(to);
        //Because we will set the compensators to the coil in the same FiffInfo which is already used to write to file.
        //Note that the data is written in raw form not in compensated form.
        m_matComp = newComp.data->data;

        //
        // Make proj sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
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

//=============================================================================================================

void RtFiffRawViewModel::updateSpharaActivation(bool state)
{
    m_bSpharaActivated = state;
}

//=============================================================================================================

void RtFiffRawViewModel::updateSpharaOptions(const QString& sSytemType, int nBaseFctsFirst, int nBaseFctsSecond)
{
    if(m_pFiffInfo) {
        qDebug()<<"RtFiffRawViewModel::updateSpharaOptions - Creating SPHARA operator for"<<sSytemType;

        MatrixXd matSpharaMultFirst = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
        MatrixXd matSpharaMultSecond = MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

        if(sSytemType == "VectorView" && m_matSpharaVVGradLoaded.size() != 0 && m_matSpharaVVMagLoaded.size() != 0) {
            matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVGradLoaded, m_vecIndicesFirstVV, m_pFiffInfo->nchan, nBaseFctsFirst, 1); //GRADIOMETERS
            matSpharaMultSecond = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaVVMagLoaded, m_vecIndicesSecondVV, m_pFiffInfo->nchan, nBaseFctsSecond, 0); //Magnetometers
        }

        if(sSytemType == "BabyMEG" && m_matSpharaBabyMEGInnerLoaded.size() != 0) {
            matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaBabyMEGInnerLoaded, m_vecIndicesFirstBabyMEG, m_pFiffInfo->nchan, nBaseFctsFirst, 0); //InnerLayer
        }

        if(sSytemType == "EEG" && m_matSpharaEEGLoaded.size() != 0) {
            matSpharaMultFirst = RTPROCESSINGLIB::makeSpharaProjector(m_matSpharaEEGLoaded, m_vecIndicesFirstEEG, m_pFiffInfo->nchan, nBaseFctsFirst, 0); //InnerLayer
        }

        //Write final operator matrices to file
//        IOUtils::write_eigen_matrix(matSpharaMultFirst, QString(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultFirst.txt"));
//        IOUtils::write_eigen_matrix(matSpharaMultSecond, QString(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/noisereduction/SPHARA/matSpharaMultSecond.txt"));
//        IOUtils::write_eigen_matrix(m_matSpharaEEGLoaded, QString(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/noisereduction/SPHARA/m_matSpharaEEGLoaded.txt"));

        //
        // Make operators sparse
        //
        qint32 nchan = this->m_pFiffInfo->nchan;
        qint32 i, k;

        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(nchan);

        //First operator
        tripletList.clear();
        tripletList.reserve(matSpharaMultFirst.rows()*matSpharaMultFirst.cols());
        for(i = 0; i < matSpharaMultFirst.rows(); ++i) {
            for(k = 0; k < matSpharaMultFirst.cols(); ++k) {
                if(matSpharaMultFirst(i,k) != 0) {
                    tripletList.push_back(T(i, k, matSpharaMultFirst(i,k)));
                }
            }
        }

        Eigen::SparseMatrix<double> matSparseSpharaMultFirst = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

        matSparseSpharaMultFirst = SparseMatrix<double>(matSpharaMultFirst.rows(),matSpharaMultFirst.cols());
        if(tripletList.size() > 0) {
            matSparseSpharaMultFirst.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Second operator
        tripletList.clear();
        tripletList.reserve(matSpharaMultSecond.rows()*matSpharaMultSecond.cols());

        for(i = 0; i < matSpharaMultSecond.rows(); ++i) {
            for(k = 0; k < matSpharaMultSecond.cols(); ++k) {
                if(matSpharaMultSecond(i,k) != 0) {
                    tripletList.push_back(T(i, k, matSpharaMultSecond(i,k)));
                }
            }
        }

        Eigen::SparseMatrix<double>matSparseSpharaMultSecond = SparseMatrix<double>(m_pFiffInfo->chs.size(),m_pFiffInfo->chs.size());

        if(tripletList.size() > 0) {
            matSparseSpharaMultSecond.setFromTriplets(tripletList.begin(), tripletList.end());
        }

        //Create full multiplication matrix
        m_matSparseSpharaMult = matSparseSpharaMultFirst * matSparseSpharaMultSecond;
    }
}

//=============================================================================================================

void RtFiffRawViewModel::setFilter(QList<FilterKernel> filterData)
{
    m_filterKernel = filterData;

    m_iMaxFilterLength = 1;
    for(int i=0; i<filterData.size(); ++i) {
        if(m_iMaxFilterLength<filterData.at(i).getFilterOrder()) {
            m_iMaxFilterLength = filterData.at(i).getFilterOrder();
        }
    }

    m_matOverlap.conservativeResize(m_pFiffInfo->chs.size(), m_iMaxFilterLength);
    m_matOverlap.setZero();

    m_bDrawFilterFront = false;

    //Filter all visible data channels at once
    //filterDataBlock();
}

//=============================================================================================================

void RtFiffRawViewModel::setFilterActive(bool state)
{
    m_bPerformFiltering = state;
}

//=============================================================================================================

void RtFiffRawViewModel::setBackgroundColor(const QColor& color)
{
    m_colBackground = color;
}

//=============================================================================================================

void RtFiffRawViewModel::setFilterChannelType(const QString &channelType)
{
    m_sFilterChannelType = channelType;
    m_filterChannelList = m_visibleChannelList;

    //This version is for when all channels of a type are to be filtered (not only the visible ones).
    //Create channel filter list independent from channelNames
    m_filterChannelList.clear();

    for(int i = 0; i<m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
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
    //filterDataBlock();
}

//=============================================================================================================

void RtFiffRawViewModel::createFilterChannelList(QStringList channelNames)
{
    m_filterChannelList.clear();
    m_visibleChannelList = channelNames;

//    //Create channel fiter list based on channelNames
//    for(int i = 0; i<m_pFiffInfo->chs.size(); ++i) {
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
    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        if((m_pFiffInfo->chs.at(i).kind == FIFFV_MEG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_EEG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EOG_CH || m_pFiffInfo->chs.at(i).kind == FIFFV_ECG_CH ||
            m_pFiffInfo->chs.at(i).kind == FIFFV_EMG_CH)/* && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)*/) {
            if(m_sFilterChannelType == "All") {
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            } else if(m_pFiffInfo->chs.at(i).ch_name.contains(m_sFilterChannelType)) {
                m_filterChannelList << m_pFiffInfo->chs.at(i).ch_name;
            }
        }
    }

//    m_bDrawFilterFront = false;

//    for(int i = 0; i<m_filterChannelList.size(); ++i)
//        std::cout<<m_filterChannelList.at(i).toStdString()<<std::endl;

    //Filter all visible data channels at once
    //filterDataBlock();
}

//=============================================================================================================

void RtFiffRawViewModel::markChBad(QModelIndex ch, bool status)
{
    QList<FiffChInfo> chInfolist = m_pFiffInfo->chs;

    if(status) {
        if(!m_pFiffInfo->bads.contains(chInfolist[ch.row()].ch_name))
            m_pFiffInfo->bads.append(chInfolist[ch.row()].ch_name);
        qDebug() << "RawModel:" << chInfolist[ch.row()].ch_name << "marked as bad.";
    } else if(m_pFiffInfo->bads.contains(chInfolist[ch.row()].ch_name)) {
        int index = m_pFiffInfo->bads.indexOf(chInfolist[ch.row()].ch_name);
        m_pFiffInfo->bads.removeAt(index);
        qDebug() << "RawModel:" << chInfolist[ch.row()].ch_name << "marked as good.";
    }

    //Redefine channels which are to be filtered
    QStringList channelNames;
    createFilterChannelList(channelNames);

    //Update indeices of bad channels (this vector is needed when creating new ssp operators)
    QStringList emptyExclude;
    m_vecBadIdcs = FiffInfoBase::pick_channels(m_pFiffInfo->ch_names, m_pFiffInfo->bads, emptyExclude);

    emit dataChanged(ch,ch);
}

//=============================================================================================================

void RtFiffRawViewModel::triggerInfoChanged(const QMap<double, QColor>& colorMap, bool active, QString triggerCh, double threshold)
{
    m_qMapTriggerColor = colorMap;
    m_bTriggerDetectionActive = active;
    m_dTriggerThreshold = threshold;

    //Find channel index and initialise detected trigger map if channel name changed
    if(m_sCurrentTriggerCh != triggerCh) {
        m_sCurrentTriggerCh = triggerCh;

        QList<QPair<int,double> > temp;
        m_qMapDetectedTrigger.clear();

        for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
            if(m_pFiffInfo->chs[i].ch_name == m_sCurrentTriggerCh) {
                m_iCurrentTriggerChIndex = i;
                m_qMapDetectedTrigger.insert(i, temp);
                break;
            }
        }
    }

    m_sCurrentTriggerCh = triggerCh;
}

//=============================================================================================================

void RtFiffRawViewModel::distanceTimeSpacerChanged(int value)
{
    if(value <= 0) {
        m_iDistanceTimerSpacer = 1000;
    } else {
        m_iDistanceTimerSpacer = value;
    }
}

//=============================================================================================================

void RtFiffRawViewModel::resetTriggerCounter()
{
    m_iDetectedTriggers = 0;
}

//=============================================================================================================

void RtFiffRawViewModel::markChBad(QModelIndexList chlist, bool status)
{
    QList<FiffChInfo> chInfolist = m_pFiffInfo->chs;

    for(int i = 0; i < chlist.size(); ++i) {
        if(status) {
            if(!m_pFiffInfo->bads.contains(chInfolist[chlist[i].row()].ch_name))
                m_pFiffInfo->bads.append(chInfolist[chlist[i].row()].ch_name);
        } else {
            if(m_pFiffInfo->bads.contains(chInfolist[chlist[i].row()].ch_name)) {
                int index = m_pFiffInfo->bads.indexOf(chInfolist[chlist[i].row()].ch_name);
                m_pFiffInfo->bads.removeAt(index);
            }
        }

        emit dataChanged(chlist[i],chlist[i]);
    }

    //Update indeices of bad channels (this vector is needed when creating new ssp operators)
    QStringList emptyExclude;
    m_vecBadIdcs = FiffInfoBase::pick_channels(m_pFiffInfo->ch_names, m_pFiffInfo->bads, emptyExclude);
}

//=============================================================================================================

void RtFiffRawViewModel::doFilterPerChannelRTMSA(QPair<QList<FilterKernel>,QPair<int,RowVectorXd> > &channelDataTime)
{
    for(int i = 0; i < channelDataTime.first.size(); ++i) {
        //channelDataTime.second.second = channelDataTime.first.at(i).applyConvFilter(channelDataTime.second.second, true);
        channelDataTime.first[i].applyFftFilter(channelDataTime.second.second, true); //FFT Convolution for rt is not suitable. FFT make the signal filtering non causal.
    }
}

//=============================================================================================================

void RtFiffRawViewModel::filterDataBlock()
{
    //std::cout<<"START RtFiffRawViewModel::filterDataBlock"<<std::endl;

    if(m_filterKernel.isEmpty() || !m_bPerformFiltering) {
        return;
    }

    //Create temporary filters with higher fft length because we are going to filter all available data at once for one time
    QList<FilterKernel> tempFilterList;

    int fftLength = m_matDataRaw.row(0).cols() + 4 * m_iMaxFilterLength;
    int exp = ceil(MNEMath::log2(fftLength));
    fftLength = pow(2, exp) < 512 ? 512 : pow(2, exp);

    for(int i = 0; i<m_filterKernel.size(); ++i) {
        FilterKernel tempFilter(m_filterKernel.at(i).getName(),
                                FilterKernel::m_filterTypes.indexOf(m_filterKernel.at(i).getFilterType()),
                                m_filterKernel.at(i).getFilterOrder(),
                                m_filterKernel.at(i).getCenterFrequency(),
                                m_filterKernel.at(i).getBandwidth(),
                                m_filterKernel.at(i).getParksWidth(),
                                m_filterKernel.at(i).getSamplingFrequency(),
                                FilterKernel::m_designMethods.indexOf(m_filterKernel.at(i).getDesignMethod()));

        tempFilterList.append(tempFilter);
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterKernel>,QPair<int,RowVectorXd> > > timeData;
    QList<int> notFilterChannelIndex;

    //Also append mirrored data in front and back to get rid of edge effects
    for(qint32 i=0; i<m_matDataRaw.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            RowVectorXd datTemp(m_matDataRaw.row(i).cols() + 2 * m_iMaxFilterLength);
            datTemp << m_matDataRaw.row(i).head(m_iMaxFilterLength).reverse(), m_matDataRaw.row(i), m_matDataRaw.row(i).tail(m_iMaxFilterLength).reverse();
            timeData.append(QPair<QList<FilterKernel>,QPair<int,RowVectorXd> >(tempFilterList,QPair<int,RowVectorXd>(i,datTemp)));
        } else {
            notFilterChannelIndex.append(i);
        }
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTMSA);

        future.waitForFinished();

        for(int r = 0; r < timeData.size(); ++r) {
            m_matDataFiltered.row(timeData.at(r).second.first) = timeData.at(r).second.second.segment(m_iMaxFilterLength+m_iMaxFilterLength/2, m_matDataRaw.cols());
            m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
        }
    }

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i < notFilterChannelIndex.size(); ++i) {
        m_matDataFiltered.row(notFilterChannelIndex.at(i)) = m_matDataRaw.row(notFilterChannelIndex.at(i));
    }

    if(!m_bIsFreezed) {
        m_vecLastBlockFirstValuesFiltered = m_matDataFiltered.col(0);
    }

    //std::cout<<"END RtFiffRawViewModel::filterDataBlock"<<std::endl;
}

//=============================================================================================================

void RtFiffRawViewModel::filterDataBlock(const MatrixXd &data, int iDataIndex)
{
    //std::cout<<"START RtFiffRawViewModel::filterDataBlock"<<std::endl;

    if(iDataIndex >= m_matDataFiltered.cols() || data.cols() < m_iMaxFilterLength) {
        return;
    }

    //Generate QList structure which can be handled by the QConcurrent framework
    QList<QPair<QList<FilterKernel>,QPair<int,RowVectorXd> > > timeData;
    QList<int> notFilterChannelIndex;

    for(qint32 i = 0; i < data.rows(); ++i) {
        if(m_filterChannelList.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            timeData.append(QPair<QList<FilterKernel>,QPair<int,RowVectorXd> >(m_filterKernel,QPair<int,RowVectorXd>(i,data.row(i))));
        } else {
            notFilterChannelIndex.append(i);
            }
    }

    //Do the concurrent filtering
    if(!timeData.isEmpty()) {
        QFuture<void> future = QtConcurrent::map(timeData,
                                             doFilterPerChannelRTMSA);

        future.waitForFinished();

        //Do the overlap add method and store in m_matDataFiltered
        int iFilterDelay = m_iMaxFilterLength/2;
        int iFilteredNumberCols = timeData.at(0).second.second.cols();

        for(int r = 0; r<timeData.size(); ++r) {
            if(iDataIndex+2*data.cols() > m_matDataRaw.cols()) {
                //Handle last data block
                //std::cout<<"Handle last data block"<<std::endl;

                if(m_bDrawFilterFront) {
                    //Get the currently filtered data. This data has a delay of filterLength/2 in front and back.
                    RowVectorXd tempData = timeData.at(r).second.second;

                    //Perform the actual overlap add by adding the last filterlength data to the newly filtered one
                    tempData.head(m_iMaxFilterLength) += m_matOverlap.row(timeData.at(r).second.first);

                    //Write the newly calulated filtered data to the filter data matrix. Keep in mind that the current block also effect last part of the last block (begin at dataIndex-iFilterDelay).
                    int start = iDataIndex-iFilterDelay < 0 ? 0 : iDataIndex-iFilterDelay;
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(start,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iDataIndex-iFilterDelay,m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iDataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            } else if(iDataIndex == 0) {
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
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iDataIndex-iFilterDelay,iFilteredNumberCols-m_iMaxFilterLength) = tempData.head(iFilteredNumberCols-m_iMaxFilterLength);
                } else {
                    //Perform this else case everytime the filter was changed. Do not begin to plot from dataIndex-iFilterDelay because the impsulse response and m_matOverlap do not match with the new filter anymore.
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iDataIndex-iFilterDelay,m_iMaxFilterLength).setZero();// = timeData.at(r).second.second.segment(m_iMaxFilterLength,m_iMaxFilterLength);
                    m_matDataFiltered.row(timeData.at(r).second.first).segment(iDataIndex+iFilterDelay,iFilteredNumberCols-2*m_iMaxFilterLength) = timeData.at(r).second.second.segment(m_iMaxFilterLength,iFilteredNumberCols-2*m_iMaxFilterLength);
                }

                //Refresh the m_matOverlap with the new calculated filtered data.
                m_matOverlap.row(timeData.at(r).second.first) = timeData.at(r).second.second.tail(m_iMaxFilterLength);
            }
        }
    }

    m_bDrawFilterFront = true;

    //Fill filtered data with raw data if the channel was not filtered
    for(int i = 0; i < notFilterChannelIndex.size(); ++i) {
        m_matDataFiltered.row(notFilterChannelIndex.at(i)).segment(iDataIndex,data.row(notFilterChannelIndex.at(i)).cols()) = data.row(notFilterChannelIndex.at(i));
    }

    //std::cout<<"END RtFiffRawViewModel::filterDataBlock"<<std::endl;
}

//=============================================================================================================

void RtFiffRawViewModel::clearModel()
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
}

//=============================================================================================================

double RtFiffRawViewModel::getMaxValueFromRawViewModel(int row) const
{
    double dMaxValue;
    qint32 kind = getKind(row);

    switch(kind) {
        case FIFFV_MEG_CH: {
            dMaxValue = 1e-11f;
            qint32 unit = getUnit(row);
            if(unit == FIFF_UNIT_T_M) { //gradiometers
                dMaxValue = 1e-10f;
                if(getScaling().contains(FIFF_UNIT_T_M))
                    dMaxValue = getScaling()[FIFF_UNIT_T_M];
            }
            else if(unit == FIFF_UNIT_T) //magnetometers
            {
                dMaxValue = 1e-11f;
                if(getScaling().contains(FIFF_UNIT_T))
                    dMaxValue = getScaling()[FIFF_UNIT_T];
            }
            break;
        }

        case FIFFV_REF_MEG_CH: {
            dMaxValue = 1e-11f;
            if( getScaling().contains(FIFF_UNIT_T))
                dMaxValue = getScaling()[FIFF_UNIT_T];
            break;
        }
        case FIFFV_EEG_CH: {
            dMaxValue = 1e-4f;
            if( getScaling().contains(FIFFV_EEG_CH))
                dMaxValue = getScaling()[FIFFV_EEG_CH];
            break;
        }
        case FIFFV_EOG_CH: {
            dMaxValue = 1e-3f;
            if( getScaling().contains(FIFFV_EOG_CH))
                dMaxValue = getScaling()[FIFFV_EOG_CH];
            break;
        }
        case FIFFV_STIM_CH: {
            dMaxValue = 5;
            if( getScaling().contains(FIFFV_STIM_CH))
                dMaxValue = getScaling()[FIFFV_STIM_CH];
            break;
        }
        case FIFFV_MISC_CH: {
            dMaxValue = 1e-3f;
            if( getScaling().contains(FIFFV_MISC_CH))
                dMaxValue = getScaling()[FIFFV_MISC_CH];
            break;
        }
        default :
        dMaxValue = 1e-9f;
        break;
    }

    return dMaxValue;
}

//=============================================================================================================

void RtFiffRawViewModel::addEvent(int iSample)
{
    auto pGroups = m_EventManager.getAllGroups();
    for(auto g : *pGroups){
        qDebug() << "Group: " << g.name.c_str() << "- Id: " << g.id;
    }

    m_EventManager.addEvent(iSample);

    auto pEvents = m_EventManager.getAllEvents();

    for(auto e : *pEvents){
        qDebug() << "Event> Sample: " << e.sample << "- Id: " << e.id;
    }
}

//=============================================================================================================

std::unique_ptr<std::vector<EVENTSLIB::Event> > RtFiffRawViewModel::getEventsToDisplay(int iBegin, int iEnd) const
{
    return m_EventManager.getEventsBetween(iBegin, iEnd);
}
