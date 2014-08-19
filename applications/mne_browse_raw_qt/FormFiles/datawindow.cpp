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


    QToolBar *toolBar = new QToolBar();
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setFixedWidth(30);
    toolBar->setMovable(false);
    toolBar->addAction(new QAction("Test", this));

    ui->horizontalLayout->addWidget(toolBar);

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
            this,&DataWindow::customContextMenuRequested);

    //activate kinetic scrolling
    QScroller::grabGesture(ui->m_tableView_rawTableView,QScroller::MiddleMouseButtonGesture);

    //connect QScrollBar with model in order to reload data samples
    connect(ui->m_tableView_rawTableView->horizontalScrollBar(),SIGNAL(valueChanged(int)),
            m_pMainWindow->m_pRawModel,SLOT(updateScrollPos(int)));
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


//*************************************************************************************************************

bool DataWindow::event(QEvent * event)
{
    if(event->type() == QEvent::Paint) {
        //Manually resize QDockWidget - This needs to be done because there is no typical central widget in QMainWindow - QT does not do a good job when resizing dock widgets (known issue)
        int newWidth;

        if(m_pMainWindow->m_pEventWindow->isHidden() || m_pMainWindow->m_pEventWindow->isFloating())
        {
            newWidth = m_pMainWindow->size().width() - m_pMainWindow->centralWidget()->size().width() - 1;
            //qDebug()<<"resize data plot event window is hidden";
        }
        else
        {
            newWidth = m_pMainWindow->size().width() - m_pMainWindow->m_pEventWindow->size().width() - 5;
            //qDebug()<<"resize data plot event window is visible";
        }

        resize(newWidth, this->size().height());
    }

    return QDockWidget::event(event);
}


//*************************************************************************************************************

void DataWindow::customContextMenuRequested(QPoint pos)
{
    //obtain index where index was clicked
    //QModelIndex index = m_pRawTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = m_pMainWindow->m_pRawTableView->selectionModel()->selectedIndexes();

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Marking ****************
    QMenu *markingSubMenu = new QMenu("Mark channels",menu);

    QAction* doMarkChBad = markingSubMenu->addAction(tr("Mark as bad"));
    connect(doMarkChBad,&QAction::triggered, [=](){
        m_pMainWindow->m_pRawModel->markChBad(selected,1);
    });

    QAction* doMarkChGood = markingSubMenu->addAction(tr("Mark as good"));
    connect(doMarkChGood,&QAction::triggered, [=](){
        m_pMainWindow->m_pRawModel->markChBad(selected,0);
    });

    //**************** FilterOperators ****************
    //selected channels
    QMenu *filtOpSubMenu = new QMenu("Apply FilterOperator to selected channel",menu);
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pMainWindow->m_pRawModel->m_Operators);
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pMainWindow->m_pRawModel->applyOperator(selected,it.value());
        });
    }

    //all channels
    QMenu *filtOpAllSubMenu = new QMenu("Apply FilterOperator to all channels",menu);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpAllSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pMainWindow->m_pRawModel->applyOperator(QModelIndexList(),it.value());
        });
    }

    //undoing filtering
    QMenu *undoFiltOpSubMenu = new QMenu("Undo filtering",menu);
    QMenu *undoFiltOpSelSubMenu = new QMenu("to selected channels",undoFiltOpSubMenu);

    //undo certain FilterOperators to selected channels
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* undoApplyFilter = undoFiltOpSelSubMenu->addAction(tr("%1").arg(it.key()));

        connect(undoApplyFilter,&QAction::triggered, [=](){
            m_pMainWindow->m_pRawModel->undoFilter(selected,it.value());
        });
    }

    undoFiltOpSubMenu->addMenu(undoFiltOpSelSubMenu);

    //undo all filterting to selected channels
    QAction* undoApplyFilterSel = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to selected channels"));
    connect(undoApplyFilterSel,&QAction::triggered, [=](){
        m_pMainWindow->m_pRawModel->undoFilter(selected);
    });

    //undo all filtering to all channels
    QAction* undoApplyFilterAll = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to all channels"));
    connect(undoApplyFilterAll,&QAction::triggered, [=](){
        m_pMainWindow->m_pRawModel->undoFilter();
    });

    //add everything to main contextmenu
    menu->addMenu(markingSubMenu);
    menu->addMenu(filtOpSubMenu);
    menu->addMenu(filtOpAllSubMenu);
    menu->addMenu(undoFiltOpSubMenu);

    //show context menu
    menu->popup(m_pMainWindow->m_pRawTableView->viewport()->mapToGlobal(pos));
}
