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
* Copyright (C) 2014, Florian Schlembach, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    mne_browse_raw_qt is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
*           to reimplement the full feature set of mne_browse_raw and even extend these.
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBrowseRawQt;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_qFileRaw("./MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
, m_qSettings()
, m_rawSettings()
{
    //setup MVC
    setupModel();
    setupDelegate();
    setupView();
    setupLayout();

    createMenus();

    createLogDockWindow();

    setWindow();
    setWindowStatus();
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

void MainWindow::setupModel()
{
//    m_pRawModel = new RawModel(this);
    m_pRawModel = new RawModel(m_qFileRaw,this);
}


//*************************************************************************************************************

void MainWindow::setupDelegate()
{
    m_pRawDelegate = new RawDelegate(this);
}


//*************************************************************************************************************

void MainWindow::setupView()
{
    m_pTableView = new QTableView;

    m_pTableView->setModel(m_pRawModel); //set custom model
    m_pTableView->setItemDelegate(m_pRawDelegate); //set custom delegate

    //TableView settings
    setupViewSettings();

}


//*************************************************************************************************************

void MainWindow::setupLayout() {
    //set vertical layout
    QVBoxLayout *mainlayout = new QVBoxLayout;

    mainlayout->addWidget(m_pTableView);

    //set layouts
    QWidget *window = new QWidget();
    window->setLayout(mainlayout);

    setCentralWidget(window);
}


//*************************************************************************************************************

void MainWindow::setupViewSettings() {
    //set some size settings for m_pTableView
    m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    m_pTableView->setShowGrid(false);
    m_pTableView->horizontalHeader()->hide();
    m_pTableView->verticalHeader()->setDefaultSectionSize(m_pRawDelegate->m_dDefaultPlotHeight);

    m_pTableView->setAutoScroll(false);
    m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

    m_pTableView->resizeColumnsToContents();

    m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    //set context menu
    m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(customContextMenuRequested(QPoint)));

    //activate kinetic scrolling
    QScroller::grabGesture(m_pTableView,QScroller::MiddleMouseButtonGesture);

    //connect QScrollBar with model in order to reload data samples
    connect(m_pTableView->horizontalScrollBar(),SIGNAL(valueChanged(int)),m_pRawModel,SLOT(updateScrollPos(int)));

    //connect other signals
    connect(m_pRawModel,SIGNAL(scrollBarValueChange(int)),this,SLOT(setScrollBarPosition(int)));
}


//*************************************************************************************************************

void MainWindow::createMenus() {
    //File
    QMenu *fileMenu = new QMenu(tr("&File"), this);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcuts(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    QAction *writeAction = fileMenu->addAction(tr("&Save As..."));
    openAction->setShortcuts(QKeySequence::SaveAs);
    connect(writeAction, SIGNAL(triggered()), this, SLOT(writeFile()));

    QAction *quitAction = fileMenu->addAction(tr("E&xit"));
    quitAction->setShortcuts(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Help
    QMenu *helpMenu = new QMenu(tr("&Help"), this);

    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    //add to menub
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(helpMenu);
}


//*************************************************************************************************************

void MainWindow::setWindow() {
    //set Window functions
    resize(m_qSettings.value("MainWindow/size").toSize());
    this->move(50,50);
}


//*************************************************************************************************************

void MainWindow::setWindowStatus() {
    QString title;

    //request status
    if(m_pRawModel->m_bFileloaded) {
        int idx = m_qFileRaw.fileName().lastIndexOf("/");
        QString filename = m_qFileRaw.fileName().remove(0,idx+1);
        title = QString("%1, (File loaded: %2)").arg(CInfo::AppNameShort()).arg(filename);
    }
    else {
        title = QString("%1, (No File Loaded)").arg(CInfo::AppNameShort());
    }

    //set title
    setWindowTitle(title);

    //let the view update (ScrollBars etc.)
    m_pTableView->resizeColumnsToContents();
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


//*************************************************************************************************************

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
        //scroll down to the latest entry
        QTextCursor c = m_pTextBrowser_Log->textCursor();
        c.movePosition(QTextCursor::End);
        m_pTextBrowser_Log->setTextCursor(c);

        m_pTextBrowser_Log->verticalScrollBar()->setValue(m_pTextBrowser_Log->verticalScrollBar()->maximum());
    }
}


//*************************************************************************************************************

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
// SLOTS
void MainWindow::openFile()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));
    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();
    m_qFileRaw.setFileName(filename);

    if(m_pRawModel->loadFiffData(m_qFileRaw)) {
        qDebug() << "Fiff data file" << filename << "loaded.";
    }
    else
        qDebug("ERROR loading fiff data file %s",filename.toLatin1().data());

    setWindowStatus();

    //set position of QScrollArea
    setScrollBarPosition(0);
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    QString filename = QFileDialog::getSaveFileName(this,QString("Write fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));
    QFile t_fileRaw(filename);

    if(!m_pRawModel->writeFiffData(t_fileRaw))
        qDebug() << "MainWindow: ERROR writing fiff data file" << t_fileRaw.fileName() << "!";
}


//*************************************************************************************************************

void MainWindow::customContextMenuRequested(QPoint pos)
{
    //obtain index where index was clicked
    QModelIndex index = m_pTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = m_pTableView->selectionModel()->selectedIndexes();

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Marking ****************
    QMenu *markingSubMenu = new QMenu("Mark channels",menu);

    QAction* doMarkChBad = markingSubMenu->addAction(tr("Mark as bad"));
    connect(doMarkChBad,&QAction::triggered, [=](){
        m_pRawModel->markChBad(selected,1);
    });

    QAction* doMarkChGood = markingSubMenu->addAction(tr("Mark as good"));
    connect(doMarkChGood,&QAction::triggered, [=](){
        m_pRawModel->markChBad(selected,0);
    });

    //**************** FilterOperators ****************
    //selected channels
    QMenu *filtOpSubMenu = new QMenu("Apply FilterOperator to selected channel",menu);
    QMutableMapIterator<QString,QSharedPointer<MNEOperator> > it(m_pRawModel->m_Operators);
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pRawModel->applyOperator(selected,it.value());
        });
    }

    //all channels
    QMenu *filtOpAllSubMenu = new QMenu("Apply FilterOperator to all channels",menu);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        QAction* doApplyFilter = filtOpAllSubMenu->addAction(tr("%1").arg(it.key()));

        connect(doApplyFilter,&QAction::triggered, [=](){
            m_pRawModel->applyOperator(QModelIndexList(),it.value());
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
            m_pRawModel->undoFilter(selected,it.value());
        });
    }

    undoFiltOpSubMenu->addMenu(undoFiltOpSelSubMenu);

    //undo all filterting to selected channels
    QAction* undoApplyFilterSel = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to selected channels"));
    connect(undoApplyFilterSel,&QAction::triggered, [=](){
        m_pRawModel->undoFilter(selected);
    });

    //undo all filtering to all channels
    QAction* undoApplyFilterAll = undoFiltOpSubMenu->addAction(tr("Undo FilterOperators to all channels"));
    connect(undoApplyFilterAll,&QAction::triggered, [=](){
        m_pRawModel->undoFilter();
    });

    //add everything to main contextmenu
    menu->addMenu(markingSubMenu);
    menu->addMenu(filtOpSubMenu);
    menu->addMenu(filtOpAllSubMenu);
    menu->addMenu(undoFiltOpSubMenu);

    //show context menu
    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void MainWindow::setScrollBarPosition(int pos)
{
    m_pTableView->horizontalScrollBar()->setValue(pos);
    qDebug() << "MainWindow: m_iAbsFiffCursor position set to" << (m_pRawModel->firstSample()+pos);
}


//*************************************************************************************************************

void MainWindow::about()
{
    QMessageBox::about(this, CInfo::AppNameShort()+ ", "+tr("Version ")+CInfo::AppVersion(),
          tr("Copyright (C) 2014 Florian Schlembach, Christoph Dinh, Matti Hamalainen, Jens Haueisen. All rights reserved.\n\n"
             "Redistribution and use in source and binary forms, with or without modification, are permitted provided that"
             " the following conditions are met:\n"
             "\t* Redistributions of source code must retain the above copyright notice, this list of conditions and the"
             " following disclaimer.\n"
             "\t* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and"
             " the following disclaimer in the documentation and/or other materials provided with the distribution.\n"
             "\t* Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used"
             " to endorse or promote products derived from this software without specific prior written permission.\n\n"
             "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED"
             " WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A"
             " PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,"
             " INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,"
             " PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)"
             " HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING"
             " NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE"
             " POSSIBILITY OF SUCH DAMAGE."));
}
