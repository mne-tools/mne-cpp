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
    if(!m_fiffInfo.chs.size()==0)
        return m_fiffInfo.chs.size();
    else
        return 0;
}


//*************************************************************************************************************

int ChInfoModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 7;
}


//*************************************************************************************************************

QVariant ChInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    //Return the number and description/comment of the fiff evoked data in the set as vertical header
    if(orientation == Qt::Vertical) {
        if(section<m_fiffInfo.chs.size())
            return QString("Set %1").arg(section);
    }

    //Return the horizontal header
    if(orientation == Qt::Horizontal) {
        switch(section) {
            case 0:
                return QString("%1").arg("Original name");
                break;

            case 1:
                return QString("%1").arg("Mapped layout name");
                break;

            case 2:
                return QString("%1").arg("Data number");
                break;

            case 3:
                return QString("%1").arg("Channel kind");
                break;

            case 4:
                return QString("%1").arg("MEG type");
                break;

            case 5:
                return QString("%1").arg("Unit");
                break;

            case 6:
                return QString("%1").arg("Position");
                break;
        }
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant ChInfoModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= m_fiffInfo.chs.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (Original channel name) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_fiffInfo.chs.at(index.row()).ch_name));
                    return v;
                    break;

                case ChInfoModelRoles::GetOrigChName:
                    v.setValue(QString("%1").arg(m_fiffInfo.chs.at(index.row()).ch_name));
                    return v;
                    break;
            }
        }//end column check

        //******** second column (mapped layout channel name) ********
        if(index.column()==1) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_mappedLayoutChNames.at(index.row())));
                    return v;
                    break;

                case ChInfoModelRoles::GetMappedLayoutChName:
                    v.setValue(QString("%1").arg(m_mappedLayoutChNames.at(index.row())));
                    return v;
                    break;
            }
        }//end column check

        //******** third column (channel number - corresponds to row in fiff data matrix) ********
        if(index.column()==2) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(index.row());
                    return v;
                    break;

                case ChInfoModelRoles::GetChNumber:
                    v.setValue(index.row());
                    return v;
                    break;
            }
        }//end column check

        //******** fourth column (channel kind - MEG, EEG, etc) ********
        if(index.column()==3) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_fiffInfo.chs.at(index.row()).kind));
                    return v;
                    break;

                case ChInfoModelRoles::GetChKind:
                    v.setValue(m_fiffInfo.chs.at(index.row()).kind);
                    return v;
                    break;
            }
        }//end column check

        //******** fifth column (MEG type) ********
        if(index.column()==4) {
            QVariant v;

            switch(m_fiffInfo.chs.at(index.row()).kind) {
                case FIFFV_MEG_CH: {
                    qint32 unit = m_fiffInfo.chs.at(index.row()).unit;
                    if(unit == FIFF_UNIT_T_M) {
                        v.setValue(QString("%1").arg("MEG_grad"));
                    }
                    else if(unit == FIFF_UNIT_T)
                        v.setValue(QString("%1").arg("MEG_mag"));
                    break;
                }

                default: {
                    v.setValue(QString("%1").arg("non_MEG"));
                    break;
                }
            }

            switch(role) {
                case Qt::DisplayRole:
                    return v;
                    break;

                case ChInfoModelRoles::GetMEGType:
                    return v;
                    break;
            }

            return v;
        }//end column check

        //******** sixth column (channel unit) ********
        if(index.column()==5) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_fiffInfo.chs.at(index.row()).unit));
                    return v;
                    break;

                case ChInfoModelRoles::GetChUnit:
                    v.setValue(m_fiffInfo.chs.at(index.row()).unit);
                    return v;
                    break;
            }
        }//end column check

        //******** sixth column (channel alias) ********
        if(index.column()==5) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(QString("%1").arg(m_aliasNames.at(index.row())));
                    return v;
                    break;

                case ChInfoModelRoles::GetChAlias:
                    v.setValue(m_aliasNames.at(index.row()));
                    return v;
                    break;
            }
        }//end column check

        //******** seventh column (channel layout position) ********
        if(index.column()==6) {
            QVariant v;

            QPointF point = m_layoutMap[m_mappedLayoutChNames.at(index.row())];

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(point);
                    return v;
                    break;

                case ChInfoModelRoles::GetChPosition:
                    v.setValue(point);
                    return v;
                    break;
            }
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

bool ChInfoModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

void ChInfoModel::initData(const FiffInfo &fiffInfo, const QMap<QString,QPointF> &layoutMap)
{
    //Clear data
    clearModel();

    //Set fiff info
    m_fiffInfo = fiffInfo;

    //Set and if necessary map layout to channels
    mapLayoutToChannels(layoutMap);
}


//*************************************************************************************************************

void ChInfoModel::mapLayoutToChannels(const QMap<QString,QPointF> &layoutMap)
{
    m_layoutMap = layoutMap;
}


//*************************************************************************************************************

void ChInfoModel::clearModel()
{
    beginResetModel();

//    m_fiffInfo = FiffInfo();
//    m_layoutMap.clear();
//    m_aliasNames.clear();
//    m_mappedLayoutChNames.clear();

    endResetModel();

    qDebug("ChInfoModel cleared.");
}
