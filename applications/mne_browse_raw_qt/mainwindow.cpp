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
, m_qFileEvent("./MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif")
, m_qSettings()
, m_rawSettings()
{
    //Setup the windows first - this NEEDS to be done here because important pointers (window pointers) which are needed for further processing are generated in this function
    setupMainWindow();
    setupWindowWidgets();

    //setup MVC
    setupModel();
    setupDelegate();
    setupViews();

    //Set event data and view in the delegate. This needs to be done here after all the above called setup routines
    m_pRawDelegate->setEventModelView(m_pEventModel, m_pEventTableView, m_pRawTableView);

    // Setup rest of the GUI
    createMenus();
    createLogDockWindow();
    setWindowStatus();
    m_pDataWindow->setWindowStatus();
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
    m_pEventModel = new EventModel(this);

    //Set fiffInfo in event model TODO: This is dirty - what happens if no file was loaded (due to missing file)
    m_pEventModel->setFiffInfo(m_pRawModel->m_fiffInfo);
    m_pEventModel->setFirstSample(m_pRawModel->firstSample());
}


//*************************************************************************************************************

void MainWindow::setupDelegate()
{
    m_pRawDelegate = new RawDelegate(this);
}


//*************************************************************************************************************

void MainWindow::setupViews()
{
    m_pRawTableView = m_pDataWindow->getTableView();
    m_pEventTableView = m_pEventWindow->getTableView();

    //set custom models
    m_pRawTableView->setModel(m_pRawModel);
    m_pEventTableView->setModel(m_pEventModel);

    //set custom delegate
    m_pRawTableView->setItemDelegate(m_pRawDelegate);

    //setup view settings
    m_pDataWindow->setupRawViewSettings();
    m_pEventWindow->setupEventViewSettings();
}


//*************************************************************************************************************

void MainWindow::setupWindowWidgets()
{
    //Create dockable data window - QTDesigner used - see /FormFiles
    m_pDataWindow = new DataWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pDataWindow);

    //Create filter window - QTDesigner used - see /FormFiles
    m_pFilterWindow = new FilterWindow(this);
    m_pFilterWindow->hide();

    //Create dockble event window - QTDesigner used - see /FormFiles
    m_pEventWindow = new EventWindow(this);
    addDockWidget(Qt::RightDockWidgetArea, m_pEventWindow);
    m_pEventWindow->hide();
}


//*************************************************************************************************************

void MainWindow::createMenus()
{
    //File
    QMenu *fileMenu = new QMenu(tr("&File"), this);

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcuts(QKeySequence::Open);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openFile()));

    QAction *writeAction = fileMenu->addAction(tr("&Save..."));
    openAction->setShortcuts(QKeySequence::SaveAs);
    connect(writeAction, SIGNAL(triggered()), this, SLOT(writeFile()));

    fileMenu->addSeparator();

    QAction *loadEvents = fileMenu->addAction(tr("&Load Events (fif)..."));
    connect(loadEvents, SIGNAL(triggered()), this, SLOT(loadEvents()));

    QAction *saveEvents = fileMenu->addAction(tr("&Save Events (fif)..."));
    saveEvents->setDisabled(true);
    connect(saveEvents, SIGNAL(triggered()), this, SLOT(saveEvents()));

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcuts(QKeySequence::Quit);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Adjust
    QMenu *adjustMenu = new QMenu(tr("&Adjust"), this);

    QAction *adjustAction = adjustMenu->addAction(tr("&Filter..."));
    connect(adjustAction, SIGNAL(triggered()), this, SLOT(showFilterWindow()));

    //Windows
    QMenu *windowsMenu = new QMenu(tr("&Windows"), this);

    QAction *dataAction = windowsMenu->addAction(tr("&Show data plot..."));
    connect(dataAction, SIGNAL(triggered()), this, SLOT(showDataWindow()));

    QAction *eventAction = windowsMenu->addAction(tr("&Show event list..."));
    connect(eventAction, SIGNAL(triggered()), this, SLOT(showEventWindow()));

    QAction *logAction = windowsMenu->addAction(tr("&Show log..."));
    connect(logAction, SIGNAL(triggered()), this, SLOT(showLogWindow()));

    //Help
    QMenu *helpMenu = new QMenu(tr("&Help"), this);

    QAction *aboutAction = helpMenu->addAction(tr("&About..."));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

    //add to menub
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(adjustMenu);
    menuBar()->addMenu(windowsMenu);
    menuBar()->addMenu(helpMenu);
}


//*************************************************************************************************************

void MainWindow::setupMainWindow()
{
    //set Window functions
    resize(m_qSettings.value("MainWindow/size").toSize()); //Resize to predefined default size
    move(50,50); // Move this main window to position 50/50 on the screen

    //Set central widget - This is needed because we are using QDockWidgets
    QWidget *window = new QWidget();
    setCentralWidget(window);
    centralWidget()->setFixedWidth(1);
}


