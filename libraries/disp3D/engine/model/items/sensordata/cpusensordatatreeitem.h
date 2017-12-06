//=============================================================================================================
/**
* @file     cpusensordatatreeitem.h
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2017
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
* @brief     CpuSensorDataTreeItem class declaration.
*
*/

#ifndef DISP3DLIB_CPUSENSORDATATREEITEM_H
#define DISP3DLIB_CPUSENSORDATATREEITEM_H


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

namespace MNELIB{
    class MNEBemSurface;
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

class CpuInterpolationItem;


//=============================================================================================================
/**
* This item allows on-the-fly changes to parameters of visualization. It integrates the features provided in
* GeometryInfo and uses the cpu for interpolation.
*
* @brief This item integrates GeometryInfo and  Interpolation into Disp3D structure.
*/
class DISP3DSHARED_EXPORT CpuSensorDataTreeItem : public SensorDataTreeItem
{
    Q_OBJECT

public:
    typedef QSharedPointer<CpuSensorDataTreeItem> SPtr;            /**< Shared pointer type for CpuSensorDataTreeItem. */
    typedef QSharedPointer<const CpuSensorDataTreeItem> ConstSPtr; /**< Const shared pointer type for CpuSensorDataTreeItem. */

    //=========================================================================================================
    /**
    * Constructs a CpuSensorDataTreeItem object.
    */
    explicit CpuSensorDataTreeItem(int iType = Data3DTreeModelItemTypes::SensorDataItem,
                                   const QString& text = "Sensor Data");

protected:
    //=========================================================================================================
    /**
    * Init the interpolation items. This cannot be done here because they might differ from GPU to CPU version.
    *
    * @param[in] matVertices       The surface vertices.
    * @param[in] matNormals        The surface normals.
    * @param[in] matTriangles      The surface triangles.
    * @param[in] p3DEntityParent   The Qt3D entity parent of the new item.
    */
    virtual void initInterpolationItem(const Eigen::MatrixX3f &matVertices,
                                       const Eigen::MatrixX3f &matNormals,
                                       const Eigen::MatrixX3i &matTriangles,
                                       Qt3DCore::QEntity* p3DEntityParent) override;

    //=========================================================================================================
    /**
    * This function gets called whenever this item receives new color values for each estimated source.
    *
    * @param[in] sourceColorSamples         The color values for the streamed data.
    */
    virtual void onNewRtSmoothedData(const MatrixX3f &matColorMatrix);

    QPointer<CpuInterpolationItem>          m_pInterpolationItem;                   /**< This item manages all 3d rendering and calculations. */

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CPUSENSORDATATREEITEM_H
