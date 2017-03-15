//=============================================================================================================
/**
* @file     sensorsurfacetreeitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief     SensorSurfaceTreeItem class declaration.
*
*/

#ifndef SENSORSURFACETREEITEM_H
#define SENSORSURFACETREEITEM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"

#include "../common/abstractsurfacetreeitem.h"
#include "../common/types.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNEBemSurface;
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
* SensorSurfaceTreeItem provides a generic brain tree item to hold sensor surfaces.
*
* @brief SensorSurfaceTreeItem provides a generic brain tree item to hold sensor surfaces.
*/
class DISP3DNEWSHARED_EXPORT SensorSurfaceTreeItem : public AbstractSurfaceTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<SensorSurfaceTreeItem> SPtr;             /**< Shared pointer type for SensorSurfaceTreeItem class. */
    typedef QSharedPointer<const SensorSurfaceTreeItem> ConstSPtr;  /**< Const shared pointer type for SensorSurfaceTreeItem class. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] iType      The type of the item. See types.h for declaration and definition.
    * @param[in] text       The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit SensorSurfaceTreeItem(int iType = Data3DTreeModelItemTypes::SensorSurfaceItem, const QString& text = "Sensor Surface");

    //=========================================================================================================
    /**
    * Adds BEM model data.
    *
    * @param[in] tSensorSurface     The bem data.
    * @param[in] parent             The Qt3D entity parent of the new item.
    */
    void addData(const MNELIB::MNEBemSurface &tSensorSurface, Qt3DCore::QEntity* parent);

protected:
    //=========================================================================================================
    /**
    * AbstractTreeItem functions
    */
    void initItem();

};

} //NAMESPACE DISP3DLIB

#endif // SENSORSURFACETREEITEM_H
