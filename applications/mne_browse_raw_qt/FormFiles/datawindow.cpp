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
* @brief    Contains the implementation of the DataWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "datawindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataWindow::DataWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DataWindowDockWidget),
    m_pMainWindow((MainWindow*)parent)
{
    ui->setupUi(this);
}


//*************************************************************************************************************

DataWindow::~DataWindow()
{
    delete ui;
}


//*************************************************************************************************************

void DataWindow::setupRawViewSettings()
{
    //set some settings for m_pRawTableView
    ui->m_tableView_rawTableView->verticalHeader()->setDefaultSectionSize(m_pMainWindow->m_pRawDelegate->m_dDefaultPlotHeight);
    ui->m_tableView_rawTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
    ui->m_tableView_rawTableView->resizeColumnsToContents();

    //set context menu
    ui->m_tableView_rawTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->m_tableView_rawTableView,&QWidget::customContextMenuRequested,
            m_pMainWindow,&MainWindow::customContextMenuRequested);

    //activate kinetic scrolling
    QScroller::grabGesture(ui->m_tableView_rawTableView,QScroller::MiddleMouseButtonGesture);

    //connect QScrollBar with model in order to reload data samples
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),SIGNAL(valueChanged(int)),
            m_pMainWindow->m_pRawModel,SLOT(updateScrollPos(int)));

    //connect other signals
    connect(m_pMainWindow->m_pRawModel,&RawModel::scrollBarValueChange,
            m_pMainWindow,&MainWindow::setScrollBarPosition);
}

//*************************************************************************************************************

QTableView* DataWindow::getTableView()
{
    return ui->m_tableView_rawTableView;
}


//*************************************************************************************************************

void DataWindow::setWindowStatus()
{
    QString title;

    //request status
    if(m_pMainWindow->m_pRawModel->m_bFileloaded) {
        int idx = m_pMainWindow->m_qFileRaw.fileName().lastIndexOf("/");
        QString filename = m_pMainWindow->m_qFileRaw.fileName().remove(0,idx+1);
        title = QString("%1 - File loaded: %2").arg("Data plot").arg(filename);
    }
    else {
        title = QString("%1 - No File loaded").arg("Data plot");
    }

    //set title
    setWindowTitle(title);
}
