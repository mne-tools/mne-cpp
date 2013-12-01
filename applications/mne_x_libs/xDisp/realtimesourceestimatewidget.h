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
* @brief    Declaration of the RealTimeSourceEstimateWidget Class.
*
*/

#ifndef REALTIMESOURCEESTIMATEWIDGET_H
#define REALTIMESOURCEESTIMATEWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"
#include "measurementwidget.h"

#include <disp3D/inverseview.h>
#include <mne/mne_forwardsolution.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSet>
#include <QList>
#include <QVector>
#include <QPainterPath>
#include <QMutex>
#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB
{
    class RealTimeSourceEstimate;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;
using namespace DISP3DLIB;
using namespace MNELIB;


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

class XDISPSHARED_EXPORT RealTimeSourceEstimateWidget : public MeasurementWidget
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
    RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> &pRTSE, QWidget* parent = 0);

//    RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> pRTMSE, QSharedPointer<QTime> pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeSourceEstimateWidget.
    */
    ~RealTimeSourceEstimateWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    * Inherited by IObserver.
    *
    * @param [in] pSubject pointer to Subject -> not used because its direct attached to the measurement.
    */
    virtual void update(Subject* pSubject);

    //=========================================================================================================
    /**
    * Initialise the RealTimeSourceEstimateWidget.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Initialise the OpenGL widget.
    *
    * @return true when successful
    */
    bool initOpenGLWidget();

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

private slots:

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

    InverseView* m_pView;
    QWidget* m_pWidgetView;

    bool m_bInitialized;

    QSharedPointer<RealTimeSourceEstimate> m_pRTMSE;    /**< The real-time source estimate measurement. */



    void actualize();                                   /**< Actualize member variables. */
//    Ui::RealTimeSourceEstimateClass   ui;               /**< The user interface of the RealTimeSampleArray widget. */
//    QSharedPointer<RealTimeSourceEstimate> m_pRTMSE;    /**< The real-time source estimate measurement. */

//    unsigned int                    m_uiNumChannels;

//    QPainterPath                    m_qPainterPath;                 /**< Holds the current painter path which is the real-time curve. */
//    QPainterPath                    m_qPainterPathTest;
//    QVector<QPainterPath>           m_qVecPainterPath;

//    double                          m_dMinValue_init;               /**< Holds the initial minimal value */
//    double                          m_dMaxValue_init;               /**< Holds the initial maximal value */
//    static QList<double>            s_listSamplingRates;            /**< Holds all real-time sample array sampling rates of the current display. */

};

} // NAMESPACE

#endif // REALTIMESOURCEESTIMATEWIDGET_H
