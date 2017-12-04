//=============================================================================================================
/**
* @file     gpusensordatatreeitem.h
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
* @brief     GpuSensorDataTreeItem class declaration.
*
*/

#ifndef DISP3DLIB_GPUSENSORDATATREEITEM_H
#define DISP3DLIB_GPUSENSORDATATREEITEM_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../../../disp3D_global.h"
#include "sensordatatreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB {


//*************************************************************************************************************
//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class GpuInterpolationItem;


//=============================================================================================================
/**
* This item allows on-the-fly changes to parameters of visualization. It integrates the features provided in
* GeometryInfo and and uses compute shaders for the interpolation.
*
* @brief This item integrates GeometryInfo and compute shader interpolation into Disp3D structure.
*/
class DISP3DSHARED_EXPORT GpuSensorDataTreeItem : public SensorDataTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<GpuSensorDataTreeItem> SPtr;            /**< Shared pointer type for GpuSensorDataTreeItem. */
    typedef QSharedPointer<const GpuSensorDataTreeItem> ConstSPtr; /**< Const shared pointer type for GpuSensorDataTreeItem. */

    //=========================================================================================================
    /**
    * Constructs a GpuSensorDataTreeItem object.
    */
    explicit GpuSensorDataTreeItem(int iType = Data3DTreeModelItemTypes::SensorDataItem,
                                   const QString& text = "Sensor Data");

protected:
    //=========================================================================================================
    /**
    * Set the new interpolation matrix.
    *
    * @param[in] matInterpolationMatrix                 The new interpolation matrix.
    */
    virtual void setInterpolationMatrix(Eigen::SparseMatrix<float> matInterpolationMatrix);

    //=========================================================================================================
    /**
    * Init the interpolation items. This cannot be done here because they might differ from GPU to CPU version.
    *
    * @param[in] bemSurface                 MNEBemSurface that holds the mesh that should be visualized.
    * @param[in] p3DEntityParent            The Qt3D entity parent of the new item.
    */
    virtual void initInterpolationItem(const MNELIB::MNEBemSurface &bemSurface,
                                       Qt3DCore::QEntity* p3DEntityParent) override;

    //=========================================================================================================
    /**
    * This function gets called whenever this item receives sensor values for each estimated source.
    *
    * @param[in] vecDataVector         The streamed raw data.
    */
    void virtual onNewRtRawData(VectorXd vecDataVector);

    //=========================================================================================================
    /**
    * This function gets called whenever the used colormap type changed.
    *
    * @param[in] sColormapType              The name of the new colormap type.
    */
    virtual void onColormapTypeChanged(const QVariant& sColormapType) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the normaization value changed. The normalization value is used to normalize the estimated source activation.
    *
    * @param[in] vecThresholds              The new threshold values used for normalizing the data.
    */
    virtual void onDataThresholdChanged(const QVariant &vecThresholds) override;

    QPointer<GpuInterpolationItem>      m_pInterpolationItem;                   /**< This item manages all 3d rendering and calculations. */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_GPUSENSORDATATREEITEM_H
