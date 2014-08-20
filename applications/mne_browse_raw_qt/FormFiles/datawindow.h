//=============================================================================================================
/**
* @file     datawindow.h
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
#include "ui_datawindowdock.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>
#include <QResizeEvent>
#include <QToolBar>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBrowseRawQt
//=============================================================================================================

namespace MNEBrowseRawQt
{

/**
* DECLARE CLASS DataWindow
*
* @brief The DataWindow class provides the data dock window.
*/
class DataWindow : public QDockWidget
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
    * Setup the table view of the data window
    */
    void setupRawViewSettings();

    //=========================================================================================================
    /**
    * Returns the QTableView of this window
    */
    QTableView* getTableView();

    //=========================================================================================================
    /**
    * setWindowStatus sets the window status depending on m_pRawModel->m_bFileloaded
    */
    void setWindowStatus();

private:
    //=========================================================================================================
    /**
    * Setup the tool bar of the data window.
    */
    void setupToolBar();

    //=========================================================================================================
    /**
    * event reimplemented virtual function to handle the resizing of the data dock window
    */
    bool event(QEvent * event);

    Ui::DataWindowDockWidget *ui;

    MainWindow* m_pMainWindow;

protected slots:
    //=========================================================================================================
    /**
    * @brief customContextMenuRequested
    * @param pos is the position, where the right-click occurred
    */
    void customContextMenuRequested(QPoint pos);
};

} // NAMESPACE MNEBrowseRawQt

#endif // DATAWINDOW_H
