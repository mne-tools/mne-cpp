//=============================================================================================================
/**
* @file     cpuinterpolationitem.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2017
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
* @brief     CpuInterpolationItem class declaration.
*
*/

#ifndef DISP3DLIB_CPUINTERPOLATIONITEM_H
#define DISP3DLIB_CPUINTERPOLATIONITEM_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstractmeshtreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/SparseCore>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB{
    class MNEBemSurface;
}

namespace Qt3DRender {
    class QComputeCommand;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class CustomMesh;
class GpuInterpolationMaterial;


//=============================================================================================================
/**
* This item is used for signal interpolation with CPU support.
*
* @brief This item is used for signal interpolation with CPU support.
*/

class DISP3DSHARED_EXPORT CpuInterpolationItem : public AbstractMeshTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<CpuInterpolationItem> SPtr;            /**< Shared pointer type for CpuInterpolationItem. */
    typedef QSharedPointer<const CpuInterpolationItem> ConstSPtr; /**< Const shared pointer type for CpuInterpolationItem. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] p3DEntityParent    The parent 3D entity.
    * @param[in] iType              The type of the item. See types.h for declaration and definition.
    * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit CpuInterpolationItem(Qt3DCore::QEntity* p3DEntityParent = nullptr,
                                  int iType = Data3DTreeModelItemTypes::CpuInterpolationItem,
                                  const QString& text = "3D Plot");

    //=========================================================================================================
    /**
    * Init the item's data.
    *
    * @param[in] tBemSurface        The bem data.
    */
    void initData(const MNELIB::MNEBemSurface &tBemSurface);

protected:
    //=========================================================================================================
    /**
     * Initialze the Item.
     */
    virtual void initItem() override;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CPUINTERPOLATIONITEM_H
