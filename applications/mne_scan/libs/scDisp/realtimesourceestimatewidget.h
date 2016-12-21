//=============================================================================================================
/**
* @file     realtimemultisamplearray_new_widget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeSourceEstimateWidget Class.
*
*/

#ifndef REALTIMESOURCEESTIMATEWIDGET_H
#define REALTIMESOURCEESTIMATEWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include "newmeasurementwidget.h"

#include "fs/annotationset.h"
#include "fs/surfaceset.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
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

class QTime;

namespace DISP3DLIB {
    class BrainRTSourceLocDataTreeItem;
    class View3D;
    class Control3DWidget;
    class Data3DTreeModel;
}

namespace SCMEASLIB {
    class RealTimeSourceEstimate;
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


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

////=============================================================================================================
///**
//* Tool enumeration.
//*/
//enum Tool
//{
//    Freeze     = 0,       /**< Freezing tool. */
//    Annotation = 1        /**< Annotation tool. */
//};


//=============================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArrayNewWidget
*
* @brief The RealTimeMultiSampleArrayNewWidget class provides a real-time curve display.
*/

class SCDISPSHARED_EXPORT RealTimeSourceEstimateWidget : public NewMeasurementWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeSourceEstimateWidget which is a child of parent.
    *
    * @param [in] pRTMSE        pointer to real-time multi sample array measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeSourceEstimateWidget(QSharedPointer<SCMEASLIB::RealTimeSourceEstimate> &pRTSE, QWidget* parent = 0);

//    RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> pRTMSE, QSharedPointer<QTime> pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeSourceEstimateWidget.
    */
    ~RealTimeSourceEstimateWidget();

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
    virtual void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Initialise the RealTimeSourceEstimateWidget.
    */
    virtual void init();

protected slots:
    //=========================================================================================================
    /**
    * Shows the 3D control widget
    */
    void show3DControlWidget();

private:
    QSharedPointer<SCMEASLIB::RealTimeSourceEstimate>   m_pRTSE;            /**< The real-time source estimate measurement. */

    bool                                                m_bInitialized;     /**< Whether init was processed successfully. */

    FSLIB::AnnotationSet                                m_annotationSet;    /**< The current annotation set. */
    FSLIB::SurfaceSet                                   m_surfSet;          /**< The current surface set. */

    QSharedPointer<DISP3DLIB::View3D>                   m_p3DView;          /**< The Disp3D view. */
    QSharedPointer<DISP3DLIB::Control3DWidget>          m_pControl3DView;   /**< The Disp3D control. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>          m_pData3DModel;     /**< The Disp3D model. */

    DISP3DLIB::BrainRTSourceLocDataTreeItem*            m_pRtItem;          /**< The Disp3D real time items. */

    QAction*                                            m_pAction3DControl; /**< Show 3D View control widget */

signals:
    void startInit();
};

} // NAMESPACE

#endif // REALTIMESOURCEESTIMATEWIDGET_H
