//=============================================================================================================
/**
* @file     realtimeconnectivityestimatewidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeConnectivityEstimateWidget Class.
*
*/

#ifndef REALTIMECONNECTIVITYESTIMATEWIDGET_H
#define REALTIMECONNECTIVITYESTIMATEWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include "measurementwidget.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB {
    class NetworkTreeItem;
    class AbstractView;
}

namespace SCMEASLIB {
    class RealTimeConnectivityEstimate;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS RealTimeConnectivityEstimateWidget
*
* @brief The RealTimeConnectivityEstimateWidget class provides a real-time network display.
*/

class SCDISPSHARED_EXPORT RealTimeConnectivityEstimateWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeConnectivityEstimateWidget> SPtr;             /**< Shared pointer type for RealTimeConnectivityEstimateWidget class. */
    typedef QSharedPointer<const RealTimeConnectivityEstimateWidget> ConstSPtr;  /**< Const shared pointer type for RealTimeConnectivityEstimateWidget class. */

    //=========================================================================================================
    /**
    * Constructs a RealTimeConnectivityEstimateWidget which is a child of parent.
    *
    * @param [in] pRTCE     pointer to real-time connectivity estimate.
    * @param [in] parent    pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeConnectivityEstimateWidget(QSharedPointer<SCMEASLIB::RealTimeConnectivityEstimate> &pRTCE,
                                       QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeConnectivityEstimateWidget.
    */
    ~RealTimeConnectivityEstimateWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    */
    virtual void getData();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Initialise the RealTimeConnectivityEstimateWidget.
    */
    virtual void init();

protected:
    //=========================================================================================================
    /**
    * Shows quick control view
    */
    void showQuickControlView();

    QSharedPointer<SCMEASLIB::RealTimeConnectivityEstimate>     m_pRTCE;                /**< The real-time source estimate measurement. */
    QPointer<DISPLIB::QuickControlView>                         m_pQuickControlView;    /**< Quick control widget. */

    bool                                                        m_bInitialized;         /**< Whether init was processed successfully. */

    FSLIB::AnnotationSet                                        m_annotationSet;        /**< The current annotation set. */
    FSLIB::SurfaceSet                                           m_surfSet;              /**< The current surface set. */

    QPointer<DISP3DLIB::AbstractView>                           m_pAbstractView;         /**< The 3D view to visualize the network data. */

    DISP3DLIB::NetworkTreeItem*                                 m_pRtItem;              /**< The Disp3D real time item. */

    QPointer<QAction>                                           m_pActionQuickControl;  /**< Show quick control widget. */
};

} // NAMESPACE

#endif // REALTIMECONNECTIVITYESTIMATEWIDGET_H
