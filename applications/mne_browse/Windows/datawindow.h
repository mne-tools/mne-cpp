//=============================================================================================================
/**
 * @file     datawindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the DataWindow class.
 *
 */

#ifndef DATAWINDOW_H
#define DATAWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include "mainwindow.h"
#include "../Utils/datamarker.h"
#include "ui_datawindowdock.h"
#include "../Delegates/rawdelegate.h"
#include "../Models/rawmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QResizeEvent>
#include <QPainter>
#include <QColor>
#include <QGesture>
#include <QScroller>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{

//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

class MainWindow;
class RawDelegate;

/**
 * DECLARE CLASS DataWindow
 *
 * @brief The DataWindow class provides the data dock window.
 */
class DataWindow : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a DataWindow dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new DataWindow becomes a window. If parent is another widget, DataWindow becomes a child window inside parent. DataWindow is deleted when its parent is deleted.
    */
    DataWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the DataWindow.
    * All DataWindow's children are deleted first. The application exits if DataWindow is the main widget.
    */
    ~DataWindow();

    //=========================================================================================================
    /**
    * Initialises this window.
    */
    void init();

    //=========================================================================================================
    /**
    * Setup the model view controller of the data window
    */
    void initMVCSettings();

    //=========================================================================================================
    /**
    * Returns the data QTableView of this window
    */
    QTableView* getDataTableView();

    //=========================================================================================================
    /**
    * Returns the undocked data QTableView of this window
    */
    QTableView* getUndockedDataTableView();

    //=========================================================================================================
    /**
    * Returns the RawModel of this window
    */
    RawModel* getDataModel();

    //=========================================================================================================
    /**
    * Returns the RawModel of this window
    */
    RawDelegate* getDataDelegate();

    //=========================================================================================================
    /**
    * Scales the data according to scaleMap
    *
    * @param [in] scaleMap map with all channel types and their current scaling value
    */
    void scaleData(const QMap<QString,double> &scaleMap);

    //=========================================================================================================
    /**
    * Updates the data table views
    */
    void updateDataTableViews();

    //=========================================================================================================
    /**
    * Only shows the channels defined in the QStringList selectedChannels
    *
    * @param [in] selectedChannels list of all channel names which are currently selected in the selection manager.
    */
    void showSelectedChannelsOnly(QStringList selectedChannels);

    //=========================================================================================================
    /**
    * Change the channel plot height in the data views to the double value heigt
    */
    void changeRowHeight(int height);

    //=========================================================================================================
    /**
    * hide all bad channels
    */
    void hideBadChannels(bool hideChannels);

private:
    //=========================================================================================================
    /**
    * Setup the tool bar of the data window.
    */
    void initToolBar();

    //=========================================================================================================
    /**
    * Setup the sample labels of the data window
    */
    void initLabels();

    //=========================================================================================================
    /**
    * Setup the marker of the data window
    */
    void initMarker();

    //=========================================================================================================
    /**
    * resizeEvent reimplemented virtual function to handle resize events of the data dock window
    */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
    * keyPressEvent reimplemented virtual function to handle key press events of the data dock window
    */
    void keyPressEvent(QKeyEvent* event);

    //=========================================================================================================
    /**
    * Installed event filter.
    */
    bool eventFilter(QObject *object, QEvent *event);

    //=========================================================================================================
    /**
    * gestureEvent processes gesture events
    */
    bool gestureEvent(QGestureEvent *event);

    //=========================================================================================================
    /**
    * pinchTriggered processes pinch gesture events
    */
    bool pinchTriggered(QPinchGesture *gesture);

    Ui::DataWindowDockWidget *ui;                   /**< Pointer to the qt designer generated ui class.*/

    MainWindow*     m_pMainWindow;                  /**< pointer to the main window (parent). */

    QSettings       m_qSettings;                    /**< QSettings variable used to write or read from independent application sessions. */

    DataMarker*     m_pDataMarker;                  /**< pointer to the data marker. */
    QLabel*         m_pCurrentDataMarkerLabel;      /**< the current data marker label to display the marker's position. */
    int             m_iCurrentMarkerSample;         /**< the current data marker sample value to display the marker's position. */

    RawDelegate*    m_pRawDelegate;                 /**< the QAbstractDelegate being part of the raw model/view framework of Qt. */
    RawModel*       m_pRawModel;                    /**< the QAbstractTable model being part of the model/view framework of Qt. */

    QScroller*      m_pKineticScroller;             /**< the kinetic scroller of the QTableView. */

    QStringList     m_slSelectedChannels;           /**< the currently selected channels from the selection manager window. */

    bool            m_bHideBadChannels;             /**< hide bad channels flag. */

signals:
    //=========================================================================================================
    /**
    * scaleChannels gets called whenever the user performed a scaling gesture (pinch)
    */
    void scaleChannels(double);

protected slots:
    //=========================================================================================================
    /**
    * @brief customContextMenuRequested
    * @param pos is the position, where the right-click occurred
    */
    void customContextMenuRequested(QPoint pos);

    //=========================================================================================================
    /**
    * Set the range sample labels of the data window
    */
    void setRangeSampleLabels();

    //=========================================================================================================
    /**
    * Set the sample labels of the data window
    */
    void setMarkerSampleLabel();

    //=========================================================================================================
    /**
    * Updates the marker position
    */
    void updateMarkerPosition();

    //=========================================================================================================
    /**
    * Highlights the current selected channels in the 2D plot of selection manager
    */
    void highlightChannelsInSelectionManager();
};

} // NAMESPACE MNEBROWSE

#endif // DATAWINDOW_H