//*************************************************************************************************************

void MainWindow::setWindowStatus()
{
    QString title;

    //request status
    title = QString("%1").arg(CInfo::AppNameShort());

    //set title
    setWindowTitle(title);

    //let the view update (ScrollBars etc.)
    m_pRawTableView->resizeColumnsToContents();
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

    if(filename.isEmpty())
    {
        qDebug("User aborted opening of fiff data file");
        return;
    }

    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();
    m_qFileRaw.setFileName(filename);

    if(m_pRawModel->loadFiffData(m_qFileRaw)) {
        qDebug() << "Fiff data file" << filename << "loaded.";
    }
    else
        qDebug("ERROR loading fiff data file %s",filename.toLatin1().data());

    m_pDataWindow->setWindowStatus();

    //set position of QScrollArea
    setScrollBarPosition(0);

    //Set fiffInfo in event model
    m_pEventModel->setFiffInfo(m_pRawModel->m_fiffInfo);
    m_pEventModel->setFirstSample(m_pRawModel->firstSample());
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    QString filename = QFileDialog::getSaveFileName(this,QString("Write fiff data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif data files (*.fif)"));

    if(filename.isEmpty())
    {
        qDebug("User aborted saving to fiff data file");
        return;
    }

    QFile t_fileRaw(filename);

    if(!m_pRawModel->writeFiffData(t_fileRaw))
        qDebug() << "MainWindow: ERROR writing fiff data file" << t_fileRaw.fileName() << "!";
}


//*************************************************************************************************************

void MainWindow::loadEvents()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open fiff event data file"),QString("./MNE-sample-data/MEG/sample/"),tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(m_qFileEvent.isOpen())
        m_qFileEvent.close();
    m_qFileEvent.setFileName(filename);

    if(m_pEventModel->loadEventData(m_qFileEvent)) {
        qDebug() << "Fiff event data file" << filename << "loaded.";
    }
    else
        qDebug("ERROR loading fiff event data file %s",filename.toLatin1().data());

    m_pDataWindow->setWindowStatus();

    //Show event widget
    showEventWindow();
}


//*************************************************************************************************************

void MainWindow::saveEvents()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save fiff event data file"),
                                                    QString("./MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(filename.isEmpty())
    {
        qDebug("User aborted saving to fiff event data file");
        return;
    }

    if(m_qFileEvent.isOpen())
        m_qFileEvent.close();
    m_qFileEvent.setFileName(filename);

    if(m_pEventModel->saveEventData(m_qFileEvent)) {
        qDebug() << "Fiff event data file" << filename << "saved.";
    }
    else
        qDebug("ERROR saving fiff event data file %s",filename.toLatin1().data());

    m_pDataWindow->setWindowStatus();
}


//*************************************************************************************************************

void MainWindow::setScrollBarPosition(int pos)
{
    m_pRawTableView->horizontalScrollBar()->setValue(pos);
    qDebug() << "MainWindow: m_iAbsFiffCursor position set to" << (m_pRawModel->firstSample()+pos);
}


//*************************************************************************************************************

void MainWindow::about()
{
    QMessageBox::about(this, CInfo::AppNameShort()+ ", "+tr("Version ")+CInfo::AppVersion(),
          tr("Copyright (C) 2014 Florian Schlembach, Lorenz Esch, Christoph Dinh, Matti Hamalainen, Jens Haueisen. All rights reserved.\n\n"
             "Redistribution and use in source and binary forms, with or without modification, are permitted provided that"
             " the following conditions are met:\n"
             "\t* Redistributions of source code must retain the above copyright notice, this list of conditions and the"
             " following disclaimer.\n"
             "\t* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and"
             " the following disclaimer in the documentation and/or other materials provided with the distribution.\n"
             "\t* Neither the name of MNE-CPP authors nor the names of its contributors may be used"
             " to endorse or promote products derived from this software without specific prior written permission.\n\n"
             "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED"
             " WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A"
             " PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,"
             " INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,"
             " PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)"
             " HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING"
             " NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE"
             " POSSIBILITY OF SUCH DAMAGE."));
}


//*************************************************************************************************************

void MainWindow::showFilterWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pFilterWindow->isVisible())
    {
        m_pFilterWindow->show();
        m_pFilterWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pFilterWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showEventWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pEventWindow->isVisible())
    {
        m_pEventWindow->show();
        m_pEventWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pEventWindow->raise();
}


//*************************************************************************************************************

void MainWindow::showLogWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pDockWidget_Log->isVisible())
    {
        m_pDockWidget_Log->show();
        m_pDockWidget_Log->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pDockWidget_Log->raise();
}


//*************************************************************************************************************

void MainWindow::showDataWindow()
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!m_pDataWindow->isVisible())
    {
        m_pDataWindow->show();
        m_pDataWindow->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        m_pDataWindow->raise();
}



