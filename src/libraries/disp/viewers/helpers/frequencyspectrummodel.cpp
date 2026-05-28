//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     frequencyspectrummodel.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     July 2018
 * @brief    Implementation of the FrequencySpectrumModel FFT-amplitude table model.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "frequencyspectrummodel.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FrequencySpectrumModel::FrequencySpectrumModel(QObject *parent)
: QAbstractTableModel(parent)
, m_fSps(1024.0f)
, m_iT(10)
, m_bIsFreezed(false)
, m_bInitialized(false)
, m_iLowerFrqIdx(0)
, m_iUpperFrqIdx(0)
, m_iScaleType(0)
{
}

//=============================================================================================================

int FrequencySpectrumModel::rowCount(const QModelIndex & /*parent*/) const
{
    if(!m_qMapIdxRowSelection.empty())
        return m_qMapIdxRowSelection.size();
    else
        return 0;
}

//=============================================================================================================

int FrequencySpectrumModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

//=============================================================================================================

QVariant FrequencySpectrumModel::data(const QModelIndex &index, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::BackgroundRole)
        return QVariant();

    if (index.isValid()) {
        qint32 r = m_qMapIdxRowSelection[index.row()];

        //******** first column (chname) ********
        if(index.column() == 0 && role == Qt::DisplayRole)
            if(m_pFiffInfo)
                return QVariant(m_pFiffInfo->chs[r].ch_name);

        //******** second column (data plot) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole: {
                    //pack all adjacent (after reload) RowVectorPairs into a QList
                    RowVectorXd vec;

                    if(m_bIsFreezed)
                    {
                        // data freeze
                        vec = m_dataCurrentFreeze.row(r);
                        v.setValue(vec);
                    }
                    else
                    {
                        // data
                        vec = m_dataCurrent.row(r);
                        v.setValue(vec);
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

//=============================================================================================================

QVariant FrequencySpectrumModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void FrequencySpectrumModel::setInfo(FiffInfo::SPtr &info)
{
    beginResetModel();
    m_pFiffInfo = info;
    endResetModel();

    resetSelection();
}

//=============================================================================================================

void FrequencySpectrumModel::setScaleType(qint8 ScaleType)
{
    m_iScaleType = ScaleType;
}

//=============================================================================================================

void FrequencySpectrumModel::addData(const MatrixXd &data)
{
    m_dataCurrent = data;

    if(m_vecFreqScale.size() != m_dataCurrent.cols() && m_pFiffInfo)
    {
        double freqRes = (m_pFiffInfo->sfreq/2) / m_dataCurrent.cols();
        double k = 1.0;
        m_vecFreqScale.resize(1,m_dataCurrent.cols());

        double currFreq = 0;
        for(qint32 i = 0; i < m_dataCurrent.cols(); ++i)
        {
            if (m_iScaleType) //log
                m_vecFreqScale[i] = log10(currFreq+k);
            else // normal
                m_vecFreqScale[i] = currFreq;

            currFreq += freqRes;
        }

        double max = m_vecFreqScale.maxCoeff();
        m_vecFreqScale /= max;

        m_vecFreqScaleBound = m_vecFreqScale;
        m_iLowerFrqIdx = 0;
        m_iUpperFrqIdx = m_vecFreqScale.size()-1;

        m_bInitialized = true;
    }

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_dataCurrent.rows()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}

//=============================================================================================================

void FrequencySpectrumModel::selectRows(const QList<qint32> &selection)
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    qint32 count = 0;
    for(qint32 i = 0; i < selection.size(); ++i)
    {
        if(selection[i] < m_pFiffInfo->chs.size())
        {
            m_qMapIdxRowSelection.insert(count,selection[i]);
            ++count;
        }
    }

    emit newSelection(selection);

    endResetModel();
}

//=============================================================================================================

void FrequencySpectrumModel::resetSelection()
{
    beginResetModel();

    m_qMapIdxRowSelection.clear();

    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i)
        m_qMapIdxRowSelection.insert(i,i);

    endResetModel();
}

//=============================================================================================================

void FrequencySpectrumModel::toggleFreeze(const QModelIndex & index)
{
    Q_UNUSED(index);

    m_bIsFreezed = !m_bIsFreezed;

    if(m_bIsFreezed)
        m_dataCurrentFreeze = m_dataCurrent;

    //Update data content
    QModelIndex topLeft = this->index(0,1);
    QModelIndex bottomRight = this->index(m_dataCurrent.rows()-1,1);
    QVector<int> roles; roles << Qt::DisplayRole;
    emit dataChanged(topLeft, bottomRight, roles);
}

//=============================================================================================================

void FrequencySpectrumModel::setBoundaries(float fLowerFrqBound, float fUpperFrqBound)
{
    if(!m_bInitialized) {
        return;
    }

    beginResetModel();

    double nf = m_pFiffInfo->sfreq/2;

    m_iLowerFrqIdx = 0;
    m_iUpperFrqIdx = m_vecFreqScale.size()-1;

    //find boundaries
    for(qint32 i = 0; i < m_vecFreqScale.size(); ++i) {
        float val = m_vecFreqScale[i]*nf;
        if(val < fLowerFrqBound) {
            m_iLowerFrqIdx = i;
        }

        if( val > fUpperFrqBound) {
            m_iUpperFrqIdx = i;
            break;
        }
    }

    // scale it new
    m_vecFreqScaleBound = m_vecFreqScale;
    for(qint32 i = 0; i < m_vecFreqScaleBound.size(); ++i) {
        m_vecFreqScaleBound[i] = (m_vecFreqScaleBound[i] - m_vecFreqScale[m_iLowerFrqIdx]) / (m_vecFreqScale[m_iUpperFrqIdx] - m_vecFreqScale[m_iLowerFrqIdx]);
    }

    endResetModel();
}
