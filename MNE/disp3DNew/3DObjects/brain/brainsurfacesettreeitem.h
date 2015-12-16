//=============================================================================================================
/**
* @file     brainsurfacesettreeitem.h
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
* @brief     BrainSurfaceSetTreeItem class declaration.
*
*/

#ifndef BRAINSURFACESETTREEITEM_H
#define BRAINSURFACESETTREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3DNew_global.h"

#include "../../helpers/abstracttreeitem.h"
#include "brainhemispheretreeitem.h"

#include "fs/label.h"
#include "fs/annotationset.h"
#include "fs/surfaceset.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DNEWLIB
//=============================================================================================================

namespace DISP3DNEWLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* BrainSurfaceSetTreeItem provides a generic brain tree item to hold of brain data (hemi, vertices, tris, etc.) from different sources (FreeSurfer, etc.).
*
* @brief Provides a generic BrainSurfaceSetTreeItem.
*/
class DISP3DNEWSHARED_EXPORT BrainSurfaceSetTreeItem : public AbstractTreeItem
{

public:
    typedef QSharedPointer<BrainSurfaceSetTreeItem> SPtr;             /**< Shared pointer type for BrainSurfaceSetTreeItem class. */
    typedef QSharedPointer<const BrainSurfaceSetTreeItem> ConstSPtr;  /**< Const shared pointer type for BrainSurfaceSetTreeItem class. */

    //=========================================================================================================
    /**
    * FreeSurfer constructor from single surface.
    */
    explicit BrainSurfaceSetTreeItem(const int& iType = BrainTreeModelItemTypes::UnknownItem, const QString & text = "");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~BrainSurfaceSetTreeItem();

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    QVariant data(int role = Qt::UserRole + 1) const;
    void setData(const QVariant& value, int role = Qt::UserRole + 1);

    //=========================================================================================================
    /**
    * Adds FreeSurfer data based on surfaces and annotation SETS to this model.
    *
    * @param[in] tSurfaceSet        FreeSurfer surface set.
    * @param[in] tAnnotationSet     FreeSurfer annotation set.
    * @param[in] p3DEntityParent    The Qt3D entity parent of the new item.
    *
    * @return                       Returns true if successful.
    */
    bool addData(const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity *p3DEntityParent = 0);

    bool addData(const Surface& tSurface, const Annotation& tAnnotation, Qt3DCore::QEntity* p3DEntityParent = 0);

private:

};

} //NAMESPACE DISP3DNEWLIB

#endif // BRAINSURFACESETTREEITEM_H
