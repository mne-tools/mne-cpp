//=============================================================================================================
/**
* @file     subjecttreemodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
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
* @brief     SubjectTreeModel class declaration.
*
*/

#ifndef SUBJECTTREEMODEL_H
#define SUBJECTTREEMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "fs/annotationset.h"
#include "fs/surfaceset.h"

#include "../brain/brainsurfacetreeitem.h"
#include "../brain/brainsurfacesettreeitem.h"

#include "../../helpers/renderable3Dentity.h"

#include "mne/mne_forwardsolution.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QStringList>
#include <QStandardItemModel>

#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainTreeItem;


//=============================================================================================================
/**
* SubjectTreeModel provides a tree based data model to hold all information about brain data which was added to the View 3D.
*
* @brief Provides a tree based data model.
*/
class DISP3DNEWSHARED_EXPORT SubjectTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<SubjectTreeModel> SPtr;             /**< Shared pointer type for SubjectTreeModel class. */
    typedef QSharedPointer<const SubjectTreeModel> ConstSPtr;  /**< Const shared pointer type for SubjectTreeModel class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit SubjectTreeModel(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~SubjectTreeModel();

    //=========================================================================================================
    /**
    * QStandardItemModel functions
    */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;    
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QStandardItem*     m_pRootItem;     /**< The root item of the tree model. */
private:

};

} //NAMESPACE DISP3DLIB

#endif // SubjectTreeModel_H
