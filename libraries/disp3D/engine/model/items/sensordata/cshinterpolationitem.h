//=============================================================================================================
/**
* @file     cshinterpolationitem.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief     CshInterpolationItem class declaration.
*
*/

#ifndef DISP3DLIB_CSHINTERPOLATIONITEM_H
#define DISP3DLIB_CSHINTERPOLATIONITEM_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "../common/abstract3Dtreeitem.h"

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
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class CustomMesh;
class CshInterpolationMaterial;

//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
*/

class DISP3DSHARED_EXPORT CshInterpolationItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<CshInterpolationItem> SPtr;            /**< Shared pointer type for CshInterpolationItem. */
    typedef QSharedPointer<const CshInterpolationItem> ConstSPtr; /**< Const shared pointer type for CshInterpolationItem. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] p3DEntityParent    The parent 3D entity.
    * @param[in] iType              The type of the item. See types.h for declaration and definition.
    * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit CshInterpolationItem(Qt3DCore::QEntity* p3DEntityParent = nullptr,
                                  int iType = Data3DTreeModelItemTypes::CshInterpolationItem,
                                  const QString& text = "CshInterpolation");

    //=========================================================================================================
    /**
     * Add interpolation data to this item
     *
     * @param tMneBemSurface        The Bem data.
     * @param tInterpolationMatrix  The weight matrix for interpolation on the bem surface.
     */
    void addData(const MNELIB::MNEBemSurface &tMneBemSurface, QSharedPointer<SparseMatrix<double>> tInterpolationMatrix);

    //=========================================================================================================
    /**
    * This function set the normalization value.
    *
    * @param[in] vecThresholds              The new threshold values used for normalizing the data.
    */
    void setNormalization(const QVector3D& tVecThresholds);

protected:

    //=========================================================================================================
    /**
     * Initialze the Item.
     */
    virtual void initItem() override;


    QPointer<CustomMesh>                    m_pCustomMesh;

    QPointer<Renderable3DEntity>            m_pMeshDrawEntity;

    QPointer<Qt3DCore::QEntity>             m_pComputeEntity;

    QPointer<Qt3DRender::QComputeCommand>   m_pComputeCommand;

    QPointer<Qt3DCore::QTransform>          m_pTransform;

    QPointer<CshInterpolationMaterial>      m_pMaterial;

    //@TODO is something missing?


};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CSHINTERPOLATIONITEM_H
