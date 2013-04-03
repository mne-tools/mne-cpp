//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the MainWindow class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne_x/Management/modulemanager.h>
#include <mne_x/Management/connector.h>
#include <rtDtMng/rtmeasurementmanager.h>

#include <rtMeas/Measurement/realtimesamplearray.h>
#include <rtMeas/Measurement/numeric.h>
#include <rtMeas/Measurement/progressbar.h>
//#include "../../../comp/rtmeas/Measurement/alert.h" //-> text.h
#include <rtMeas/Measurement/text.h>

#include <generics/observerpattern.h>

#include <disp/displaymanager.h>
#include <disp/realtimesamplearraywidget.h>
#include <disp/numericwidget.h>
#include <disp/progressbarwidget.h>
#include <disp/textwidget.h>

#include <mne_x/Interfaces/IModule.h>
#include <mne_x/Interfaces/ISensor.h>
#include <mne_x/Interfaces/IRTAlgorithm.h>
#include <mne_x/Interfaces/IRTVisualization.h>
#include <mne_x/Interfaces/IRTRecord.h>
#include <mne_x/Interfaces/IAlert.h>

//GUI
#include "mainwindow.h"
#include "moduledockwidget.h"
#include "runwidget.h"
#include "startupwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMultiHash>

#include <QtWidgets>

#include <QDebug>

#include <QTimer>
#include <QTime>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// CONST
//=============================================================================================================

const char* moduleDir = "/mne_x_plugins";        /**< holds path to plugins.*/


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_pStartUpWidget(new StartUpWidget())
, m_pRunWidget(0)
, m_bDisplayMax(false)
, m_bIsRunning(false)
, m_pTimer(0)
, m_pTime(0)
, m_iTimeoutMSec(1000)
, m_eLogLevelCurrent(_LogLvMax)
{
    qDebug() << "Clinical Sensing and Analysis - Version" << CInfo::AppVersion();

    setCentralWidget(m_pStartUpWidget);

    setWindowTitle(CInfo::AppNameShort());
    setMinimumSize(400, 400);
    resize(1280, 800);

    setUnifiedTitleAndToolBarOnMac(true);

    m_pModuleManager = new ModuleManager();
    m_pModuleManager->loadModules(qApp->applicationDirPath()+moduleDir);

    createActions();
    createMenus();
    createToolBars();
    createModuleDockWindow();
    createLogDockWindow();

    connect(this, SIGNAL(newLogMsg(const QString&, LogKind, LogLevel)), this, SLOT(writeToLog(const QString&, LogKind, LogLevel)));

    //ToDo Debug Startup
    emit newLogMsg(tr("Test normal message, Max"), _LogKndMessage, _LogLvMax);
    emit newLogMsg(tr("Test warning message, Normal"), _LogKndWarning, _LogLvNormal);
    emit newLogMsg(tr("Test error message, Min"), _LogKndError, _LogLvMin);

    Connector::init();

    initStatusBar();
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
    //ToDo cleanup work
}


//*************************************************************************************************************
//File QMenu
void MainWindow::newConfiguration()
{
    emit newLogMsg(tr("Invoked <b>File|NewPreferences</b>"), _LogKndMessage, _LogLvMin);
}


//*************************************************************************************************************

void MainWindow::openConfiguration()
{
    emit newLogMsg(tr("Invoked <b>File|OpenPreferences</b>"), _LogKndMessage, _LogLvMin);
}


//*************************************************************************************************************

void MainWindow::saveConfiguration()
{
    emit newLogMsg(tr("Invoked <b>File|SavePreferences</b>"), _LogKndMessage, _LogLvMin);
}


//*************************************************************************************************************
//Help QMenu
void MainWindow::helpContents()
{
    emit newLogMsg(tr("Invoked <b>Help|HelpContents</b>"), _LogKndMessage, _LogLvMin);
}


//*************************************************************************************************************

