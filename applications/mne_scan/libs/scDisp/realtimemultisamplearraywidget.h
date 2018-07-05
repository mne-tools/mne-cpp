//=============================================================================================================
/**
* @file     realtimemultisamplearraywidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <lesch@mgh.harvard.edu>;
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

#include "scdisp_global.h"
#include "measurementwidget.h"


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
#include <QMap>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;
class QTableView;
class QToolBox;

namespace DISP3DLIB {
    class SensorDataTreeItem;
    class View3D;
    class Control3DWidget;
    class Data3DTreeModel;
}

namespace DISPLIB {
    class SelectionManagerWindow;
    class FilterView;
    class ChInfoModel;
}

namespace FIFFLIB {
    class FiffInfo;
}

namespace MNELIB {
    class MNEBem;
}

namespace SCMEASLIB{
    class NewRealTimeMultiSampleArray;
    class RealTimeSampleArrayChInfo;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{


//*************************************************************************************************************
//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================

class RealTimeMultiSampleArrayModel;
class RealTimeMultiSampleArrayDelegate;
class QuickControlWidget;


//=============================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArrayWidget
*
* @brief The RealTimeMultiSampleArrayWidget class provides a real-time curve display.
*/
class SCDISPSHARED_EXPORT RealTimeMultiSampleArrayWidget : public MeasurementWidget
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
    RealTimeMultiSampleArrayWidget(QSharedPointer<SCMEASLIB::NewRealTimeMultiSampleArray> pRTMSA_New, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

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
    virtual void update(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

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

    //=========================================================================================================
    /**
    * Shows the 3D control widget
    */
    void show3DControlWidget();

signals:
    //=========================================================================================================
    /**
    * fiffFileUpdated is emitted whenever the fiff info changed
    *
    * @param FiffInfo the current loaded fiffinfo
    */
    void fiffFileUpdated(const FIFFLIB::FiffInfo&);

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

private slots:
    //=========================================================================================================
    /**
    * Broadcast channel scaling
    *
    * @param [in] scaleMap QMap with scaling values which is to be broadcasted to the model.
    */
    void broadcastScaling(QMap<qint32, float> scaleMap);

    //=========================================================================================================
    /**
    * Sets new zoom factor
    *
    * @param [in] zoomFac  time window size;
    */
    void zoomChanged(double zoomFac);

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
    * Gets called when the views in the viewport of the table view change
    *
    * @param [in] value unused int.
    */
    void visibleRowsChanged(int value);

    //=========================================================================================================
    /**
    * Gets called when the bad channels are about to be marked as bad or good
    */
    void markChBad();

    //=========================================================================================================
    /**
    * Hides/shows all bad channels in the view
    */
    void hideBadChannels();

    //=========================================================================================================
    /**
    * Shows the filter widget
    */
    void showFilterWidget(bool state = true);

    //=========================================================================================================
    /**
    * Shows sensor selection widget
    */
    void showSensorSelectionWidget();

    //=========================================================================================================
    /**
    * Shows quick control widget
    */
    void showQuickControlWidget();

    //=========================================================================================================
    /**
    * Broadcast the background color changes made in the QuickControl widget
    *
    * @param [in] backgroundColor  The new background color.
    */
    void onTableViewBackgroundColorChanged(const QColor& backgroundColor);

    //=========================================================================================================
    /**
    * Call this slot whenever you want to make a screenshot current view.
    *
    * @param[in] imageType  The current iamge type: png, svg.
    */
    void onMakeScreenshot(const QString& imageType);

private:
    QSharedPointer<RealTimeMultiSampleArrayModel>           m_pRTMSAModel;                  /**< RTMSA data model */
    QSharedPointer<RealTimeMultiSampleArrayDelegate>        m_pRTMSADelegate;               /**< RTMSA data delegate */
    QSharedPointer<QuickControlWidget>                      m_pQuickControlWidget;          /**< quick control widget. */
    QSharedPointer<DISPLIB::ChInfoModel>                    m_pChInfoModel;                 /**< channel info model. */
    QSharedPointer<SCMEASLIB::NewRealTimeMultiSampleArray>  m_pRTMSA;                       /**< The real-time sample array measurement. */
    QSharedPointer<DISPLIB::SelectionManagerWindow>         m_pSelectionManagerWindow;      /**< SelectionManagerWindow. */
    QSharedPointer<DISPLIB::FilterView>                   m_pFilterWindow;                /**< Filter window. */

    bool                                        m_bInitialized;                 /**< Is Initialized */
    bool                                        m_bHideBadChannels;             /**< hide bad channels flag. */
    bool                                        m_bVisualize3DSensorData;       /**< Whether to visualize sensor data in 3D using Disp3D. */
    qint32                                      m_iMaxFilterTapSize;            /**< maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */
    float                                       m_fDefaultSectionSize;          /**< Default row height */
    float                                       m_fZoomFactor;                  /**< Zoom factor */
    float                                       m_fSamplingRate;                /**< Sampling rate */
    qint32                                      m_iT;                           /**< Display window size in seconds */

    QStringList                                 m_slAvailableModalities;        /**< List of available modalitites: EEG, MEG, etc. */
    QStringList                                 m_slSelectedChannels;           /**< the currently selected channels from the selection manager window. */
    QList<qint32>                               m_qListCurrentSelection;        /**< Current selection list -> hack around C++11 lambda  */
    QList<qint32>                               m_qListBadChannels;             /**< Current list of bad channels  */
    QList<SCMEASLIB::RealTimeSampleArrayChInfo> m_qListChInfo;                  /**< Channel info list. ToDo: check if this is obsolete later on -> ToDo use fiff Info instead*/
    QMap<qint32,float>                          m_qMapChScaling;                /**< Channel scaling values. */

    QSharedPointer<FIFFLIB::FiffInfo>           m_pFiffInfo;                    /**< FiffInfo, which is used insteadd of ListChInfo*/

    QTableView*                                 m_pTableView;                   /**< the QTableView being part of the model/view framework of Qt. */
    QSharedPointer<QToolBox>                    m_pToolBox;                     /**< The toolbox which holds the table view and real-time interpolation plot. */

    QSharedPointer<DISP3DLIB::View3D>           m_p3DView;                      /**< The Disp3D view. */
    QSharedPointer<DISP3DLIB::Control3DWidget>  m_pControl3DView;               /**< The Disp3D control. */
    QSharedPointer<DISP3DLIB::Data3DTreeModel>  m_pData3DModel;                 /**< The Disp3D model. */
    DISP3DLIB::SensorDataTreeItem*              m_pRtEEGSensorDataItem;         /**< The Disp3D real time item for EEG sensor data. */
    DISP3DLIB::SensorDataTreeItem*              m_pRtMEGSensorDataItem;         /**< The Disp3D real time item for MEG sensor data. */
    QSharedPointer<MNELIB::MNEBem>              m_pBemHead;                     /**< The Disp3D Bem head data. */
    QSharedPointer<MNELIB::MNEBem>              m_pBemSensor;                   /**< The Disp3D BEM sensor data. */

    QAction*                                    m_pActionSelectSensors;         /**< show roi select widget */
    QAction*                                    m_pActionHideBad;               /**< Hide bad channels. */
    QAction*                                    m_pActionQuickControl;          /**< Show quick control widget. */
    QAction*                                    m_pAction3DControl;             /**< Show 3D View control widget */
 };

} // NAMESPACE SCDISPLIB

#endif // REALTIMEMULTISAMPLEARRAYWIDGET_H
