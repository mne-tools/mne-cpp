//=============================================================================================================
/**
 * @file     eventwindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  1.0
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
 * @brief    Contains the declaration of the EventWindow class.
 *
 */

#ifndef EVENTWINDOW_H
#define EVENTWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"
#include "ui_eventwindowdock.h"
#include "../Delegates/eventdelegate.h"
#include "../Models/eventmodel.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QColorDialog>


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

/**
 * DECLARE CLASS EventWindow
 *
 * @brief The EventWindow class provides the event dock window.
 */
class EventWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a EventWindow dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new EventWindow becomes a window. If parent is another widget, EventWindow becomes a child window inside parent. EventWindow is deleted when its parent is deleted.
    */
    EventWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the EventWindow.
    * All EventWindow's children are deleted first. The application exits if EventWindow is the main widget.
    */
    ~EventWindow();

    //=========================================================================================================
    /**
    * Initialises this window.
    */
    void init();

    //=========================================================================================================
    /**
    * Returns the QTableView of this window.
    */
    QTableView* getEventTableView();

    //=========================================================================================================
    /**
    * Returns the EventModel of this window
    */
    EventModel* getEventModel();

    //=========================================================================================================
    /**
    * Returns the EventModel of this window
    */
    EventDelegate* getEventDelegate();

private:
    //=========================================================================================================
    /**
    * Setup the model view controller of the event window.
    */
    void initMVCSettings();

    //=========================================================================================================
    /**
    * Inits all the QCheckBoxes of the event window.
    */
    void initCheckBoxes();

    //=========================================================================================================
    /**
    * Inits all the QCheckBoxes of the event window.
    */
    void initComboBoxes();

    //=========================================================================================================
    /**
    * Inits all the QCheckBoxes of the event window.
    */
    void initToolButtons();

    //=========================================================================================================
    /**
    * Inits all the QPushButtons of the event window.
    */
    void initPushButtons();

    //=========================================================================================================
    /**
    * Updates the event filter type combo box whenever a new event file was loaded
    */
    void updateComboBox(const QString &currentEventType);

    //=========================================================================================================
    /**
    * event reimplemented virtual function to handle events of the event dock window
    */
    bool event(QEvent * event);

    Ui::EventWindowDockWidget *ui;                  /**< Pointer to the qt designer generated ui class.*/

    MainWindow*         m_pMainWindow;              /**< Pointer to the parent, the MainWindow class.*/

    QSettings           m_qSettings;                /**< QSettings variable used to write or read from independent application sessions. */

    EventDelegate*      m_pEventDelegate;           /**< the QAbstractDelegate being part of the event model/view framework of Qt. */
    EventModel*         m_pEventModel;              /**< the QAbstractTable event model being part of the model/view framework of Qt. */

    QColorDialog*       m_pColordialog;             /**< The qt color dialog for changing event type colors.*/

protected slots:
    //=========================================================================================================
    /**
    * jumpToEvent jumps to a event specified in the event table view
    *
    * @param [in] current model item focused in the view
    * @param [in] previous model item focused in the view
    */
    void jumpToEvent(const QModelIndex &current, const QModelIndex &previous);

    //=========================================================================================================
    /**
    * jumpToEvent jumps to a event specified in the event table view
    */
    void removeEventfromEventModel();

    //=========================================================================================================
    /**
    * Adds an event to the event model and its QTableView
    */
    void addEventToEventModel();

    //=========================================================================================================
    /**
    * call this function whenever a new event type is to be added
    */
    void addNewEventType();
};

} // NAMESPACE MNEBROWSE

#endif // EVENTWINDOW_H
