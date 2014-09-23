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
* @brief    Contains the implementation of the EventWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventWindow::EventWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::EventWindowDockWidget),
    m_pMainWindow(static_cast<MainWindow*>(parent))
{
    ui->setupUi(this);

    //------------------------
    //--- Setup data model ---
    //------------------------
    if(m_pMainWindow->m_qEventFile.exists())
        m_pEventModel = new EventModel(m_pMainWindow->m_qEventFile, this);
    else
        m_pEventModel = new EventModel(this);
}


//*************************************************************************************************************

EventWindow::~EventWindow()
{
    delete ui;
}


//*************************************************************************************************************

void EventWindow::init()
{
    initMVCSettings();
    initCheckBoxes();
    initComboBoxes();
}


//*************************************************************************************************************

QTableView* EventWindow::getEventTableView()
{
    return ui->m_tableView_eventTableView;
}


//*************************************************************************************************************

EventModel* EventWindow::getEventModel()
{
    return m_pEventModel;
}


//*************************************************************************************************************

EventDelegate* EventWindow::getEventDelegate()
{
    return m_pEventDelegate;
}


//*************************************************************************************************************

void EventWindow::initMVCSettings()
{
    //Set fiffInfo and first/last sample in the event model
    m_pEventModel->setFiffInfo(m_pMainWindow->m_pDataWindow->getDataModel()->m_fiffInfo);
    m_pEventModel->setFirstLastSample(m_pMainWindow->m_pDataWindow->getDataModel()->firstSample(),
                                      m_pMainWindow->m_pDataWindow->getDataModel()->lastSample());

    //set MVC model
    ui->m_tableView_eventTableView->setModel(m_pEventModel);

    //set MVC delegate
    m_pEventDelegate = new EventDelegate(this);
    ui->m_tableView_eventTableView->setItemDelegate(m_pEventDelegate);

    //Resize columns to contents
    ui->m_tableView_eventTableView->resizeColumnsToContents();

    //Connect selection in event window to jumpEvent slot
    connect(ui->m_tableView_eventTableView->selectionModel(),&QItemSelectionModel::currentRowChanged,
                this,&EventWindow::jumpToEvent);

    //Set MVC in delegate
    m_pEventDelegate->setModelView(m_pEventModel);
}


//*************************************************************************************************************

void EventWindow::initCheckBoxes()
{
    connect(ui->m_checkBox_activateEvents,&QCheckBox::stateChanged, [=](int state){
        m_pMainWindow->m_pDataWindow->getDataDelegate()->m_bActivateEvents = state;
        jumpToEvent(ui->m_tableView_eventTableView->selectionModel()->currentIndex(), QModelIndex());
    });

    connect(ui->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged, [=](int state){
        m_pMainWindow->m_pDataWindow->getDataDelegate()->m_bShowSelectedEventsOnly = state;
        jumpToEvent(ui->m_tableView_eventTableView->selectionModel()->currentIndex(), QModelIndex());
    });
}


//*************************************************************************************************************

void EventWindow::initComboBoxes()
{
    ui->m_comboBox_filterTypes->addItem("All");
    ui->m_comboBox_filterTypes->addItems(m_pEventModel->getEventTypeList());
    ui->m_comboBox_filterTypes->setCurrentText("All");

    //Connect filter types to event model
    connect(ui->m_comboBox_filterTypes,&QComboBox::currentTextChanged,
                this,[=](QString string){
            m_pEventModel->setEventFilterType(string);
    });

    connect(m_pEventModel,&EventModel::updateEventTypes,
            this, &EventWindow::updateComboBox);
}


//*************************************************************************************************************

void EventWindow::updateComboBox()
{
    ui->m_comboBox_filterTypes->clear();
    ui->m_comboBox_filterTypes->addItem("All");
    ui->m_comboBox_filterTypes->addItems(m_pEventModel->getEventTypeList());
    ui->m_comboBox_filterTypes->setCurrentText("All");
}


//*************************************************************************************************************

bool EventWindow::event(QEvent * event)
{
    //On resize event center marker again
    if(event->type() == QEvent::Resize) {
        qDebug()<<"resize";
        jumpToEvent(ui->m_tableView_eventTableView->selectionModel()->currentIndex(), QModelIndex());
    }

    //Delete selected row on delete key press event
    if(event->type() == QEvent::KeyPress && ui->m_tableView_eventTableView->hasFocus()) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Delete) {
            QModelIndexList indexList = ui->m_tableView_eventTableView->selectionModel()->selectedIndexes();

            for(int i = 0; i<indexList.size(); i++)
                m_pEventModel->removeRow(indexList.at(i).row() - i); // - i because the internal data structure gets smaller by one with each succession in this for statement
       }
    }

    return QDockWidget::event(event);
}


//*************************************************************************************************************

void EventWindow::jumpToEvent(const QModelIndex & current, const QModelIndex & previous)
{
    Q_UNUSED(previous);

    if(ui->m_checkBox_activateEvents->isChecked()) {
        //Always get the first column 0 (sample) of the model - Note: Need to map index from sorting model back to source model
        QModelIndex index = m_pEventModel->index(current.row(), 0);

        //Get the sample value
        int sample = m_pEventModel->data(index, Qt::DisplayRole).toInt();

        //Jump to sample - put sample in the middle of the view - the viewport holds the width of the are which is changed through scrolling
        int rawTableViewColumnWidth = m_pMainWindow->m_pDataWindow->getDataTableView()->viewport()->width();

        if(sample-rawTableViewColumnWidth/2 < rawTableViewColumnWidth/2) //events lie in the first half of the data window at the beginning of the loaded data -> cannot centralize view on event
            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(0);
        else if(sample+rawTableViewColumnWidth/2 > m_pMainWindow->m_pDataWindow->getDataModel()->lastSample()-rawTableViewColumnWidth/2) //events lie in the last half of the data window at the end of the loaded data -> cannot centralize view on event
            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(m_pMainWindow->m_pDataWindow->getDataTableView()->maximumWidth());
        else //centralize view on event
            m_pMainWindow->m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(sample-rawTableViewColumnWidth/2);

        qDebug()<<"Jumping to Event at sample "<<sample<<"rawTableViewColumnWidth"<<rawTableViewColumnWidth;
    }
}
