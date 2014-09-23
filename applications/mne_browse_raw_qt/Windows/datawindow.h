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
#include <QToolBar>
#include <QPainter>
#include <QColor>


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
    * Returns the event QTableView of this window
    */
    QTableView* getDataTableView();

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

private:
    //=========================================================================================================
    /**
    * Setup the model view controller of the data window
    */
    void initMVCSettings();

    //=========================================================================================================
    /**
    * Setup the undocked data view window.
    */
    void initUndockedWindow();

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

    Ui::DataWindowDockWidget *ui;                   /**< the ui variabe to initalise and access the ui file with this class */

    MainWindow*     m_pMainWindow;                  /**< pointer to the main window (parent) */

    QSettings       m_qSettings;                    /**< global mne browse raw qt settings */

    QWidget*        m_pUndockedViewWidget;          /**< widget which is shown whenever the view undock action is triggered */

    DataMarker*     m_pDataMarker;                  /**< pointer to the data marker */

    QLabel*         m_pCurrentDataMarkerLabel;      /**< the current data marker label to display the marker's position */

    int             m_iCurrentMarkerSample;         /**< the current data marker sample value to display the marker's position */

    RawDelegate*    m_pRawDelegate;                 /**< the QAbstractDelegate being part of the raw model/view framework of Qt */
    RawModel*       m_pRawModel;                    /**< the QAbstractTable model being part of the model/view framework of Qt */

    QTableView*     m_pUndockedDataView;
    QVBoxLayout*    m_pUndockedDataViewLayout;

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
    * Adds an event to the event model and its QTableView
    */
    void addEventToEventModel();

    //=========================================================================================================
    /**
    * Undock the table view to a normal window (not dock widget)
    */
    void undockDataViewToWindow();

    //=========================================================================================================
    /**
    * Updates the marker position
    */
    void updateMarkerPosition();
};

} // NAMESPACE MNEBrowseRawQt

#endif // DATAWINDOW_H
