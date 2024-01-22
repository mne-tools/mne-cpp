//=============================================================================================================
/**
 * @file     fiffrawview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC   <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke, Gabriel Motta, Juan GPC. All rights reserved.
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
 * @brief    Declaration of the FiffRawView Class.
 *
 */

#ifndef FIFFRAWVIEW_H
#define FIFFRAWVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawdataviewer_global.h"
#include <disp/viewers/abstractview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QWidget>
#include <QPointer>
#include <QMap>
#include <QMenu>
#include <QScroller>

//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class FiffRawViewModel;
}

namespace RTPROCESSINGLIB {
    class FilterKernel;
}

class QTableView;
class QLabel;

//=============================================================================================================
// DEFINE NAMESPACE RAWDATAVIEWERPLUGIN
//=============================================================================================================

namespace RAWDATAVIEWERPLUGIN {

//=============================================================================================================
// RAWDATAVIEWERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class FiffRawViewDelegate;

//=============================================================================================================
/**
 * TableView for Fiff data.
 */
class RAWDATAVIEWERSHARED_EXPORT FiffRawView : public DISPLIB::AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawView> SPtr;            /**< Shared pointer type for FiffRawView. */
    typedef QSharedPointer<const FiffRawView> ConstSPtr; /**< Const shared pointer type for FiffRawView. */

