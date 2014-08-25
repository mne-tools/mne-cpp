//=============================================================================================================
/**
* @file     eventwindow.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
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
    * Setup the QtableView of the event window.
    */
    void initEventViewSettings();

    //=========================================================================================================
    /**
    * Returns the QTableView of this window.
    */
    QTableView* getTableView();

private:
    //=========================================================================================================
    /**
    * Inits all the QCheckBoxes of the event window.
    */
    void initCheckBoxes();

    //=========================================================================================================
    /**
    * event reimplemented virtual function to handle events of the event dock window
    */
    bool event(QEvent * event);

    Ui::EventWindowDockWidget *ui;

    MainWindow*     m_pMainWindow;

    QSettings       m_qSettings;

protected slots:
    //=========================================================================================================
    /**
    * jumpToEvent jumps to a event specified in the event table view
    *
    * @param [in] current model item focused in the view
    * @param [in] previous model item focused in the view
    */
    void jumpToEvent(const QModelIndex &current, const QModelIndex &previous);
};

} // NAMESPACE MNEBrowseRawQt

#endif // EVENTWINDOW_H
