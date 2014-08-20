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
    m_pMainWindow((MainWindow*)parent)
{
    ui->setupUi(this);
    initCheckBoxes();
}


//*************************************************************************************************************

EventWindow::~EventWindow()
{
    delete ui;
}


//*************************************************************************************************************

void EventWindow::setupEventViewSettings()
{
    ui->m_tableView_eventTableView->resizeColumnsToContents();

    //Connect selection in event window to specific slot
    connect(ui->m_tableView_eventTableView->selectionModel(),&QItemSelectionModel::currentRowChanged,
                this,&EventWindow::jumpToEvent);
}


//*************************************************************************************************************

QTableView* EventWindow::getTableView()
{
    return ui->m_tableView_eventTableView;
}


//*************************************************************************************************************

void EventWindow::initCheckBoxes()
{
    connect(ui->m_checkBox_activateEvents,&QCheckBox::stateChanged, [=](int state){
        m_pMainWindow->m_pRawDelegate->m_bActivateEvents = state;
        jumpToEvent(m_pMainWindow->m_pEventTableView->selectionModel()->currentIndex(), QModelIndex());
    });

    connect(ui->m_checkBox_showSelectedEventsOnly,&QCheckBox::stateChanged, [=](int state){
        m_pMainWindow->m_pRawDelegate->m_bShowSelectedEventsOnly = state;
        jumpToEvent(m_pMainWindow->m_pEventTableView->selectionModel()->currentIndex(), QModelIndex());
    });
}


//*************************************************************************************************************

void EventWindow::jumpToEvent(const QModelIndex & current, const QModelIndex & previous)
{
    Q_UNUSED(previous);

    //Always get the first column 0 (sample) of the model
    QModelIndex index = m_pMainWindow->m_pEventModel->index(current.row(), 0);

    //Get the sample value
    int sample = m_pMainWindow->m_pEventModel->data(index, Qt::DisplayRole).toInt();

    //Jump to sample - put sample in the middle of the view
    int rawTableViewColumnWidth = m_pMainWindow->m_pRawTableView->viewport()->width();

    if(sample-rawTableViewColumnWidth/2 < rawTableViewColumnWidth/2) //events lie in the first half of the data window at the beginning of the loaded data -> cannot centralize view on event
         m_pMainWindow->m_pRawTableView->horizontalScrollBar()->setValue(0);
    else if(sample+rawTableViewColumnWidth/2 > m_pMainWindow->m_pRawModel->lastSample()-rawTableViewColumnWidth/2) //events lie in the last half of the data window at the end of the loaded data -> cannot centralize view on event
        m_pMainWindow->m_pRawTableView->horizontalScrollBar()->setValue(m_pMainWindow->m_pRawTableView->maximumWidth());
    else //centralize view on event
        m_pMainWindow->m_pRawTableView->horizontalScrollBar()->setValue(sample-rawTableViewColumnWidth/2);

    qDebug()<<"Jumping to Event at sample "<<sample<<"rawTableViewColumnWidth"<<rawTableViewColumnWidth;
}
