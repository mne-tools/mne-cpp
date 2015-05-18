//=============================================================================================================
/**
* @file     realtimemultisamplearraywidget.h
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
* @brief    Declaration of the RealTimeMultiSampleArrayWidget Class.
*
*/

#ifndef REALTIMEMULTISAMPLEARRAYWIDGET_H
#define REALTIMEMULTISAMPLEARRAYWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"

#include "newmeasurementwidget.h"

#include <xMeas/newrealtimemultisamplearray.h>

//#include "annotationwindow.h"

#include "helpers/realtimemultisamplearraymodel.h"
#include "helpers/realtimemultisamplearraydelegate.h"
#include "helpers/realtimemultisamplearrayscalingwidget.h"
#include "helpers/projectorwidget.h"
#include "helpers/selectionmanagerwindow.h"
#include "helpers/chinfomodel.h"

#include "disp/filterwindow.h"

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QMap>
#include <QTableView>
#include <QAction>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QTime>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QScroller>
#include <QScrollBar>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB{class NewRealTimeMultiSampleArray;}


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
using namespace DISPLIB;


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
* DECLARE CLASS RealTimeMultiSampleArrayWidget
*
* @brief The RealTimeMultiSampleArrayWidget class provides a real-time curve display.
*/
class XDISPSHARED_EXPORT RealTimeMultiSampleArrayWidget : public NewMeasurementWidget
{
    Q_OBJECT

    friend class RealTimeMultiSampleArrayScalingWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeMultiSampleArrayWidget which is a child of parent.
    *
    * @param [in] pRTMSA_New    pointer to real-time multi sample array measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeMultiSampleArrayWidget(QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA_New, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeMultiSampleArrayWidget.
    */
    ~RealTimeMultiSampleArrayWidget();

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

    //=========================================================================================================
    /**
    * Is called when mouse wheel is used.
    * Function is selecting the tool (freezing/annotation);
    *
    * @param object
    * @param event o
    *
    * @return
    */
    bool eventFilter(QObject *object, QEvent *event);

signals:
    //=========================================================================================================
    /**
    * fiffFileUpdated is emitted whenever the fiff info changed
    *
    * @param FiffInfo the current loaded fiffinfo
    */
    void fiffFileUpdated(const FiffInfo&);

    //=========================================================================================================
    /**
    * samplingRateChanged is emitted whenever the sampling rate is changed
    *
    * @param samplingRate the current (downsampled) sampling rate
    */
    void samplingRateChanged(double samplingRate);

    //=========================================================================================================
    /**
    * position is emitted whenever user moves the mouse inside of the table view viewport
    *
    * @param position   the current mouse position
    * @param activeRow  the current row which the mouse is moved over
    */
    void markerMoved(QPoint position, int activeRow);

private:
    //=========================================================================================================
    /**
    * Broadcast channel scaling
    */
    void broadcastScaling();

    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] zoomFac  time window size;
    */
    void zoomChanged(double zoomFac);

    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] dsFactor downsampling factor
    */
    void dsFactorChanged(int dsFactor);

    //=========================================================================================================
    /**
    * Sets new time window size
    *
    * @param [in] T  time window size;
    */
    void timeWindowChanged(int T);

    //=========================================================================================================
    /**
    * apply the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void applySelection();

    //=========================================================================================================
    /**
    * hides the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void hideSelection();

    //=========================================================================================================
    /**
    * reset the in m_qListCurrentSelection stored selection -> hack around C++11 lambda
    */
    void resetSelection();

    //=========================================================================================================
    /**
    * Only shows the channels defined in the QStringList selectedChannels
    *
    * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
    */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
    * hides/show all bad channels in the view
    */
    void hideBadChannels();

    //=========================================================================================================
    /**
    * Show channel scaling widget
    */
    void showChScalingWidget();

    //=========================================================================================================
    /**
    * Shows the projection widget
    */
    void showProjectionWidget();

    //=========================================================================================================
    /**
    * Shows the filter widget
    */
    void showFilterWidget();

    //=========================================================================================================
    /**
    * Shows sensor selection widget
    */
    void showSensorSelectionWidget();

    //=========================================================================================================
    /**
    * Gets called when the views in the viewport of the table view change
    */
    void viewableRowsChanged(int value);

    RealTimeMultiSampleArrayModel*      m_pRTMSAModel;                  /**< RTMSA data model */
    RealTimeMultiSampleArrayDelegate*   m_pRTMSADelegate;               /**< RTMSA data delegate */

    bool            m_bInitialized;                                     /**< Is Initialized */
    bool            m_bHideBadChannels;                                 /**< hide bad channels flag. */
    float           m_fDefaultSectionSize;                              /**< Default row height */
    float           m_fZoomFactor;                                      /**< Zoom factor */
    float           m_fSamplingRate;                                    /**< Sampling rate */
    float           m_fDesiredSamplingRate;                             /**< Desired display sampling rate */
    qint32          m_iDSFactor;                                        /**< Downsampling factor */
    qint32          m_iT;                                               /**< Display window size in seconds */

    QStringList     m_slSelectedChannels;                               /**< the currently selected channels from the selection manager window. */
    QList<qint32>   m_qListCurrentSelection;                            /**< Current selection list -> hack around C++11 lambda  */
    QList<qint32>   m_qListBadChannels;                                 /**< Current list of bad channels  */
    QList<RealTimeSampleArrayChInfo> m_qListChInfo;                     /**< Channel info list. ToDo: check if this is obsolete later on -> ToDo use fiff Info instead*/
    QMap< qint32,float > m_qMapChScaling;                               /**< Sensor selection widget. */

    FiffInfo::SPtr  m_pFiffInfo;                                        /**< FiffInfo, which is used insteadd of ListChInfo*/

    QDoubleSpinBox* m_pDoubleSpinBoxZoom;                               /**< Adjust Zoom Factor */
    QSpinBox*       m_pSpinBoxTimeScale;                                /**< Time scale spin box */
    QSpinBox*       m_pSpinBoxDSFactor;                                 /**< downsampling factor */
    QTableView*     m_pTableView;                                       /**< the QTableView being part of the model/view framework of Qt */

    QSharedPointer<ChInfoModel>                     m_pChInfoModel;                 /**< channel info model. */
    QSharedPointer<NewRealTimeMultiSampleArray>     m_pRTMSA;                       /**< The real-time sample array measurement. */
    QSharedPointer<SelectionManagerWindow>          m_pSelectionManagerWindow;      /**< SelectionManagerWindow. */
    QSharedPointer<FilterWindow>                    m_pFilterWindow;                /**< SelectionManagerWindow. */
    QSharedPointer<ProjectorWidget>                 m_pProjectorSelectionWidget;    /**< Projector selection widget. */
    QSharedPointer<RealTimeMultiSampleArrayScalingWidget> m_pRTMSAScalingWidget;    /**< Channel scaling widget. */

    QAction*        m_pActionSelectSensors;                             /**< show roi select widget */
    QAction*        m_pActionFiltering;                                 /**< show filter window */
    QAction*        m_pActionChScaling;                                 /**< Show channel scaling Action. */
    QAction*        m_pActionProjection;                                /**< Show projections Action. */
    QAction*        m_pActionHideBad;                                   /**< Hide bad channels. */

 };

} // NAMESPACE XDISPLIB

#endif // REALTIMEMULTISAMPLEARRAYWIDGET_H
