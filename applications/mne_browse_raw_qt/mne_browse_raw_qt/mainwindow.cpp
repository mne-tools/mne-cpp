//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     January, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implements the mainwindow function of mne_browse_raw_qt
*
*/

//*************************************************************************************************************
// INCLUDES

#include "mainwindow.h"
#include "info.h"

//Qt
#include <QDebug>

#include <QWidget>
#include <QPainter>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>

//#include "plotsignalwidget.h"

//*************************************************************************************************************
//namespaces

using namespace MNE_BROWSE_RAW_QT;

//*************************************************************************************************************

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_qFileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
{
    //setup MVC
    setupModel();
    setupDelegate();
    setupView();

    createMenus();

    createLogDockWindow();
    setWindow();
}

//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

void MainWindow::setupModel()
{
    m_pRawModel = new RawModel(m_qFileRaw,this);
}

void MainWindow::setupDelegate()
{
    m_pRawDelegate = new RawDelegate(this);
}

void MainWindow::setupView()
{
    m_pTableView = new QTableView;

    m_pTableView->setModel(m_pRawModel); //set custom model
    m_pTableView->setItemDelegate(m_pRawDelegate); //set custom delegate

    //set some size settings for m_pTableView
    m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    //TableView settings
    m_pTableView->setShowGrid(false);
//    m_pTableView->verticalHeader()->hide();
    m_pTableView->horizontalHeader()->hide();
    m_pTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_dPlotHeight);

    m_pTableView->setAutoScroll(false);
    m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1
//    m_pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_pTableView->resizeColumnToContents(0); //based on returned sizeHint of RawDelegate
//    m_pTableView->resizeColumnsToContents();

    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    //obtain position of QScrollArea
    m_pTableView->horizontalScrollBar()->setMinimum(0);
    m_pTableView->horizontalScrollBar()->setValue(m_pRawModel->sizeOfData()/2-this->width()/2);
    m_pTableView->horizontalScrollBar()->setMaximum(m_pRawModel->sizeOfData()*m_pRawDelegate->m_dDx);
    qDebug("INFO: ScrollBar: Position: %i, Min %i, Max %i",m_pTableView->horizontalScrollBar()->value(),m_pTableView->horizontalScrollBar()->minimum(),m_pTableView->horizontalScrollBar()->maximum());

    QScroller::grabGesture(m_pTableView,QScroller::MiddleMouseButtonGesture); //activate kinetic scrolling

    //connect QScrollBar with model in order to reload data samples
    connect(m_pTableView->horizontalScrollBar(),SIGNAL(valueChanged(int)),m_pRawModel,SLOT(reloadData(int)));

    //*****************************
//    //example for PlotSignalWidget
//    QFile t_rawFile("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif");
//    FiffIO m_fiffIO(t_rawFile);

//    MatrixXd t_samples,t_times;
//    m_fiffIO.m_qlistRaw[0]->read_raw_segment_times(t_samples,t_times,100,102);
//    MatrixXd t_data;
//    t_data.resize(2,t_samples.cols());
//    t_data.row(0) = t_samples.row(0);
//    t_data.row(1) = t_times;
//    //generate PlotSignalWidget
//    PlotSignalWidget *plotSignalWidget = new PlotSignalWidget(t_data,this);
//    plotSignalWidget->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    //*****************************

    //set vertical layout
    QVBoxLayout *mainlayout = new QVBoxLayout;

    mainlayout->addWidget(m_pTableView);

    //set layouts
    QWidget *window = new QWidget();
    window->setLayout(mainlayout);

    setCentralWidget(window);
}


void MainWindow::createMenus() {
    QMenu *fileMenu = new QMenu(tr("&File"), this);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcuts(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcuts(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    menuBar()->addMenu(fileMenu);
}

void MainWindow::setWindow() {
    setWindowTitle("MNE_BROWSE_RAW_QT");
    resize(1200,800);
    this->move(50,50);
}

//*************************************************************************************************************
//Log

void MainWindow::createLogDockWindow()
{
    //Log TextBrowser
    m_pDockWidget_Log = new QDockWidget(tr("Log"), this);

    m_pTextBrowser_Log = new QTextBrowser(m_pDockWidget_Log);

    m_pDockWidget_Log->setWidget(m_pTextBrowser_Log);

    m_pDockWidget_Log->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_pDockWidget_Log);

    //Set standard LogLevel
    setLogLevel(_LogLvMax);
}

void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    if(lglvl<=m_eLogLevelCurrent) {
        if(lgknd == _LogKndError)
            m_pTextBrowser_Log->insertHtml("<font color=red><b>Error:</b> "+logMsg+"</font>");
        else if(lgknd == _LogKndWarning)
            m_pTextBrowser_Log->insertHtml("<font color=blue><b>Warning:</b> "+logMsg+"</font>");
        else
            m_pTextBrowser_Log->insertHtml(logMsg);
        m_pTextBrowser_Log->insertPlainText("\n"); // new line
        //scroll down to the newest entry
        QTextCursor c = m_pTextBrowser_Log->textCursor();
        c.movePosition(QTextCursor::End);
        m_pTextBrowser_Log->setTextCursor(c);

        m_pTextBrowser_Log->verticalScrollBar()->setValue(m_pTextBrowser_Log->verticalScrollBar()->maximum());
    }
}

void MainWindow::setLogLevel(LogLevel lvl)
{
    switch(lvl) {
    case _LogLvMin:
        writeToLog(tr("minimal log level set"), _LogKndMessage, _LogLvMin);
        break;
    case _LogLvNormal:
        writeToLog(tr("normal log level set"), _LogKndMessage, _LogLvMin);
        break;
    case _LogLvMax:
        writeToLog(tr("maximum log level set"), _LogKndMessage, _LogLvMin);
        break;
    }

    m_eLogLevelCurrent = lvl;
}

//*************************************************************************************************************

void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));
    QFile t_fileRaw(filename);

    m_pRawModel->loadFiffData(t_fileRaw);
    //ui->textEdit->setText(filename);
}

