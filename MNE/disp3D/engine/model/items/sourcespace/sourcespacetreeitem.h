//=============================================================================================================
/**
* @file     sourcespacetreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
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
* @brief     SourceSpaceTreeItem class declaration.
*
*/

#ifndef SOURCESPACETREEITEM_H
#define SOURCESPACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstractsurfacetreeitem.h"
#include "../common/types.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEHemisphere;
}

namespace Qt3DCore {
    class QEntity;
}


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

class Renderable3DEntity;


//=============================================================================================================
/**
* SourceSpaceTreeItem provides a generic brain tree item to hold data about the source space.
*
* @brief Provides a generic brain tree item.
*/
class DISP3DNEWSHARED_EXPORT SourceSpaceTreeItem : public AbstractSurfaceTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SourceSpaceTreeItem> SPtr;             /**< Shared pointer type for SourceSpaceTreeItem class. */
    typedef QSharedPointer<const SourceSpaceTreeItem> ConstSPtr;  /**< Const shared pointer type for SourceSpaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit SourceSpaceTreeItem(int iType = Data3DTreeModelItemTypes::SourceSpaceItem, const QString& text = "Source space");

    //=========================================================================================================
    /**
    * Default destructor
    */
    ~SourceSpaceTreeItem();

    //=========================================================================================================
    /**
    * Adds source space data.
    *
    * @param[in] tHemisphere        The hemisphere data of the source space.
    * @param[in] parent             The Qt3D entity parent of the new item.
    */
    void addData(const MNELIB::MNEHemisphere& tHemisphere, Qt3DCore::QEntity* parent);

protected:
    //=========================================================================================================
    /**
    * Plots the sources a spheres.
    *
    * @param[in] tHemisphere        The hemisphere data of the source space.
    */
    void plotSources(const MNELIB::MNEHemisphere& tHemisphere);

    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();

    QList<QPointer<Renderable3DEntity> >        m_lSpheres;                 /**< The currently displayed source points as 3D spheres. */

};

} //NAMESPACE DISP3DLIB

#endif // SOURCESPACETREEITEM_H
