//=============================================================================================================
/**
* @file     realtimesamplearraywidget.h
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
* @brief    Declaration of the RealTimeSampleArrayWidget Class.
*
*/

#ifndef REALTIMESAMPLEARRAYWIDGET_H
#define REALTIMESAMPLEARRAYWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"
#include "ui_realtimesamplearraywidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSet>
#include <QList>
#include <QPainterPath>
#include <QMutex>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace SCMEASLIB
{
class RealTimeSampleArray;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;


//=============================================================================================================
/**
* DECLARE CLASS RealTimeSampleArrayWidget
*
* @brief The RealTimeSampleArrayWidget class provides a real-time curve display.
*/
class SCDISPSHARED_EXPORT RealTimeSampleArrayWidget : public MeasurementWidget
{
    Q_OBJECT
public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeSampleArrayWidget which is a child of parent.
    *
    * @param [in] pRTSA pointer to real-time sample array measurement.
    * @param [in] pTime pointer to application time.
    * @param [in] parent pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeSampleArrayWidget(QSharedPointer<RealTimeSampleArray> &pRTSA, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeSampleArrayWidget.
    */
    ~RealTimeSampleArrayWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Initialise the RealTimeSampleArrayWidget.
    */
    virtual void init();

protected:

    //=========================================================================================================
    /**
    * Is called to paint the incoming real-time data.
    * Function is painting the real-time curve, the grid, the measurement curve (when left button is pressed) and is calculating the zoom (when right button is pressed -> ToDo it's maybe better done in press event directly).
    *
    * @param [in] event pointer to PaintEvent -> not used.
    */
    virtual void paintEvent( QPaintEvent* event );

    //=========================================================================================================
    /**
    * Is called when RealTimeSampleArrayWidget is resized.
    *
    * @param [in] event pointer to ResizeEvent -> not used.
    */
    virtual void resizeEvent(QResizeEvent* event);

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

private slots:
    //=========================================================================================================
    /**
    * Stops the Annotation
    */
    void stopAnnotation();

    //=========================================================================================================
    /**
    * Sets a new maximal value to the real time sample array.
    * This is used for zooming functionality.
    */
    void maxValueChanged(double maxValue);

    //=========================================================================================================
    /**
    * Sets a new minimal value to the real time sample array.
    * This is used for zooming functionality.
    */
    void minValueChanged(double);

private:
    void actualize();                                               /**< Actualize member variables. Like y position, scaling factor, middle value of the frame and the highest sampling rate to calculate the sample width.*/
    Ui::RealTimeSampleArrayClass    ui;                             /**< the user interface of the RealTimeSampleArray widget. */
    QSharedPointer<RealTimeSampleArray> m_pRTSA;                 /**< the real-time sample array measurement. */
    QPainterPath                    m_qPainterPath;                 /**< the current painter path which is the real-time curve. */
    QPainterPath                    m_qPainterPath_Freeze;          /**< the frozen painter path which is the frozen real-time curve. */
    QMutex                          m_qMutex;                       /**< a mutex to make the access to the painter path thread safe. */
    bool                            m_bMeasurement;                 /**< current status whether curve measurement is active (left mouse). */
    bool                            m_bPosition;                    /**< current status whether current coordinates should be shown. */
    bool                            m_bFrozen;                      /**< current status whether curve is frozen. */
    bool                            m_bScaling;                     /**< current status whether scaling of curve is active. */
    bool                            m_bToolInUse;                   /**< current status whether tool (annotation/freezing) is active. */
    QPoint                          m_qPointMouseStartPosition;     /**< mouse start position which is the position where mouse was first pressed. */
    QPoint                          m_qPointMouseEndPosition;       /**< mouse end position which is current mouse position. */
    float                           m_fScaleFactor;                 /**< current scaling factor -> renewed over actualize. */
    double                          m_dMinValue_init;               /**< the initial minimal value */
    double                          m_dMaxValue_init;               /**< the initial maximal value */
    double                          m_dMiddle;                      /**< the current middle value depending on the current scaling factor -> renewed over actualize. */
    double                          m_dPosition;                    /**< the start position which is the x position of the frame. */
    double                          m_dSampleWidth;                 /**< Sample distance to synchronize all real-time sample array widgets independent from their sampling rate. */
    double                          m_dPosX;                        /**< the x position of the frame. */
    double                          m_dPosY;                        /**< the middle y position of the frame. */
    bool                            m_bStartFlag;                   /**< status whether the real-time curve should be restarted. */
    std::vector<QString>            m_vecTool;                      /**< the available tools. */
    unsigned char                   m_ucToolIndex;                  /**< the selected tool index. */
    QSharedPointer<QTimer>          m_pTimerToolDisplay;            /**< Timer for blending the tool label. */
    QSharedPointer<QTimer>          m_pTimerUpdate;                 /**< Timer which is caring about a continuous paint update of the widget. */
    QSharedPointer<QTime>           m_pTime;                        /**< the application time. */
    QSharedPointer<QTime>           m_pTimeCurrentDisplay;          /**< Time which corresponds to the x starting position of each segment. */
    static QList<double>            s_listSamplingRates;            /**< all real-time sample array sampling rates of the current display. */

};

} // NAMESPACE

#endif // REALTIMESAMPLEARRAYWIDGET_H