void MainWindow::about()
{
    emit newLogMsg(tr("Invoked <b>Help|About</b>"), _LogKndMessage, _LogLvMin);
    QMessageBox::about(this, CInfo::AppNameShort()+ ", "+tr("Version ")+CInfo::AppVersion(),
         tr("Copyright (C) 2013 Christoph Dinh, Martin Luessi, Jens Haueisen, Matti Hamalainen. All rights reserved.\n\n"
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


//*************************************************************************************************************

void MainWindow::setMinLogLevel()
{
    emit newLogMsg(tr("minimal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvMin;
}


//*************************************************************************************************************

void MainWindow::setNormalLogLevel()
{
    emit newLogMsg(tr("normal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvNormal;
}


//*************************************************************************************************************

void MainWindow::setMaxLogLevel()
{
    emit newLogMsg(tr("maximal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvMax;
}

//*************************************************************************************************************

void MainWindow::createActions()
{
    //File QMenu
    m_pActionNewConfig = new QAction(QIcon(":/images/new.png"), tr("&New configuration"), this);
    m_pActionNewConfig->setShortcuts(QKeySequence::New);
    m_pActionNewConfig->setStatusTip(tr("Create a new configuration"));
    connect(m_pActionNewConfig, SIGNAL(triggered()), this, SLOT(newPreferences()));

    m_pActionOpenConfig = new QAction(tr("&Open configuration..."), this);
    m_pActionOpenConfig->setShortcuts(QKeySequence::Open);
    m_pActionOpenConfig->setStatusTip(tr("Open an existing configuration"));
    connect(m_pActionOpenConfig, SIGNAL(triggered()), this, SLOT(openPreferences()));

    m_pActionSaveConfig = new QAction(QIcon(":/images/save.png"), tr("&Save configuration..."), this);
    m_pActionSaveConfig->setShortcuts(QKeySequence::Save);
    m_pActionSaveConfig->setStatusTip(tr("Save the current configuration"));
    connect(m_pActionSaveConfig, SIGNAL(triggered()), this, SLOT(savePreferences()));

    m_pActionExit = new QAction(tr("E&xit"), this);
    m_pActionExit->setShortcuts(QKeySequence::Quit);
    m_pActionExit->setStatusTip(tr("Exit the application"));
    connect(m_pActionExit, SIGNAL(triggered()), this, SLOT(close()));


    //View QMenu
    m_pActionMinLgLv = new QAction(tr("&Minimal"), this);
    m_pActionMinLgLv->setCheckable(true);
    m_pActionMinLgLv->setShortcut(tr("Ctrl+1"));
    m_pActionMinLgLv->setStatusTip(tr("Open an existing file"));
    connect(m_pActionMinLgLv, SIGNAL(triggered()), this, SLOT(setMinLogLevel()));

    m_pActionNormLgLv = new QAction(tr("&Normal"), this);
    m_pActionNormLgLv->setCheckable(true);
    m_pActionNormLgLv->setShortcut(tr("Ctrl+2"));
    m_pActionNormLgLv->setStatusTip(tr("Save the document to disk"));
    connect(m_pActionNormLgLv, SIGNAL(triggered()), this, SLOT(setNormalLogLevel()));

    m_pActionMaxLgLv = new QAction(tr("Maximal"), this);
    m_pActionMaxLgLv->setCheckable(true);
    m_pActionMaxLgLv->setShortcut(tr("Ctrl+3"));
    m_pActionMaxLgLv->setStatusTip(tr("Exit the application"));
    connect(m_pActionMaxLgLv, SIGNAL(triggered()), this, SLOT(setMaxLogLevel()));

    m_pActionGroupLgLv = new QActionGroup(this);
    m_pActionGroupLgLv->addAction(m_pActionMinLgLv);
    m_pActionGroupLgLv->addAction(m_pActionNormLgLv);
    m_pActionGroupLgLv->addAction(m_pActionMaxLgLv);
    if (m_eLogLevelCurrent == _LogLvMin){
        m_pActionMinLgLv->setChecked(true);}
    else if (m_eLogLevelCurrent == _LogLvNormal){
        m_pActionNormLgLv->setChecked(true);}
    else {
        m_pActionMaxLgLv->setChecked(true);}

    //Help QMenu
    m_pActionHelpContents = new QAction(tr("Help &Contents"), this);
    m_pActionHelpContents->setShortcuts(QKeySequence::HelpContents);
    m_pActionHelpContents->setStatusTip(tr("Show the help contents"));
    connect(m_pActionHelpContents, SIGNAL(triggered()), this, SLOT(helpContents()));

    m_pActionAbout = new QAction(tr("&About"), this);
    m_pActionAbout->setStatusTip(tr("Show the application's About box"));
    connect(m_pActionAbout, SIGNAL(triggered()), this, SLOT(about()));

    //Debug QMenu ToDo
    m_pActionDebugDisconnect = new QAction(tr("Disconnect Widgets"), this);
//ToDo    connect(m_pActionDebugDisconnect, SIGNAL(triggered()), this, SLOT(Connector::disconnectMeasurementWidgets()));// Function is not anmyore valid

    //QToolbar

    m_pActionRun = new QAction(QIcon(":/images/run.png"), tr("Run (F5)"), this);
    m_pActionRun->setShortcut(tr("F5"));
    m_pActionRun->setStatusTip(tr("Runs (F5) ")+CInfo::AppNameShort());
    connect(m_pActionRun, SIGNAL(triggered()), this, SLOT(startRTMeasurement()));

    m_pActionStop = new QAction(QIcon(":/images/stop.png"), tr("Stop (F6)"), this);
    m_pActionStop->setShortcut(tr("F6"));
    m_pActionStop->setStatusTip(tr("Stops (F6) ")+CInfo::AppNameShort());
    connect(m_pActionStop, SIGNAL(triggered()), this, SLOT(stopRTMeasurement()));

    m_pActionZoomStd = new QAction(QIcon(":/images/zoomStd.png"), tr("Standard Zoom (Ctrl+0)"), this);
    m_pActionZoomStd->setShortcut(tr("Ctrl+0"));
    m_pActionZoomStd->setStatusTip(tr("Sets the standard Zoom (Ctrl+0)"));
    connect(m_pActionZoomStd, SIGNAL(triggered()), this, SLOT(zoomStd()));

    m_pActionZoomIn = new QAction(QIcon(":/images/zoomIn.png"), tr("Zoom In ")+QKeySequence(QKeySequence::ZoomIn).toString(), this);
    m_pActionZoomIn->setShortcuts(QKeySequence::ZoomIn);
    m_pActionZoomIn->setStatusTip(tr("Zooms in the magnitude ")+QKeySequence(QKeySequence::ZoomIn).toString());
    connect(m_pActionZoomIn, SIGNAL(triggered()), this, SLOT(zoomIn()));

    m_pActionZoomOut = new QAction(QIcon(":/images/zoomOut.png"), tr("Zoom Out ")+QKeySequence(QKeySequence::ZoomOut).toString(), this);
    m_pActionZoomOut->setShortcuts(QKeySequence::ZoomOut);
    m_pActionZoomOut->setStatusTip(tr("Zooms out the magnitude ")+QKeySequence(QKeySequence::ZoomOut).toString());
    connect(m_pActionZoomOut, SIGNAL(triggered()), this, SLOT(zoomOut()));

    m_pActionDisplayMax = new QAction(QIcon(":/images/displayMax.png"), tr("Maximize current Display (F11)"), this);
    m_pActionDisplayMax->setShortcut(tr("F11"));
    m_pActionDisplayMax->setStatusTip(tr("Maximizes the current Display (F11)"));
    connect(m_pActionDisplayMax, SIGNAL(triggered()), this, SLOT(toggleDisplayMax()));
}


//*************************************************************************************************************

void MainWindow::createMenus()
{
    m_pMenuFile = menuBar()->addMenu(tr("&File"));
    m_pMenuFile->addAction(m_pActionNewConfig);
    m_pMenuFile->addAction(m_pActionOpenConfig);
    m_pMenuFile->addAction(m_pActionSaveConfig);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(m_pActionExit);

    m_pMenuView = menuBar()->addMenu(tr("&View"));
    m_pMenuLgLv = m_pMenuView->addMenu(tr("&Log Level"));
    m_pMenuLgLv->addAction(m_pActionMinLgLv);
    m_pMenuLgLv->addAction(m_pActionNormLgLv);
    m_pMenuLgLv->addAction(m_pActionMaxLgLv);
    m_pMenuView->addSeparator();

    menuBar()->addSeparator();

    m_pMenuHelp = menuBar()->addMenu(tr("&Help"));
    m_pMenuHelp->addAction(m_pActionHelpContents);
    m_pMenuHelp->addSeparator();
    m_pMenuHelp->addAction(m_pActionAbout);

    menuBar()->addSeparator();

    m_pMenuDebug = menuBar()->addMenu(tr("&Debug"));
    m_pMenuDebug->addAction(m_pActionDebugDisconnect);
}


//*************************************************************************************************************

void MainWindow::createToolBars()
{
    m_pToolBar = addToolBar(tr("File"));
    m_pToolBar->addAction(m_pActionRun);
    m_pToolBar->addAction(m_pActionStop);
    m_pActionStop->setEnabled(false);

    m_pToolBar->addSeparator();

    m_pToolBar->addAction(m_pActionZoomStd);
    m_pToolBar->addAction(m_pActionZoomIn);
    m_pToolBar->addAction(m_pActionZoomOut);
    m_pToolBar->addAction(m_pActionDisplayMax);
    m_pActionZoomStd->setEnabled(false);
    m_pActionZoomIn->setEnabled(false);
    m_pActionZoomOut->setEnabled(false);
    m_pActionDisplayMax->setEnabled(false);

    m_pToolBar->addSeparator();

    m_pLabel_Time = new QLabel;
    m_pToolBar->addWidget(m_pLabel_Time);
    m_pLabel_Time->setText(QTime(0, 0).toString());
}


//*************************************************************************************************************

void MainWindow::initStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}


//*************************************************************************************************************

void MainWindow::createModuleDockWindow()
{

    m_pModuleDockWidget = new ModuleDockWidget(tr("Modules"), this);
    m_pModuleDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    addDockWidget(Qt::LeftDockWidgetArea, m_pModuleDockWidget);

    m_pMenuView->addAction(m_pModuleDockWidget->toggleViewAction());

    connect(m_pModuleDockWidget, SIGNAL(moduleChanged(int, const QTreeWidgetItem*)),
            this, SLOT(CentralWidgetShowModule()));
}


//*************************************************************************************************************

void MainWindow::createLogDockWindow()
{

    //Log TextBrowser
    m_pDockWidget_Log = new QDockWidget(tr("Log"), this);

    m_pTextBrowser_Log = new QTextBrowser(m_pDockWidget_Log);

    m_pDockWidget_Log->setWidget(m_pTextBrowser_Log);

    m_pDockWidget_Log->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_pDockWidget_Log);

    //dock->setVisible(false);

    m_pMenuView->addAction(m_pDockWidget_Log->toggleViewAction());

}


//*************************************************************************************************************
//Module stuff
void MainWindow::CentralWidgetShowModule()//int iCurrentModuleNum, const QTreeWidgetItem* pCurrentItem)
{
    int iCurrentModuleNum = m_pModuleDockWidget->m_iCurrentModuleIdx;//m_pModulesDockWidget->getCurrentModuleNum()

     // ToDo for root menu options
    //const QTreeWidgetItem* pCurrentItem = m_pModuleDockWidget->m_pCurrentItem; //m_pModulesDockWidget->getCurrentItem()

    qDebug() << "MainCSART::CentralWidgetShowModule(): current module number " << iCurrentModuleNum;

    Subject::notifyEnabled = false; //like Mutex.lock -> for now dirty hack - it's okay when values are queued -> than it"s like a Mutex

    Connector::disconnectMeasurementWidgets(m_pListCurrentDisplayModules);//Make sure to only disconnect widgets
    m_pListCurrentDisplayModules.clear();

    QWidget* defaultWidget = new QWidget;
    setCentralWidget(defaultWidget);

    if(m_pModuleDockWidget->isActivated(iCurrentModuleNum))
    {
        if(!m_bIsRunning)
        {
            setCentralWidget(ModuleManager::s_vecModules[iCurrentModuleNum]->setupWidget()); //QMainWindow takes ownership of the widget pointer and deletes it at the appropriate time.
        }
        else if(m_bIsRunning)
        {

            m_pListCurrentDisplayModules << ModuleManager::getModules()[iCurrentModuleNum]->getModule_ID();

            Connector::connectMeasurementWidgets(m_pListCurrentDisplayModules, m_pTime);

            m_pRunWidget = new RunWidget(DisplayManager::show());
            m_pRunWidget->addTab(ModuleManager::s_vecModules[iCurrentModuleNum]->runWidget(), tr("Confi&guration"));

            if(m_bDisplayMax)//ToDo send events to main window
            {
                m_pRunWidget->showFullScreen();
                connect(m_pRunWidget, SIGNAL(displayClosed()), this, SLOT(toggleDisplayMax()));
            }
            else
                setCentralWidget(m_pRunWidget);
        }
    }

    Subject::notifyEnabled = true; //like Mutex.unlock
}


//*************************************************************************************************************

void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    if(lglvl<=m_eLogLevelCurrent)
    {
        if(lgknd == _LogKndError)
        {
            m_pTextBrowser_Log->insertHtml("<font color=red><b>Error:</b> "+logMsg+"</font>");

        }
        else if(lgknd == _LogKndWarning)
        {
            m_pTextBrowser_Log->insertHtml("<font color=blue><b>Warning:</b> "+logMsg+"</font>");
        }
        else
        {
            m_pTextBrowser_Log->insertHtml(logMsg);
        }
        m_pTextBrowser_Log->insertPlainText("\n"); // new line
        //scroll down to the newest entry
        QTextCursor c = m_pTextBrowser_Log->textCursor();
        c.movePosition(QTextCursor::End);
        m_pTextBrowser_Log->setTextCursor(c);
    }
}


//*************************************************************************************************************

void MainWindow::startRTMeasurement()
{
    emit newLogMsg(tr("Starting real-time measurement..."), _LogKndMessage, _LogLvMin);

    qDebug() << "MainCSART::startRTMeasurement()";

    //RTMeasurementManager::clean();
    //DisplayManager::clean();

    if(!ModuleManager::startModules())
    {
        QMessageBox::information(0, QObject::tr("CSA RT - Start modules"), QString(QObject::tr("No Sensor module is active")), QMessageBox::Ok);
        return;
    }


    uiSetupRunningState(true);
    startTimer(m_iTimeoutMSec);
    CentralWidgetShowModule();
}


//*************************************************************************************************************

void MainWindow::stopRTMeasurement()
{
    emit newLogMsg(tr("Stopping real-time measurement..."), _LogKndMessage, _LogLvMin);

    qDebug() << "MainWindow::stopRTMeasurement()";


    ModuleManager::stopModules();

    Connector::disconnectMeasurementWidgets(m_pListCurrentDisplayModules);//was before stopModules();

    qDebug() << "set stopped UI";

    uiSetupRunningState(false);
    stopTimer();
    CentralWidgetShowModule();
}


//*************************************************************************************************************

void MainWindow::zoomStd()
{
    if(m_pRunWidget)
    {
        m_pRunWidget->setStandardZoom();
        m_pActionZoomStd->setEnabled(false);
    }
}


//*************************************************************************************************************

void MainWindow::zoomIn()
{
    if(m_pRunWidget)
    {
        m_pRunWidget->zoomVert(2);
        m_pActionZoomStd->setEnabled(true);
    }
}


//*************************************************************************************************************

void MainWindow::zoomOut()
{
    if(m_pRunWidget)
    {
        m_pRunWidget->zoomVert(0.5);
        m_pActionZoomStd->setEnabled(true);
    }

}


//*************************************************************************************************************

void MainWindow::toggleDisplayMax()
{

    m_bDisplayMax = !m_bDisplayMax;

    m_pActionDisplayMax->setEnabled(!m_bDisplayMax);

    CentralWidgetShowModule();
}


//*************************************************************************************************************

void MainWindow::uiSetupRunningState(bool state)
{
    m_pActionRun->setEnabled(!state);
    m_pActionStop->setEnabled(state);

    if(state)
    {
        m_pActionZoomStd->setEnabled(!state);
        m_pActionZoomIn->setEnabled(state);
        m_pActionZoomOut->setEnabled(state);
        m_pActionDisplayMax->setEnabled(state);
    }
    else
    {
        m_pActionZoomStd->setEnabled(state);
        m_pActionZoomIn->setEnabled(state);
        m_pActionZoomOut->setEnabled(state);
        m_pActionDisplayMax->setEnabled(state);
    }

    m_bIsRunning = state;
}



//*************************************************************************************************************

void MainWindow::startTimer(int msec)
{
    if(m_pTimer)
        delete m_pTimer;
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateTime()));
    m_pTimer->start(msec);
    if(m_pTime)
        delete m_pTime;
    m_pTime = new QTime(0, 0);
    QString strTime = m_pTime->toString();
    m_pLabel_Time->setText(strTime);
}


//*************************************************************************************************************

void MainWindow::stopTimer()
{
    disconnect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateTime()));

    delete m_pTimer;
    m_pTimer = 0;

    delete m_pTime;
    m_pTime = 0;
}


//*************************************************************************************************************

void MainWindow::updateTime()
{
    *m_pTime = m_pTime->addMSecs(m_iTimeoutMSec);
    QString strTime = m_pTime->toString();
    m_pLabel_Time->setText(strTime);
}
