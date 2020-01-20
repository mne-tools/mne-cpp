//=============================================================================================================
/**
 * @file     averagemodel.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    This class represents the average model of the model/view framework of mne_browse application.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagemodel.h"
#include <fiff/fiff_io.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageModel::AverageModel(QObject *parent)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
, m_pfiffIO(QSharedPointer<FiffIO>(new FiffIO()))
, m_pEvokedDataSet(FiffEvokedSet::SPtr(new FiffEvokedSet))
{
}


//*************************************************************************************************************

AverageModel::AverageModel(QFile& qFile, QObject *parent)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
, m_pfiffIO(QSharedPointer<FiffIO>(new FiffIO()))
, m_pEvokedDataSet(FiffEvokedSet::SPtr(new FiffEvokedSet))
{
    //read evoked fiff data
    loadEvokedData(qFile);
}


//*************************************************************************************************************
//virtual functions
int AverageModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Return number of stored evoked sets
    if(!m_pEvokedDataSet->evoked.size()==0)
        return m_pEvokedDataSet->evoked.size();
    else
        return 0;
}


//*************************************************************************************************************

int AverageModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}


//*************************************************************************************************************

QVariant AverageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    //Return the number and description/comment of the fiff evoked data in the set as vertical header
    if(orientation == Qt::Vertical) {
        if(section<m_pEvokedDataSet->evoked.size())
            return QString("Set %1").arg(section);
    }

    //Return the horizontal header
    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0:
                return QString("%1").arg("Comment");
                break;

            case 1:
                return QString("%1").arg("Aspect kind");
                break;

            case 2:
                return QString("%1").arg("First sample");
                break;

            case 3:
                return QString("%1").arg("Last sample");
                break;

            case 4:
                return QString("%1").arg("Data types");
                break;
        }
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant AverageModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= m_pEvokedDataSet->evoked.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (evoked set comment) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pEvokedDataSet->evoked.at(index.row()).comment));
                    return v;
                    break;

                case AverageModelRoles::GetComment:
                    v.setValue(m_pEvokedDataSet->evoked.at(index.row()).comment);
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** second column (evoked set aspect kind) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pEvokedDataSet->evoked.at(index.row()).aspect_kind));
                    return v;
                    break;

                case AverageModelRoles::GetAspectKind:
                    v.setValue(m_pEvokedDataSet->evoked.at(index.row()).aspect_kind);
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** third column (evoked set first sample) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pEvokedDataSet->evoked.at(index.row()).first));
                    return v;
                    break;

            case AverageModelRoles::GetFirstSample:
                v.setValue(m_pEvokedDataSet->evoked.at(index.row()).first);
                return v;
                break;

            case Qt::TextAlignmentRole:
                return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fourth column (evoked set last sample) ********
        if(index.column()==3) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pEvokedDataSet->evoked.at(index.row()).last));
                    return v;
                    break;

            case AverageModelRoles::GetLastSample:
                v.setValue(m_pEvokedDataSet->evoked.at(index.row()).last);
                return v;
                break;

            case Qt::TextAlignmentRole:
                return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fifth column (evoked set data types) ********
        if(index.column()==4) {
            QVariant v;
            RowVectorPair averagedData;
            const FiffInfo *fiffInfo;
            RowVectorPairF timeData;
            RowVectorPair projections;

            switch(role) {
                case AverageModelRoles::GetAverageData:
                    averagedData.first = m_pEvokedDataSet->evoked.at(index.row()).data.data();
                    averagedData.second = m_pEvokedDataSet->evoked.at(index.row()).data.cols();
                    v.setValue(averagedData);
                    break;

                case AverageModelRoles::GetFiffInfo:
                    fiffInfo = &m_pEvokedDataSet->evoked.at(index.row()).info;
                    v.setValue(fiffInfo);
                    break;

                case AverageModelRoles::GetTimeData:
                    timeData.first = m_pEvokedDataSet->evoked.at(index.row()).times.data();
                    timeData.second = m_pEvokedDataSet->evoked.at(index.row()).times.cols();
                    v.setValue(timeData);
                    break;

                case AverageModelRoles::GetProjections:
                    projections.first = m_pEvokedDataSet->evoked.at(index.row()).proj.data();
                    projections.second = m_pEvokedDataSet->evoked.at(index.row()).proj.cols();
                    v.setValue(projections);
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }

            return v;
        }//end column check
    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool AverageModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

bool AverageModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

Qt::ItemFlags AverageModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}


//*************************************************************************************************************

bool AverageModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

bool AverageModel::loadEvokedData(QFile& qFile)
{
    beginResetModel();
    clearModel();

    FiffEvokedSet::read(qFile, *m_pEvokedDataSet.data());

    qDebug()<<"m_pEvokedDataSet->evoked.size()"<<m_pEvokedDataSet->evoked.size();

    if(!m_pEvokedDataSet->evoked.empty())
        m_bFileloaded = true;
    else {
        qDebug("AverageModel: ERROR! Data set does not contain any evoked data!");
        endResetModel();
        m_bFileloaded = false;
        emit fileLoaded(false);
        return false;
    }

    endResetModel();

    emit fileLoaded(true);
    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));

    return true;
}


//*************************************************************************************************************

bool AverageModel::saveEvokedData(QFile& qFile)
{
    Q_UNUSED(qFile);

    beginResetModel();
    clearModel();

    //TODO: Save evoked to file

    endResetModel();
    return true;
}


//*************************************************************************************************************

const FiffInfo AverageModel::getFiffInfo()
{
    return m_pEvokedDataSet->info;
}


//*************************************************************************************************************

void AverageModel::clearModel()
{
    beginResetModel();
    //clear average data model structure
    m_pEvokedDataSet->clear();

    m_bFileloaded = false;

    endResetModel();

    qDebug("AverageModel cleared.");
}
