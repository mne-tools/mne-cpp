//=============================================================================================================
/**
* @file     projectionmodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     October, 2014
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
* @brief    This class represents the projection model of the model/view framework of mne_browse_raw_qt application.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "projectionmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProjectionModel::ProjectionModel(QObject *parent)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
{
}


//*************************************************************************************************************

ProjectionModel::ProjectionModel(QObject *parent, QFile& qFile)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
{
    //read projections from fif file
    loadProjections(qFile);
}


//*************************************************************************************************************

ProjectionModel::ProjectionModel(QObject *parent, QList<FiffProj>& dataProjs)
: QAbstractTableModel(parent)
, m_bFileloaded(false)
{
    addProjections(dataProjs);
}


//*************************************************************************************************************
//virtual functions
int ProjectionModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Return number of stored evoked sets
    if(m_dataProjs.size()!=0)
        return m_dataProjs.size();
    else
        return 0;
}


//*************************************************************************************************************

int ProjectionModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 3;
}


//*************************************************************************************************************

QVariant ProjectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    //Return the number and description/comment of the fiff evoked data in the set as vertical header
    if(orientation == Qt::Vertical) {
        if(section<m_dataProjs.size())
            return QString("%1").arg(section);
    }

    if(role==Qt::TextAlignmentRole)
        return Qt::AlignHCenter + Qt::AlignVCenter;

    //Return the horizontal header
    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0:
                return QString("%1").arg("Name");
                break;

            case 1:
                return QString("%1").arg("State");
                break;

            case 2:
                return QString("%1").arg("Dimension");
                break;

            case 3:
                return QString("%1").arg("Data");
                break;
        }
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant ProjectionModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= m_dataProjs.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (projection name) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_dataProjs.at(index.row()).desc));
                    return v;
                    break;

                case ProjectionModelRoles::GetProjectionName:
                    v.setValue(m_dataProjs.at(index.row()).desc);
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** second column (project state - active or inactive) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    if(m_dataProjs.at(index.row()).active)
                        v.setValue(QString("%1").arg("Active"));
                    else
                        v.setValue(QString("%1").arg("Inactive"));
                    return v;
                    break;

                case ProjectionModelRoles::GetProjectionState:
                    v.setValue(m_dataProjs.at(index.row()).active);
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** third column (projection data size / dimensions) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("(%1|%2)").arg(m_dataProjs.at(index.row()).data->data.rows()).arg(m_dataProjs.at(index.row()).data->data.cols()));
                    return v;
                    break;

                case ProjectionModelRoles::GetProjectionDimension:
                    v.setValue(QPair<int,int>(m_dataProjs.at(index.row()).data->data.rows(), m_dataProjs.at(index.row()).data->data.cols()));
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fourth column (projection data) ********
        if(index.column()==3) {
            QVariant v;

            switch(role) {
                case ProjectionModelRoles::GetProjectionData:
                    v.setValue(m_dataProjs.at(index.row()).desc);
                    return v;
                    break;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check
    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool ProjectionModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

bool ProjectionModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

Qt::ItemFlags ProjectionModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}


//*************************************************************************************************************

bool ProjectionModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

bool ProjectionModel::loadProjections(QFile& qFile)
{
    beginResetModel();
    clearModel();

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&qFile));
    QString t_sFileName = t_pStream->streamName();

    qDebug()<<"Opening header data %s...\n"<<t_sFileName.toUtf8().constData();

    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;

    if(!t_pStream->open(t_Tree, t_Dir))
        return false;

    QList<FiffProj> q_ListProj = t_pStream->read_proj(t_Tree);

    if (q_ListProj.size() == 0)
    {
        qDebug()<<"Could not find projectors\n";
        return false;
    }

    m_dataProjs.append(q_ListProj);

    //garbage collecting
    t_pStream->device()->close();

    endResetModel();

    emit fileLoaded(true);
    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, rowCount());

    return true;
}


//*************************************************************************************************************

bool ProjectionModel::saveProjections(QFile& qFile)
{
    Q_UNUSED(qFile);

    beginResetModel();
    clearModel();

    //TODO: Save projections to file

    endResetModel();
    return true;
}


//*************************************************************************************************************

void ProjectionModel::addProjections(const QList<FiffProj>& dataProjs)
{
    m_dataProjs.append(dataProjs);

    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, rowCount());
}


//*************************************************************************************************************

void ProjectionModel::addProjections(FiffInfo::ConstSPtr pFiffInfo)
{
    m_dataProjs.append(pFiffInfo->projs);

    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, rowCount());
}


//*************************************************************************************************************

void ProjectionModel::clearModel()
{
    beginResetModel();

    m_dataProjs.clear();

    m_bFileloaded = false;

    endResetModel();

    qDebug("ProjectionModel cleared.");
}
