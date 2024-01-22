//=============================================================================================================
/**
 * @file     eventview.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.9
 * @date     March, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Christoph Dinh, Lorenz Esch, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Declaration of the EventView Class.
 *
 */

#ifndef EVENTVIEW_H
#define EVENTVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_info.h>
#include <anShared/Model/eventmodel.h>
#include "eventdelegate.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QColorDialog>
#include <QStringListModel>
#include <QFuture>
#include <QFutureWatcher>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class TriggerDetectionView;
}

namespace ANSHAREDLIB {
    class FiffRawViewModel;
}

namespace Ui {
    class EventWindowDockWidget;
}

namespace FIFFLIB {
    class FiffInfo;
    class FiffRawData;
}

//=============================================================================================================
/**
 * EventView
 *
 * @brief The EventView class provides the GUI for adding and removing event.
 */
class EventView : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructor
     */
    EventView();

    ~EventView();

    //=========================================================================================================
    /**
     * Resets the view to its default settings.
     */
    void reset();

    //=========================================================================================================
    /**
     * Passes a shared pointer to the event model triggers all the relevant init functions
     *
     * @param[in] pEventModel    Pointer to the event model of the current loaded file.
     */
    void setModel(QSharedPointer<ANSHAREDLIB::EventModel> pEventModel);

    //=========================================================================================================
    /**
     * Disconnects from current model for switching between files
     */
    void disconnectFromModel();

    //=========================================================================================================
    /**
     * Clears the view
     */
    void clearView(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

public slots:
    //=========================================================================================================
    /**
     * Adds input parameter to event model as a new event.
     *
     * @param[in] iSample   Sample number to be added to event model.
     */
    void addEventToModel(int iSamplePos);

    //=========================================================================================================
    /**
     * Update pointer to currently loaded FiffRawViewModel
     *
     * @param[in] pFiffRawModel     saves pointer to ucurrently loaded FiffRawViewModel.
     */
    void onNewFiffRawViewModel(QSharedPointer<ANSHAREDLIB::FiffRawViewModel> pFiffRawModel);

signals:
    //=========================================================================================================
    /**
     * Emits state of the Activate Events checkbox
     *
     * @param[in] iCheckBoxState   0 for unchecked, 1 for checked.
     */
    void activeEventsChecked(const int& iCheckBoxState);

    //=========================================================================================================
    /**
     * Used to force the fiffrawviewer to redraw the data plot.
     */
    void triggerRedraw();

    //=========================================================================================================
    /**
     * Notifies that the number/names of groups have changed
     */
    void eventsUpdated();

    //=========================================================================================================
    /**
     * Tells view to move to location of selected event
     */
    void jumpToSelected();

    //=========================================================================================================
    /**
     * ends event to trigger progress bar to appear
     *
     * @param[in] sMessage     message to appear.
     */
    void loadingStart(QString sMessage = "Loading Events...");

    //=========================================================================================================
    /**
     * Sends event to trigger progress bar to be hidden
     */
    void loadingEnd(QString sMessage = "Loading Events...");

protected slots:

    //=========================================================================================================
    /**
     * Removes currently selected event from model.
     */
    void removeEvent();

    //=========================================================================================================
    /**
     * Creates new event of the type currently on the Spin Box widget.
     */
    void addEventGroup();

    //=========================================================================================================
    /**
     * Transmits the checkbox state of the 'Show events in Signal Viewer' checkbox
     *
     * @param[in] iCheckBoxState    0 for unchecked, 2 for checked.
     */
    void onActiveEventsChecked(int iCheckBoxState);

    //=========================================================================================================
    /**
     * Transmits the checkbox state of the 'Show selected events only' checkbox
     *
     * @param[in] iCheckBoxState    0 for unchecked, 2 for checked.
     */
    void onSelectedEventsChecked(int iCheckBoxState);

    //=========================================================================================================
    /**
     * Sets the current selected event in the gui and passes it to the model
     */
    void onCurrentSelectedChanged();

    //=========================================================================================================
    /**
     * Used to handle key presses to interact with events.
     *
     * @param[in] event    a key press event.
     */
    void keyReleaseEvent(QKeyEvent* event);

private slots:
    //=========================================================================================================
    /**
     * Forces the dock widget to be redrawn.
     */
    void onDataChanged();

    //=========================================================================================================
    /**
     * Triggers the save file dialog. Calls the save file function in the Event Model
     */
    void onSaveButton();

    //=========================================================================================================
    /**
     * Populates the trigger detect view with the correct channel info
     *
     * @param[in] info  data for the trigger view to get a list of the stim channels.
     */
    void initTriggerDetect(const QSharedPointer<FIFFLIB::FiffInfo> info);

    //=========================================================================================================
    /**
     * Gets event map from QFuture and creates new groups baseed on it.
     */
    void createGroupsFromTriggers();

    //=========================================================================================================
    /**
     * Redraws groups based on EventModel
     */
    void redrawGroups();

private:
    //=========================================================================================================
    /**
     * Creates and connects GUI items to work with view class.
     */
    void initGUIFunctionality();

    //=========================================================================================================
    /**
     * Links delegate, model and view.
     */
    void initMVCSettings();

    //=========================================================================================================
    /**
     * Creates a new user-made group of events
     *
     * @param[in] sName     group name.
     * @param[in] iType     group default type.
     */
    bool newUserGroup(const QString& sName,
                      int iType = 0,
                      bool bDefaultColor = false);

    //=========================================================================================================
    /**
     * Creates a new group based on Stim channel triggers
     *
     * @param[in] sName         group name.
     * @param[in] iType         group default type.
     * @param[in] groupColor    group color.
     *
     * @return      returns whether group creation was succesful.
     */
    bool newStimGroup(const QString& sName,
                      int iType,
                      const QColor &groupColor = Qt::black);

    //=========================================================================================================
    /**
     * Deletes selected event group
     */
    void deleteGroup();

    //=========================================================================================================
    /**
     * Changes currently loaded group based on selection in the group list view
     */
    void groupChanged();

    //=========================================================================================================
    /**
     * Prompts user to rename selected group
     */
    void renameGroup(const QString &currentText);

    //=========================================================================================================
    /**
     * Gives user popup to select new color for selected group.
     */
    void changeGroupColor();

    //=========================================================================================================
    /**
     * Brings up a menu for interacting with events
     *
     * @param[in] pos   Position on screen where the menu will show up.
     */
    void customEventContextMenuRequested(const QPoint &pos);

    //=========================================================================================================
    /**
     * Brings up a menu for interacting with event groups
     *
     * @param[in] pos   Position on screen where the menu will show up.
     */
    void customGroupContextMenuRequested(const QPoint &pos);

    //=========================================================================================================
    /**
     * Makes TriggerDetectView vidisble and active
     */
    void onStimButtonClicked();

    //=========================================================================================================
    /**
     * Used to pass parameters of the currently loaded fif file to the event model.
     *
     * @param[in] iFirst   Sample number of the first sample in the file.
     * @param[in] iLast    Sample number of the last sample in the file.
     * @param[in] fFreq    Sample frequency for the data in the file.
     */
    void passFiffParams(int iFirst,
                        int iLast,
                        float fFreq);

    //=========================================================================================================
    /**
     * Starts trigger detection in separate thread
     *
     * @param[in] sChannelName      name of stim channel from which we will be reading.
     * @param[in] dThreshold        threshold for a spike to count as a trigger.
     */
    void onDetectTriggers(const QString& sChannelName,
                          double dThreshold);

    //=========================================================================================================
    /**
     * Perfroms trigger detection and sorts events into map of events by group based on detection threshold
     *
     * @param[in] sChannelName      name of stim channel from which we will be reading.
     * @param[in] dThreshold        threshold for a spike to count as a trigger.
     *
     * @return      returns map of events sorted by groups based on threshold.
     */
    QMap<double,QList<int>> detectTriggerCalculations(const QString& sChannelName,
                                                      double dThreshold,
                                                      FIFFLIB::FiffInfo fiffInfo,
                                                      FIFFLIB::FiffRawData fiffRaw);

    //=========================================================================================================
    /**
     * Triggered when group item in Group ListWidget has its name changed. Triggers an update to the name in the back end.
     *
     * @param[in] item      Item whose name was changed
     */
    void onGroupItemNameChanged(QListWidgetItem *item);

    void createContextMenu();

    Ui::EventWindowDockWidget*                      m_pUi;                          /** < Pointer to GUI elements */

    int                                             m_iCheckState;                  /** < State of show events checkbox (0 unchecked, 2 checked) */
    int                                             m_iCheckSelectedState;          /** < State of the show selected checkbox (0 unchecked, 2 checked) */
    int                                             m_iLastSampClicked;             /** < Number of the last sample clicked */

    QSharedPointer<EventDelegate>                   m_pAnnDelegate;                 /** < Pointer to associated delegate */
    QSharedPointer<ANSHAREDLIB::EventModel>         m_pEventModel;                  /** < Pointer to associated model. Points to currently loaded. */
    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>   m_pFiffRawModel;                /** < Pointer to currently loaded FIffRawViewModel */

    QSharedPointer<DISPLIB::TriggerDetectionView>   m_pTriggerDetectView;           /** < Pointer to viewer to control GUI for detecting triggers */

    QColorDialog*                                   m_pColordialog;                 /** < Used for Prompting users for event type colors */

    QFutureWatcher <QMap<double,QList<int>>>        m_FutureWatcher;                /** < Watches m_Future and signals when calculations are done */
    QFuture<QMap<double,QList<int>>>                m_Future;                       /** < Used to perfom trigger detection on a separate thread */

    QPointer<QMenu>                                 m_pEventContexMenu;
    QPointer<QMenu>                                 m_pGroupContexMenu;
};

#endif // EVENTVIEW_H
