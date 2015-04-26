//=============================================================================================================
/**
* @file     filterdatamodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     April, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    This class represents the filter data model, which is used to hold different filter types in form of a convenient MVC structure.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdatamodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDataModel::FilterDataModel(QObject *parent)
: QAbstractTableModel(parent)
{
}


//*************************************************************************************************************
//virtual functions
int FilterDataModel::rowCount(const QModelIndex & /*parent*/) const
{
    //Return number of stored evoked sets
    if(!m_filterData.size()==0)
        return m_filterData.size();
    else
        return 0;
}


//*************************************************************************************************************

int FilterDataModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 5;
}


//*************************************************************************************************************

QVariant FilterDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole && role != Qt::TextAlignmentRole)
        return QVariant();

    switch(role) {
        case Qt::TextAlignmentRole:
            return Qt::AlignHCenter + Qt::AlignVCenter;

        case Qt::DisplayRole: {
            //Return the number and description/comment of the fiff evoked data in the set as vertical header
            if(orientation == Qt::Vertical)
                if(section<m_filterData.size())
                    return QString("%1").arg(section);

            //Return the horizontal header
            if(orientation == Qt::Horizontal) {
                switch(section) {
                    case 0:
                        return QString("%1").arg("Name");
                        break;

                    case 1:
                        return QString("%1").arg("Type");
                        break;

                    case 2:
                        return QString("%1").arg("HP (Hz)");
                        break;

                    case 3:
                        return QString("%1").arg("LP (Hz)");
                        break;

                    case 4:
                        return QString("%1").arg("Order");
                        break;

                    case 5:
                        return QString("%1").arg("sFreq");
                        break;

                    case 6:
                        return QString("%1").arg("State");
                        break;
                }
            }
        }
    }

    return QVariant();
}


//*************************************************************************************************************

QVariant FilterDataModel::data(const QModelIndex &index, int role) const
{
    if(index.row() >= m_filterData.size())
        return QVariant();

    if (index.isValid()) {
        //******** first column (filter name) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_filterData.at(index.row()).m_sName);
                    return v;

                case FilterDataModelRoles::GetFilterName:
                    v.setValue(m_filterData.at(index.row()).m_sName);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** second column (Filter type) ********
        if(index.column()==1) {
            QVariant v;

            QString filterType("Unknown");

            if(m_filterData.at(index.row()).m_Type == FilterData::HPF)
                filterType = "HP";
            if(m_filterData.at(index.row()).m_Type == FilterData::LPF)
                filterType = "LP";
            if(m_filterData.at(index.row()).m_Type == FilterData::BPF)
                filterType = "BP";
            if(m_filterData.at(index.row()).m_Type == FilterData::NOTCH)
                filterType = "NOTCH";

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(filterType);
                    return v;

                case FilterDataModelRoles::GetFilterType:
                    v.setValue(filterType);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check
    } // end index.valid() check

    return QVariant();
}


//*************************************************************************************************************

bool FilterDataModel::insertRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

bool FilterDataModel::removeRows(int position, int span, const QModelIndex & parent)
{
    Q_UNUSED(position);
    Q_UNUSED(span);
    Q_UNUSED(parent);

    return true;
}


//*************************************************************************************************************

Qt::ItemFlags FilterDataModel::flags(const QModelIndex & index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable /*| Qt::ItemIsEditable*/;
}


//*************************************************************************************************************

bool FilterDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

void FilterDataModel::clearModel()
{
    beginResetModel();

    m_filterData.clear();

    endResetModel();

    qDebug("FilterDataModel cleared.");
}
