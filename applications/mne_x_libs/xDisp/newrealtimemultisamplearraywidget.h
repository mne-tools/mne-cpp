//=============================================================================================================
/**
* @file     newrealtimemultisamplearray_new_widget.h
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
* @brief    Declaration of the RealTimeMultiSampleArrayNewWidget Class.
*
*/

#ifndef NEWREALTIMEMULTISAMPLEARRAYNEWWIDGET_H
#define NEWREALTIMEMULTISAMPLEARRAYNEWWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"
#include "newmeasurementwidget.h"
#include "ui_newrealtimemultisamplearraywidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QSet>
#include <QList>
#include <QVector>
#include <QPainterPath>
#include <QMutex>
#include <QThread>
#include <QTableView>

//#define NEWTABLEVIEW 1



//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB
{
class NewRealTimeMultiSampleArray;
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

class XDISPSHARED_EXPORT NewRealTimeMultiSampleArrayWidget : public NewMeasurementWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RealTimeMultiSampleArrayWidget which is a child of parent.
    *
    * @param [in] pRTMSA_New    pointer to real-time multi sample array measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    NewRealTimeMultiSampleArrayWidget(QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA_New, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeMultiSampleArrayWidget.
    */
    ~NewRealTimeMultiSampleArrayWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Initialise the RealTimeMultiSampleArrayWidget.
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

#ifdef NEWTABLEVIEW

    QTableView *m_pTableView; /**< the QTableView being part of the model/view framework of Qt */

#endif


    void actualize();                                               /**< Actualize member variables. Like y position, scaling factor, middle value of the frame and the highest sampling rate to calculate the sample width.*/
    Ui::NewRealTimeMultiSampleArrayClass   ui;                      /**< The user interface of the RealTimeSampleArray widget. */
    QSharedPointer<NewRealTimeMultiSampleArray> m_pRTMSA_New;       /**< The real-time sample array measurement. */

    quint32                         m_uiMaxNumChannels;
    quint32                         m_uiNumChannels;
    quint32                          m_uiFirstChannel;

    QPainterPath                    m_qPainterPath;                 /**< The current painter path which is the real-time curve. */
    QPainterPath                    m_qPainterPathTest;
    QVector<QPainterPath>           m_qVecPainterPath;
    QVector<QPolygonF>              m_qVecPolygonF; //New

    QPainterPath                    m_qPainterPath_Freeze;          /**< The frozen painter path which is the frozen real-time curve. */
    QPainterPath                    m_qPainterPath_FreezeTest;
    QVector<QPainterPath>           m_qVecPainterPath_Freeze;
    QVector<QPolygonF>              m_qVecPolygonF_Freeze; //New

    QMutex                          m_qMutex;                       /**< A mutex to make the access to the painter path thread safe. */
    bool                            m_bMeasurement;                 /**< Current status whether curve measurement is active (left mouse). */
    bool                            m_bPosition;                    /**< Current status whether current coordinates should be shown. */
    bool                            m_bFrozen;                      /**< Current status whether curve is frozen. */
    bool                            m_bScaling;                     /**< Current status whether scaling of curve is active. */
    bool                            m_bToolInUse;                   /**< Current status whether tool (annotation/freezing) is active. */
    QPoint                          m_qPointMouseStartPosition;     /**< Mouse start position which is the position where mouse was first pressed. */
    QPoint                          m_qPointMouseEndPosition;       /**< Mouse end position which is current mouse position. */
    float                           m_fScaleFactor;                 /**< Current scaling factor -> renewed over actualize. */
    double                          m_dMinValue_init;               /**< The initial minimal value */
    double                          m_dMaxValue_init;               /**< The initial maximal value */
    qint32                          m_iSamples;                     /**< The current number of samples. */
    qint32                          m_iSampleCount;                 /**< The current sample count. */
    double                          m_dMiddle;                      /**< The current middle value depending on the current scaling factor -> renewed over actualize. */
    double                          m_dPosition;                    /**< The start position which is the x position of the frame. */
    double                          m_dSampleWidth;                 /**< Sample distance to synchronize all real-time sample array widgets independent from their sampling rate. */
    double                          m_dPosX;                        /**< The x position of the frame. */
    double                          m_dPosY;                        /**< The middle y position of the frame. */
    bool                            m_bStartFlag;                   /**< Status whether the real-time curve should be restarted. */
    std::vector<QString>            m_vecTool;                      /**< The available tools. */
    unsigned char                   m_ucToolIndex;                  /**< The selected tool index. */
    QSharedPointer<QTimer>          m_pTimerToolDisplay;            /**< Timer for blending the tool label. */
    QSharedPointer<QTimer>          m_pTimerUpdate;                 /**< Timer which is caring about a continuous paint update of the widget. */
    QSharedPointer<QTime>           m_pTime;                        /**< The application time. */
    QSharedPointer<QTime>           m_pTimeCurrentDisplay;          /**< Time which corresponds to the x starting position of each segment. */
    static QList<double>            s_listSamplingRates;            /**< All real-time sample array sampling rates of the current display. */

};

} // NAMESPACE

#endif // REALTIMEMULTISAMPLEARRAYNEWWIDGET_H
