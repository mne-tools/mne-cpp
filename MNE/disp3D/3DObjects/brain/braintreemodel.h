//=============================================================================================================
/**
* @file     braintreemodel.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     BrainTreeModel class declaration.
*
*/

#ifndef BRAINTREEMODEL_H
#define BRAINTREEMODEL_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include "fs/annotationset.h"
#include "fs/surfaceset.h"

#include "brainsurfacetreeitem.h"
#include "brainsurfacesettreeitem.h"

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
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainTreeItem;


//=============================================================================================================
/**
* BrainTreeModel provides a tree based data model to hold all information about brain data which was added to the View 3D.
*
* @brief Provides a tree based data model.
*/
class DISP3DNEWSHARED_EXPORT BrainTreeModel : public QStandardItemModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<BrainTreeModel> SPtr;             /**< Shared pointer type for BrainTreeModel class. */
    typedef QSharedPointer<const BrainTreeModel> ConstSPtr;  /**< Const shared pointer type for BrainTreeModel class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] parent      The parent of the QObject.
    */
    explicit BrainTreeModel(QObject *parent = 0);

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainTreeModel();

    //=========================================================================================================
    /**
    * QStandardItemModel functions
    */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surfaces and annotation SETS to this model.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSurfaceSet        FreeSurfer surface set.
    * @param[in] tAnnotationSet     FreeSurfer annotation set.
    * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const QString& text, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surfaces and annotation data to this model.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSurface           FreeSurfer surface.
    * @param[in] tAnnotation        FreeSurfer annotation.
    * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const QString& text, const Surface& tSurface, const Annotation& tAnnotation, Qt3DCore::QEntity* p3DEntityParent = 0);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surfaces and annotation data to this model.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSourceSpace       The source space information.
    * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const QString& text, const MNESourceSpace& tSourceSpace, Qt3DCore::QEntity* p3DEntityParent);

    //=========================================================================================================
    /**
    * Adds real time source localization data to this model.
    *
    * @param[in] text               The text of the surface set tree item which this data should be added to. If no item with text exists it will be created.
    * @param[in] tSourceEstimate    The real tiem data.
    * @param[in] tForwardSolution   The forward solution.
    *
    * @return                       Returns a list of the rt data tree items. These items should be used to efficienelty update the rt data.
    */
    QList<BrainRTDataTreeItem*> addData(const QString& text, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution = MNEForwardSolution());

private:
    QStandardItem*     m_pRootItem;     /**< The root item of the tree model. */

};

} //NAMESPACE DISP3DLIB

#endif // BRAINTREEMODEL_H