    //=========================================================================================================
    /**
     * Constructs a FiffRawView which is a child of parent.
     *
     * @param[in] parent    The parent of widget.
     */
    FiffRawView(QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Resets the view to its default settings.
     */
    void reset();

    //=========================================================================================================
    /**
     * Returns the currently set delegate
     */
    QSharedPointer<FiffRawViewDelegate> getDelegate();

    //=========================================================================================================
    /**
     * Setups the delegate of this view.
     *
     * @param[in] pDelegate    The new delegate.
     */
    void setDelegate(const QSharedPointer<FiffRawViewDelegate>& pDelegate);

    //=========================================================================================================
    /**
     * Returns the currently set model
     */
    QSharedPointer<ANSHAREDLIB::FiffRawViewModel> getModel();

    //=========================================================================================================
    /**
     * Setups the model of this view.
     *
     * @param[in] pModel    The new model.
     */
    void setModel(const QSharedPointer<ANSHAREDLIB::FiffRawViewModel>& pModel);

    //=========================================================================================================
    /**
     * Broadcast channel scaling
     *
     * @param[in] scaleMap QMap with scaling values which is to be broadcasted to the model.
     */
    void setScalingMap(const QMap<qint32, float>& scaleMap);

    //=========================================================================================================
    /**
     * Set the signal color.
     *
     * @param[in] signalColor  The new signal color.
     */
    void setSignalColor(const QColor& signalColor);

    //=========================================================================================================
    /**
     * Broadcast the background color changes made in the QuickControl widget
     *
     * @param[in] backgroundColor  The new background color.
     */
    void setBackgroundColor(const QColor& backgroundColor);

    //=========================================================================================================
    /**
     * Sets new zoom factor
     *
     * @param[in] dZoomFac  time window size;.
     */
    void setZoom(double dZoomFac);

    //=========================================================================================================
    /**
     * Sets new time window size
     *
     * @param[in] iT  time window size;.
     */
    void setWindowSize(int iT);

    //=========================================================================================================
    /**
     * distanceTimeSpacerChanged changes the distance of the time spacers
     *
     * @param[in] iValue The new distance for the time spacers.
     */
    void setDistanceTimeSpacer(int iValue);

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot current view.
     *
     * @param[in] imageType  The current iamge type: png, svg.
     */
    void onMakeScreenshot(const QString& imageType);

    //=========================================================================================================
    /**
     * Brings up a menu for interacting with data events.
     * @param[in] pos   Position on screen where the menu will show up.
     */
    void customContextMenuRequested(const QPoint &pos);

    //=========================================================================================================
    /**
     * Create a new event
     *
     * @param[in] con   Whether a new annotaton should be created;.
     */
    void addTimeMark(bool con);

    //=========================================================================================================
    /**
     * Controls toggling of event data
     *
     * @param[in] iToggle   Sets toggle: 0 - don't show, 1+ - show.
     */
    void toggleDisplayEvent(const int& iToggle);

    //=========================================================================================================
    /**
     * Triggers a redraw of the data viewer
     */
    void updateView();

    //=========================================================================================================
    /**
     * Moves data viewer to a position where the selected event is in the middle of the viewer.
     */
    void updateScrollPositionToEvent();

    //=========================================================================================================
    /**
     * Filter parameters changed
     *
     * @param[in] filterData   the currently active filter.
     */
    void setFilter(const RTPROCESSINGLIB::FilterKernel &filterData);

    //=========================================================================================================
    /**
     * Filter avtivated
     *
     * @param[in] state    filter on/off flag.
     */
    void setFilterActive(bool state);

    //=========================================================================================================
    /**
     * Sets the type of channel which are to be filtered
     *
     * @param[in] channelType    the channel type which is to be filtered (EEG, MEG, All).
     */
    void setFilterChannelType(const QString& channelType);

    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     */
    void saveSettings();

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     */
    void loadSettings();

    //=========================================================================================================
    /**
     * Update the views GUI based on the set GuiMode (Clinical=0, Research=1).
     *
     * @param[in] mode     The new mode (Clinical=0, Research=1).
     */
    void updateGuiMode(GuiMode mode);

    //=========================================================================================================
    /**
     * Update the views GUI based on the set ProcessingMode (RealTime=0, Offline=1).
     *
     * @param[in] mode     The new mode (RealTime=0, Offline=1).
     */
    void updateProcessingMode(ProcessingMode mode);

    //=========================================================================================================
    /**
     * Shows channels based on input selectedChannelsIndexes
     *
     * @param[in] selectedChannelsIndexes      list of channels to be shown.
     */
    void showSelectedChannelsOnly(const QList<int> selectedChannelsIndexes);

    //=========================================================================================================
    /**
     * Shows all channels in the view
     */
    void showAllChannels();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView();

signals:
    //=========================================================================================================
    /**
     * Emits sample number to be added aan event
     *
     * @param[in] iSample      sample number to be added.
     */
    void sendSamplePos(int iSample);

private:
    //=========================================================================================================
    /**
     * resizeEvent reimplemented virtual function to handle resize events of the data dock window
     */
    void resizeEvent(QResizeEvent* event);

    //=========================================================================================================
    /**
     * Catches events to performa specific action handling
     *
     * @param[in, out] object      Pointer to object the event pertains to.
     * @param[in] event            Type of object with associated data.
     *
     * @return                      true if handled by custom event handling, false if not.
     */
    bool eventFilter(QObject *object, QEvent *event);

    //=========================================================================================================
    /**
     * Creates the lables for sample/time values displayed beneath the data view
     */
    void createBottomLabels();

    //=========================================================================================================
    /**
     * Triggers the update of the update label based on the data viewer horizontal positon (not on iValue)
     *
     * @param[in] iValue   Horizontal scroll bar position (unused).
     */
    void updateTimeLabels(int iValue);

    //=========================================================================================================
    /**
     * Updates file labels with info from current set model
     */
    void updateFileLabel();

    //=========================================================================================================
    /**
     * Updates the information about the filter shown in the filterLabel.
     */
    void updateFilterLabel();

    //=========================================================================================================
    /**
     * Disconnects the model from the view's scrollbar and resizing
     */
    void disconnectModel();

    //=========================================================================================================
    /**
     * This tells the model where the view currently is vertically.
     *
     * @param[in] newScrollPosition Absolute sample number.
     */
    void updateVerticalScrollPosition(qint32 newScrollPosition);

    //=========================================================================================================
    /**
     * Reloads the view to show the new real-time data.
     */
    void onNewRealtimeData();

    //=========================================================================================================
    /**
     * Initialize the objects necessary for the righ-click add event menu to appear.
     */
    void initRightClickContextMenu();

    QPointer<QTableView>                                m_pTableView;                   /**< Pointer to table view ui element. */

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>       m_pModel;                       /**< Pointer to associated Model. */

    QSharedPointer<FiffRawViewDelegate>                 m_pDelegate;                    /**< Pointer to associated Delegate. */

    QMap<qint32,float>                                  m_qMapChScaling;                /**< Channel scaling values. */

    float                                               m_fDefaultSectionSize;          /**< Default row height */
    float                                               m_fZoomFactor;                  /**< Zoom factor */
    int                                                 m_iLastClickedSample;            /**< Stores last clicked sample on screen */

    qint32                                              m_iT;                           /**< Display window size in seconds. */

    QScroller*                                          m_pKineticScroller;             /**< Used for kinetic scrolling through data view. */

    QLabel*                                             m_pInitialTimeLabel;            /**< Left 'Sample | Seconds' display label. */
    QLabel*                                             m_pEndTimeLabel;                /**< Right 'Sample | Seconds' display label. */
    QLabel*                                             m_pFileLabel;                   /**< File name and path, Fs and duration. */
    QLabel*                                             m_pFilterLabel;                 /**< Short filter description to be shown under the time-series. */
    QMenu*                                              m_pRightClickContextMenu;       /**< Hold the menu that appears when a right-click event occurs. */
    QAction*                                            m_pAddEventAction;              /**< Hold the action for directing callback for adding a new event. */
signals:
    void tableViewDataWidthChanged(int iWidth);

    void realtimeDataUpdated();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE RAWDATAVIEWERPLUGIN

#endif // FIFFRAWVIEW_H
