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

namespace FIFFLIB{
    class FiffEvoked;
}

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

class RtSensorDataController;


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
    explicit CpuSensorDataTreeItem(int iType = Data3DTreeModelItemTypes::SensorDataItem, const QString& text = "Sensor Data");

    //=========================================================================================================
    /**
    * Destructor, stops and deletes rtsensordata worker.
    */
    virtual ~CpuSensorDataTreeItem();

    //=========================================================================================================
    /**
     * Initializes the sensor data item with neccessary information for visualization computations.
     * Constructs and initalizes the worker for this item.
     *
     * @param[in] matSurfaceVertColor       The base color for the vertices which the streamed data is later plotted on.
     * @param[in] bemSurface                MNEBemSurface that holds the mesh that should be visualized.
     * @param[in] fiffInfo                  FiffInfo that holds the sensors information.
     * @param[in] sSensorType               The sensor type that is later used for live interpolation.
     * @param[in] dCancelDist               Distances higher than this are ignored for the interpolation.
     * @param[in] sInterpolationFunction    Function that computes interpolation coefficients using the distance values.
     */
    void init(const MatrixX3f& matSurfaceVertColor,
              const MNELIB::MNEBemSurface& bemSurface,
              const FIFFLIB::FiffInfo &fiffInfo,
              const QString& sSensorType,
              const double dCancelDist,
              const QString &sInterpolationFunction);

    //=========================================================================================================
    /**
    * Adds a block actual rt data which is streamed by this item's worker thread item.
    * In order for this function to worker, you must call init(...) beforehand.
    *
    * @param[in] tSensorData                The matrix that holds rt measurement data.
    */
    virtual void addData(const MatrixXd& tSensorData) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the origin of the surface vertex color changed.
    *
    * @param[in] matVertColor               The matrix that holds the origin colors for all vertices of the surface
    */
    void setColorOrigin(const MatrixX3f& matVertColor);

    //=========================================================================================================
    /**
    * Set the sampling frequency.
    *
    * @param[in] dSFreq                 The new sampling frequency.
    */
    virtual void setSFreq(const double dSFreq) override;

    //=========================================================================================================
    /**
    * Update bad channels and recalculate interpolation matrix.
    *
    * @param[in] info                 The fiff info including the new bad channels.
    */
    virtual void setBadChannels(const FIFFLIB::FiffInfo& info) override;

protected:
    //=========================================================================================================
    /**
    * This function gets called whenever the check/activation state of the rt data worker changed.
    *
    * @param[in] checkState                 The check state of the worker.
    */
    virtual void onStreamingStateChanged(const Qt::CheckState& checkState) override;

    //=========================================================================================================
    /**
    * This function gets called whenever this item receives new color values for each estimated source.
    *
    * @param[in] sourceColorSamples         The color values for the streamed data.
    */
    virtual void onNewRtSmoothedData(const MatrixX3f &matColorMatrix);

    //=========================================================================================================
    /**
    * This function gets called whenever the used colormap type changed.
    *
    * @param[in] sColormapType              The name of the new colormap type.
    */
    virtual void onColormapTypeChanged(const QVariant& sColormapType) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the time interval in between the streamed samples changed.
    *
    * @param[in] iMSec                      The new time in milliseconds waited in between each streamed sample.
    */
    virtual void onTimeIntervalChanged(const QVariant &iMSec) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the normaization value changed.
    * The normalization value is used to normalize the estimated source activation.
    *
    * @param[in] vecThresholds              The new threshold values used for normalizing the data.
    */
    virtual void onDataNormalizationValueChanged(const QVariant &vecThresholds) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the check/activation state of the looped streaming state changed.
    *
    * @param[in] checkState                 The check state of the looped streaming state.
    */
    virtual void onCheckStateLoopedStateChanged(const Qt::CheckState& checkState) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the number of averages of the streamed samples changed.
    *
    * @param[in] iNumAvr                    The new number of averages.
    */
    virtual  void onNumberAveragesChanged(const QVariant& iNumAvr) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the cancel distance of the interpolation changed.
    *
    * @param[in] dCancelDist     The new cancel distance.
    */
    virtual void onCancelDistanceChanged(const QVariant& dCancelDist) override;

    //=========================================================================================================
    /**
    * This function gets called whenever the function of the interpolation changed.
    *
    * @param[in] sInterpolationFunction     The new function name.
    */
    virtual void onInterpolationFunctionChanged(const QVariant& sInterpolationFunction) override;

    QPointer<RtSensorDataController>     m_pSensorRtDataWorkController;             /**< The source data worker. This worker streams the rt data to this item.*/

signals:
    //=========================================================================================================
    /**
    * Emit this signal whenever you want to provide newly generated colors from the stream rt data.
    *
    * @param[in] vertColors                 The colors for the underlying mesh surface
    */
    void rtVertColorChanged(const QVariant &vertColors);

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace DISP3DLIB

#endif // DISP3DLIB_CPUSENSORDATATREEITEM_H
