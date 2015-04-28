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

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDataModel::FilterDataModel(QObject *parent)
: QAbstractTableModel(parent)
{
}


//*************************************************************************************************************

FilterDataModel::FilterDataModel(QObject *parent, FilterData &dataFilter)
: QAbstractTableModel(parent)
{
    addFilter(dataFilter);
}


//*************************************************************************************************************

FilterDataModel::FilterDataModel(QObject *parent, QList<FilterData> &dataFilter)
: QAbstractTableModel(parent)
{
    addFilter(dataFilter);
}


//*************************************************************************************************************
//virtual functions
int FilterDataModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    //Return number of stored evoked sets
    if(!m_filterData.size()==0)
        return m_filterData.size();
    else
        return 0;
}


//*************************************************************************************************************

int FilterDataModel::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return 9;
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
                        return QString("%1").arg("State");
                        break;

                    case 1:
                        return QString("%1").arg("Name");
                        break;

                    case 2:
                        return QString("%1").arg("Type");
                        break;

                    case 3:
                        return QString("%1").arg("HP (Hz)");
                        break;

                    case 4:
                        return QString("%1").arg("LP (Hz)");
                        break;

                    case 5:
                        return QString("%1").arg("Order");
                        break;

                    case 6:
                        return QString("%1").arg("sFreq");
                        break;

                    case 7:
                        return QString("%1").arg("Filter");
                        break;

                    case 8:
                        return QString("%1").arg("ActiveFilters");
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
    if(role!=Qt::DisplayRole || role!=Qt::TextAlignmentRole)
        if(role<Qt::UserRole + 1009 && role > Qt::UserRole + 1017)
            return QVariant();

    if(index.row() >= m_filterData.size() || index.column()>=columnCount())
        return QVariant();

    if (index.isValid()) {
        //******** zeroth column (filter state) ********
        if(index.column()==0) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_isActive.at(index.row()));
                    return v;

                case FilterDataModelRoles::GetFilterName:
                    v.setValue(m_isActive.at(index.row()));
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** first column (filter name) ********
        if(index.column()==1) {
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
        if(index.column()==2) {
            QVariant v;

            QString filterType = FilterData::getStringForFilterType(m_filterData.at(index.row()).m_Type);

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

        //******** third column (HP (Hz)) ********
        if(index.column()==3) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_filterData.at(index.row()).m_dHighpassFreq);
                    return v;

                case FilterDataModelRoles::GetFiltertHP:
                    v.setValue(m_filterData.at(index.row()).m_dHighpassFreq);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fourth column (LP (Hz)) ********
        if(index.column()==4) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_filterData.at(index.row()).m_dLowpassFreq);
                    return v;

                case FilterDataModelRoles::GetFiltertLP:
                    v.setValue(m_filterData.at(index.row()).m_dLowpassFreq);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** fifth column (Order) ********
        if(index.column()==5) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_filterData.at(index.row()).m_iFilterOrder);
                    return v;

                case FilterDataModelRoles::GetFiltertOrder:
                    v.setValue(m_filterData.at(index.row()).m_iFilterOrder);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check

        //******** sixth column (sFreq) ********
        if(index.column()==6) {
            QVariant v;

            switch(role) {
                case Qt::DisplayRole:
                    v.setValue(m_filterData.at(index.row()).m_sFreq);
                    return v;

                case FilterDataModelRoles::GetFilterSamplingFrequency:
                    v.setValue(m_filterData.at(index.row()).m_sFreq);
                    return v;

                case Qt::TextAlignmentRole:
                    return Qt::AlignHCenter + Qt::AlignVCenter;
            }
        }//end column check


        //******** seventh column (Filter data) ********
        if(index.column()==7 && FilterDataModelRoles::GetFilter) {
            QVariant v;
            v.setValue(m_filterData.at(index.row()));
            return v;
        }//end column check

        //******** eigth column (Active Filters) ********
        if(index.column()==8 && FilterDataModelRoles::GetActiveFilters) {
            QVariant v;

            QList<FilterData> activeFilters;
            for(int i=0; i<m_filterData.size(); i++)
                if(m_isActive.at(i))
                    activeFilters.append(m_filterData.at(i));

            v.setValue(activeFilters);
            return v;
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
    if(index.column() == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


//*************************************************************************************************************

bool FilterDataModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(index.row()>=rowCount() || index.column()>=columnCount())
        return false;

    std::cout<<"setting active flag for"<<index.row()<<std::endl;

    if(index.column() == 0 && role == Qt::EditRole)
        m_isActive[index.row()] = value.toBool();

    return true;
}


//*************************************************************************************************************

void FilterDataModel::addFilter(const QList<FilterData>& dataFilter)
{
    m_filterData.append(dataFilter);

    //set inactive by default
    for(int i=0; i<dataFilter.size(); i++)
        m_isActive.append(false);

    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, rowCount());
}


//*************************************************************************************************************

void FilterDataModel::addFilter(const FilterData &dataFilter)
{
    m_filterData.append(dataFilter);

    //set inactive by default
    m_isActive.append(false);

    emit dataChanged(createIndex(0,0), createIndex(rowCount(),columnCount()));
    emit headerDataChanged(Qt::Vertical, 0, rowCount());
}



//*************************************************************************************************************

void FilterDataModel::clearModel()
{
    beginResetModel();

    m_filterData.clear();
    m_isActive.clear();

    endResetModel();

    qDebug("FilterDataModel cleared.");
}
