//=============================================================================================================
/**
* @file     realtimeevokedwidget.h
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
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Declaration of the RealTimeEvokedWidget Class.
*
*/

#ifndef REALTIMEEVOKEDWIDGET_H
#define REALTIMEEVOKEDWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"
#include "newmeasurementwidget.h"
#include "helpers/realtimeevokedmodel.h"
#include "helpers/realtimebutterflyplot.h"

#include "helpers/sensorwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QAction>
#include <QSpinBox>
#include <QDoubleSpinBox>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB
{
class RealTimeEvoked;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;


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
class XDISPSHARED_EXPORT RealTimeEvokedWidget : public NewMeasurementWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeEvokedWidget which is a child of parent.
    *
    * @param [in] pRTE          pointer to real-time evoked measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeEvokedWidget(QSharedPointer<RealTimeEvoked> pRTE, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeEvokedWidget.
    */
    ~RealTimeEvokedWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Is called when new data are available.
    */
    virtual void getData();

    //=========================================================================================================
    /**
    * Initialise the RealTimeEvokedWidget.
    */
    virtual void init();

public slots:
    //=========================================================================================================
    /**
    * Show channel context menu
    *
    * @param [in] pos   Position to popup the conext menu.
    */
    void channelContextMenu(QPoint pos);

protected:
    //=========================================================================================================
    /**
    * Is called when RealTimeSampleArrayWidget is resized.
    *
    * @param [in] event pointer to ResizeEvent -> not used.
    */
    virtual void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
    * Is called when key is pressed.
    * Function is getting the current key event.
    *
    * @param [in] keyEvent pointer to KeyEvent.
    */
    virtual void keyPressEvent(QKeyEvent * keyEvent);

    //=========================================================================================================
    /**
    * Is called when mouse button is pressed.
    * Function is getting the current mouse position and to differ between left(measure curve) and right(zoom) mouse button.
    *
    * @param [in] mouseEvent pointer to MouseEvent.
    */
    virtual void mousePressEvent(QMouseEvent* mouseEvent);

    //=========================================================================================================
    /**
    * Is called when mouse is moved.
    * Function is getting the current mouse position for measurement of the real-time curve and to zoom in or out.
    *
    * @param [in] mouseEvent pointer to MouseEvent.
    */
    virtual void mouseMoveEvent(QMouseEvent* mouseEvent);

    //=========================================================================================================
    /**
    * Is called when mouse button is released.
    * Function is stopping measurement of the real-time curve or the zooming.
    *
    * @param [in] event pointer to MouseEvent -> not used.
    */
    virtual void mouseReleaseEvent(QMouseEvent* event);

    //=========================================================================================================
    /**
    * Is called when mouse button is double clicked.
    * Depending on the current selected tool: Function is (un-)freezing the real-time curve or an annotation point is set.
    *
    * @param [in] event pointer to MouseEvent -> not used.
    */
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    //=========================================================================================================
    /**
    * Is called when mouse wheel is used.
    * Function is selecting the tool (freezing/annotation);
    *
    * @param [in] wheelEvent pointer to WheelEvent. Depending on the delta movement a tool is selected.
    */
    virtual void wheelEvent(QWheelEvent* wheelEvent);

private:
    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] zoomFac  time window size;
    */
    void zoomChanged(double zoomFac);

    //=========================================================================================================
    /**
    * Shows sensor selection widget
    */
    void showSensorSelectionWidget();

    RealTimeEvokedModel*        m_pRTEModel;            /**< RTE data model */
    RealTimeButterflyPlot*      m_pButterflyPlot;       /**< Butterfly plot */

    float m_fZoomFactor;                                    /**< Zoom factor */
    QDoubleSpinBox* m_pDoubleSpinBoxZoom;                   /**< Adjust Zoom Factor */

    QSharedPointer<RealTimeEvoked> m_pRTE;                  /**< The real-time evoked measurement. */

    bool m_bInitialized;                                    /**< Is Initialized */

    QList<RealTimeSampleArrayChInfo>    m_qListChInfo;      /**< Channel info list. ToDo: check if this is obsolete later on*/

    QAction*    m_pActionSelectSensors;                     /**< show roi select widget */

    SensorModel* m_pSensorModel;                            /**< Sensor model for channel selection */
    QSharedPointer<SensorWidget> m_pSensorSelectionWidget;  /**< Sensor selection widget. */

    QList<qint32> m_qListCurrentSelection;  /**< Current selection list -> hack around C++11 lambda  */
    void applySelection();                  /**< apply the in m_qListCurrentSelection stored selection -> hack around C++11 lambda */
    void resetSelection();                  /**< reset the in m_qListCurrentSelection stored selection -> hack around C++11 lambda */
};

} // NAMESPACE

#endif // REALTIMEEVOKEDWIDGET_H
