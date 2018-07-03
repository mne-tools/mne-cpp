//=============================================================================================================
/**
* @file     filterdatamodel.h
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

#ifndef FILTERDATAMODEL_H
#define FILTERDATAMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp_global.h"
#include "utils/filterTools/filterdata.h"
#include "utils/filterTools/filterio.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractTableModel>
#include <QVector3D>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP
//=============================================================================================================

namespace DISPLIB
{

//Declare type roles
namespace FilterDataModelRoles
{
    enum ItemRole{GetFilterName = Qt::UserRole + 1009,
                    GetFilterType = Qt::UserRole + 1010,
                    GetFiltertHP = Qt::UserRole + 1011,
                    GetFiltertLP = Qt::UserRole + 1012,
                    GetFiltertOrder = Qt::UserRole + 1013,
                    GetFilterSamplingFrequency = Qt::UserRole + 1014,
                    GetFilterState = Qt::UserRole + 1015,
                    GetFilter = Qt::UserRole + 1016,
                    GetActiveFilters = Qt::UserRole + 1017,
                    GetAllFilters = Qt::UserRole + 1018,
                    SetUserDesignedFilter = Qt::UserRole + 1019};
}

//=============================================================================================================
/**
* DECLARE CLASS FilterDataModel
*/
class DISPSHARED_EXPORT FilterDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<FilterDataModel> SPtr;            /**< Shared pointer type for FilterDataModel class. */
    typedef QSharedPointer<const FilterDataModel> ConstSPtr; /**< Const shared pointer type for FilterDataModel class. */

    FilterDataModel(QObject *parent = 0);
    FilterDataModel(QObject *parent, FilterData &dataFilter);
    FilterDataModel(QObject *parent, QList<FilterData> &dataFilter);

    //=========================================================================================================
    /**
    * Reimplemented virtual functions
    *
    */
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex & index) const;
    virtual bool insertRows(int position, int span, const QModelIndex & parent = QModelIndex());
    virtual bool removeRows(int position, int span, const QModelIndex & parent = QModelIndex());

    //=========================================================================================================
    /**
    * addFilter adds filter to the model data.
    *
    * @param dataFilter filter list with already loaded filters.
    */
    void addFilter(const QList<FilterData>& dataFilter);

    //=========================================================================================================
    /**
    * addFilter adds filter to the model data
    *
    * @param dataFilter filter data with an already loaded filter.
    */
    void addFilter(const FilterData &dataFilter);

    //=========================================================================================================
    /**
    * getUserDesignedFilterIndex returns the index of the user designed filter.
    *
    * @return index of the user designed filter.
    */
    int getUserDesignedFilterIndex();

protected:
    //=========================================================================================================
    /**
    * clearModel clears all model's members
    *
    */
    void clearModel();

    QList<FilterData>       m_filterData;           /**< list of the loaded filters and their data. */
    QList<bool>             m_isActive;             /**< list of the current activation state of the filters. */

    int                     m_iDesignFilterIndex;   /**< index of the user designed filter. */
};

} // NAMESPACE DISPLIB

Q_DECLARE_METATYPE(FilterData);
Q_DECLARE_METATYPE(QList<FilterData>);

#endif // FILTERDATAMODEL_H

