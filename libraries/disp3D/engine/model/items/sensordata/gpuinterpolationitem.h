//=============================================================================================================
/**
* @file     gpuinterpolationitem.h
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
* @brief     GpuInterpolationItem class declaration.
*
*/

#ifndef DISP3DLIB_GPUINTERPOLATIONITEM_H
#define DISP3DLIB_GPUINTERPOLATIONITEM_H


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
class GpuInterpolationMaterial;


//=============================================================================================================
/**
* This item is used for signal interpolation with a compute shader.
* It stores all Qt3DEnities needed for this process.
*
* @brief Signal interpolation with qt3d compute shader.
*/

class DISP3DSHARED_EXPORT GpuInterpolationItem : public Abstract3DTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<GpuInterpolationItem> SPtr;            /**< Shared pointer type for GpuInterpolationItem. */
    typedef QSharedPointer<const GpuInterpolationItem> ConstSPtr; /**< Const shared pointer type for GpuInterpolationItem. */

    //=========================================================================================================
    /**
    * Default constructor.
    *
    * @param[in] p3DEntityParent    The parent 3D entity.
    * @param[in] iType              The type of the item. See types.h for declaration and definition.
    * @param[in] text               The text of this item. This is also by default the displayed name of the item in a view.
    */
    explicit GpuInterpolationItem(Qt3DCore::QEntity* p3DEntityParent = nullptr,
                                  int iType = Data3DTreeModelItemTypes::GpuInterpolationItem,
                                  const QString& text = "3D Plot");

    //=========================================================================================================
    /**
     * Initialize interpolation data of this item.
     *
     * @param tMneBemSurface        The bem surface data.
     */
    void initData(const MNELIB::MNEBemSurface &tMneBemSurface);

    //=========================================================================================================
    /**
     * Set the new weight matrix for the interpolation.
     *
     * @param pInterpolationMatrix  The new weight matrix for interpolation on the bem surface.
     */
    void setWeightMatrix(QSharedPointer<SparseMatrix<float>> pInterpolationMatrix);

    //=========================================================================================================
    /**
    * Add a new vector with signal data form the sensors.
    *
    * @param tSignalVec              Vector with one float value for each sensor.
    */
    void addNewRtData(const Eigen::VectorXf &tSignalVec);

    //=========================================================================================================
    /**
    * This function set the normalization value.
    *
    * @param[in] vecThresholds       The new threshold values used for normalizing the data.
    */
    void setNormalization(const QVector3D& tVecThresholds);

    //=========================================================================================================
    /**
     * This function sets the colormap type
     *
     * @param tColormapType           The new colormap name.
     */
    void setColormapType(const QString& tColormapType);

protected:

    //=========================================================================================================
    /**
     * Initialze the Item.
     */
    virtual void initItem() override;

    bool                                    m_bIsDataInit;          /**< The initialization flag. */

    QPointer<CustomMesh>                    m_pCustomMesh;          /**< Stores 3D data of the surfce. */

    QPointer<Qt3DCore::QEntity>             m_pMeshDrawEntity;      /**< Top level Entity for the draw part. */

    QPointer<Qt3DCore::QEntity>             m_pComputeEntity;       /**< Top level Entity for the compute part. */

    QPointer<Qt3DRender::QComputeCommand>   m_pComputeCommand;      /**< This component issues work for the csh to the gpu. */

    QPointer<GpuInterpolationMaterial>      m_pMaterial;            /**< Compute material used for the process. */

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_GPUINTERPOLATIONITEM_H
