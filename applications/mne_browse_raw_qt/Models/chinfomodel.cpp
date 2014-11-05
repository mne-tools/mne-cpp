//=============================================================================================================
/**
* @file     chinfomodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     November, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the channel info model of the model/view framework of mne_browse_raw_qt application.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "chinfomodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ChInfoModel::ChInfoModel(QObject *parent)
: QAbstractTableModel(parent)
{
}


//*************************************************************************************************************
//virtual functions
int ChInfoModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Return number of stored evoked sets
//    if(!m_pEvokedDataSet->evoked.size()==0)
//        return m_pEvokedDataSet->evoked.size();
//    else
        return 0;
}


//*************************************************************************************************************

int ChInfoModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}


//*************************************************************************************************************

QVariant ChInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QVariant ChInfoModel::data(const QModelIndex &index, int role) const
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

                case ChInfoModelRoles::GetComment:
                    v.setValue(m_pEvokedDataSet->evoked.at(index.row()).comment);
                    return v;
                    break;
            }
        }//end column check

        //******** second column (evoked set aspect kind) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_pEvokedDataSet->evoked.at(index.row()).aspect_kind));
                    return v;
                    break;

                case ChInfoModelRoles::GetAspectKind:
                    v.setValue(m_pEvokedDataSet->evoked.at(index.row()).aspect_kind);
                    return v;
                    break;
            }
        }//end column check

        //******** third column (evoked set first sample) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("-%1").arg(m_pEvokedDataSet->evoked.at(index.row()).first));
                    return v;
                    break;

            case ChInfoModelRoles::GetFirstSample:
                v.setValue(m_pEvokedDataSet->evoked.at(index.row()).first);
                return v;
                break;
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

            case ChInfoModelRoles::GetLastSample:
                v.setValue(m_pEvokedDataSet->evoked.at(index.row()).last);
                return v;
                break;
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
                case ChInfoModelRoles::GetAverageData:
                    averagedData.first = m_pEvokedDataSet->evoked.at(index.row()).data.data();
                    averagedData.second = m_pEvokedDataSet->evoked.at(index.row()).data.cols();
                    v.setValue(averagedData);
                    break;

                case ChInfoModelRoles::GetFiffInfo:
                    fiffInfo = &m_pEvokedDataSet->evoked.at(index.row()).info;
                    v.setValue(fiffInfo);
                    break;

                case ChInfoModelRoles::GetTimeData:
                    timeData.first = m_pEvokedDataSet->evoked.at(index.row()).times.data();
                    timeData.second = m_pEvokedDataSet->evoked.at(index.row()).times.cols();
                    v.setValue(timeData);
                    break;

                case ChInfoModelRoles::GetProjections:
                    projections.first = m_pEvokedDataSet->evoked.at(index.row()).proj.data();
                    projections.second = m_pEvokedDataSet->evoked.at(index.row()).proj.cols();
                    v.setValue(projections);
                    break;
            }

            return v;
        }//end column check
    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool ChInfoModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

bool ChInfoModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

Qt::ItemFlags ChInfoModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}


//*************************************************************************************************************

bool ChInfoModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

void ChInfoModel::clearModel()
{
    beginResetModel();

    endResetModel();

    qDebug("ChInfoModel cleared.");
}
