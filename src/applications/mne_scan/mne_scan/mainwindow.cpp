//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the MainWindow class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include <scShared/Management/pluginmanager.h>
#include <scShared/Management/pluginscenemanager.h>
#include <scShared/Management/displaymanager.h>

#include <scShared/Plugins/abstractplugin.h>

#include <scDisp/measurementwidget.h>
#include <scDisp/realtimemultisamplearraywidget.h>
#include <scDisp/realtimeevokedsetwidget.h>

#include <disp/viewers/multiview.h>
#include <disp/viewers/multiviewwindow.h>

#include <disp/viewers/quickcontrolview.h>

#include <utils/buildinfo.h>

#include "mainwindow.h"
#include "startupwidget.h"
#include "plugingui.h"
#include "info.h"
#include "mainsplashscreencloser.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QDebug>
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;
using namespace SCSHAREDLIB;
using namespace SCDISPLIB;
using namespace DISPLIB;

//=============================================================================================================
// CONST
//=============================================================================================================

const QString pluginDir = "/mne_scan_plugins";          /**< holds path to plugins.*/
constexpr unsigned long waitUntilHidingSplashScreen(1);     /**< Seconds to wait after the application setup has finished, before hiding the splash screen.*/

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_bIsRunning(false)
, m_iTimeoutMSec(1000)
, m_pStartUpWidget(new StartUpWidget(this))
, m_eLogLevelCurrent(_LogLvMax)
, m_pTime(new QTime(0, 0))
, m_pPluginManager(new SCSHAREDLIB::PluginManager(this))
, m_pPluginSceneManager(new SCSHAREDLIB::PluginSceneManager(this))
, m_pDisplayManager(new SCSHAREDLIB::DisplayManager(this))
, m_sSettingsPath("MNESCAN/MainWindow")
, m_sCurrentStyle("default")
{
    printf( "%s - Version %s\n",
            CInfo::AppNameShort().toUtf8().constData(),
            CInfo::AppVersion().toUtf8().constData());

    setCentralWidget(m_pStartUpWidget);

    setWindowTitle(CInfo::AppNameShort());
    setMinimumSize(400, 400);
    resize(1280, 800);

    setUnifiedTitleAndToolBarOnMac(false);

    initSplashScreen();

    setupPlugins();
    setupUI();

    loadSettings();

    //Load application icon for linux builds only, mac and win executables have built in icons from .pro file
#ifdef __linux__
    qInfo() << "Loading icon...";
    QMainWindow::setWindowIcon(QIcon(":/images/images/appIcons/icon_mne_scan_256x256.png"));
#endif

    hideSplashScreen();
    show();
}

//=============================================================================================================

MainWindow::~MainWindow()
{
    saveSettings();

    if(m_bIsRunning) {
        this->stopMeasurement();
    }

    if(m_pToolBar) {
        if(m_pLabelTime) {
            delete m_pLabelTime;
        }
        delete m_pToolBar;
    }

    delete m_pDynamicPluginToolBar;
}

//=============================================================================================================

void MainWindow::setupPlugins()
{
    m_pPluginManager->loadPlugins(qApp->applicationDirPath()+pluginDir);
}

//=============================================================================================================

void MainWindow::setupUI()
{
    // Quick control selection
    m_pQuickControlView = new QuickControlView(QString("MNESCAN/MainWindow/QuickControl"),
                                                       "MNE Scan",
                                                       Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint,
                                                       this);
    createActions();
    createMenus();
    createToolBars();
    createPluginDockWindow();
    createLogDockWindow();

    initStatusBar();
}

//=============================================================================================================

void MainWindow::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/styleMode"), m_sCurrentStyle);
}

//=============================================================================================================

void MainWindow::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

    m_sCurrentStyle = settings.value(m_sSettingsPath + QString("/styleMode"), "default").toString();
    if(m_sCurrentStyle == "dark") {
        m_pActionDarkMode->setChecked(true);
    } else {
        m_pActionDarkMode->setChecked(false);
    }

    onStyleChanged(m_sCurrentStyle);
}

//=============================================================================================================

void MainWindow::onStyleChanged(const QString& sStyle)
{
    qInfo() << "MainWindow::onStyleChanged";
    qInfo() << "MainWindow::onStyleChanged sStyle" << sStyle;
    if(QApplication *pApp = qobject_cast<QApplication *>(QApplication::instance())) {
        if(sStyle == "default") {
            qInfo() << "MainWindow::onStyleChanged 1";
            m_sCurrentStyle = "default";
            pApp->setStyleSheet("");
        } else if (sStyle == "dark") {
            qInfo() << "MainWindow::onStyleChanged 2";
            m_sCurrentStyle = "dark";
            QFile file(":/dark.qss");
            if(file.open(QFile::ReadOnly)){
                QTextStream stream(&file);
                pApp->setStyleSheet(stream.readAll());
            }
        }

        // Set default font
        int id = QFontDatabase::addApplicationFont(":/fonts/Roboto-Light.ttf");
        if(id != -1){
            pApp->setFont(QFont(QFontDatabase::applicationFontFamilies(id).at(0)));
        }
    }
}

//=============================================================================================================

void MainWindow::onGuiModeChanged()
{
    if(m_pActionResearchMode->isChecked()) {
        emit guiModeChanged(DISPLIB::AbstractView::GuiMode::Research);
    } else if(m_pActionClinicalMode->isChecked()) {
        emit guiModeChanged(DISPLIB::AbstractView::GuiMode::Clinical);
    }
}

//=============================================================================================================

void MainWindow::initSplashScreen()
{
    bool showSplashScreen(true);
    initSplashScreen(showSplashScreen);
}

//=============================================================================================================

void MainWindow::initSplashScreen(bool bShowSplashScreen)
{
    QPixmap splashPixMap(":/images/splashscreen.png");
    m_pSplashScreen = MainSplashScreen::SPtr::create(splashPixMap,
                                                    Qt::WindowFlags() | Qt::WindowStaysOnTopHint );
    if(m_pSplashScreen && m_pPluginManager) {
        QObject::connect(m_pPluginManager.data(), &PluginManager::pluginLoaded,
                         m_pSplashScreen.data(), &MainSplashScreen::showMessage);
    }

    if(m_pSplashScreen && bShowSplashScreen) {
        m_pSplashScreen->show();
    }
}

//=============================================================================================================

void MainWindow::hideSplashScreen()
{
    m_pSplashScreenHider = MainSplashScreenCloser::SPtr::create(*m_pSplashScreen.data(),
                                                       waitUntilHidingSplashScreen);
    m_pSplashScreen->clearMessage();
    m_pSplashScreenHider->start();
}

//=============================================================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopMeasurement();

    QMainWindow::closeEvent(event);
}

//=============================================================================================================

void MainWindow::newConfiguration()
{
    writeToLog(tr("Invoked <b>File|NewConfiguration</b>"), _LogKndMessage, _LogLvMin);
    m_pPluginGui->clearScene();
}

//=============================================================================================================

void MainWindow::openConfiguration()
{
    writeToLog(tr("Invoked <b>File|OpenConfiguration</b>"), _LogKndMessage, _LogLvMin);

    QString path = QFileDialog::getOpenFileName(this,
                                                "Open MNE Scan Configuration File",
                                                QStandardPaths::writableLocation(QStandardPaths::DataLocation),
                                                 tr("Configuration file (*.xml)"));

    QFileInfo qFileInfo(path);
    m_pPluginGui->loadConfig(qFileInfo.path(), qFileInfo.fileName());
}

//=============================================================================================================

void MainWindow::saveConfiguration()
{
    writeToLog(tr("Invoked <b>File|SaveConfiguration</b>"), _LogKndMessage, _LogLvMin);

    QString path = QFileDialog::getSaveFileName(
                this,
                "Save MNE Scan Configuration File",
                QStandardPaths::writableLocation(QStandardPaths::DataLocation),
                 tr("Configuration file (*.xml)"));

    QFileInfo qFileInfo(path);
    m_pPluginGui->saveConfig(qFileInfo.path(), qFileInfo.fileName());
}

//=============================================================================================================

void MainWindow::helpContents()
{
    writeToLog(tr("Invoked <b>Help|HelpContents</b>"), _LogKndMessage, _LogLvMin);
}

//=============================================================================================================

void MainWindow::about()
{
    if(!m_pAboutWindow) {
        m_pAboutWindow = QSharedPointer<QWidget>(new QWidget());

        QGridLayout *gridLayout;
        QLabel *m_label_splashcreen;
        QTextEdit *m_textEdit_aboutText;

        m_pAboutWindow->setObjectName(QStringLiteral("AboutWindow"));
        m_pAboutWindow->resize(541, 708);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pAboutWindow->sizePolicy().hasHeightForWidth());
        m_pAboutWindow->setSizePolicy(sizePolicy);
        m_pAboutWindow->setMinimumSize(QSize(541, 708));
        m_pAboutWindow->setMaximumSize(QSize(541, 708));
        gridLayout = new QGridLayout(m_pAboutWindow.data());
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        m_label_splashcreen = new QLabel(m_pAboutWindow.data());
        m_label_splashcreen->setObjectName(QStringLiteral("m_label_splashcreen"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_label_splashcreen->sizePolicy().hasHeightForWidth());
        m_label_splashcreen->setSizePolicy(sizePolicy1);
        m_label_splashcreen->setMinimumSize(QSize(0, 0));
        m_label_splashcreen->setPixmap(QPixmap(QString::fromUtf8(":/images/splashscreen.png")));
        m_label_splashcreen->setScaledContents(true);

        gridLayout->addWidget(m_label_splashcreen, 0, 0, 1, 1);

        m_textEdit_aboutText = new QTextEdit(m_pAboutWindow.data());
        m_textEdit_aboutText->setObjectName(QStringLiteral("m_textEdit_aboutText"));
        m_textEdit_aboutText->setEnabled(true);
        m_textEdit_aboutText->setReadOnly(true);
        m_textEdit_aboutText->setOverwriteMode(true);
        m_textEdit_aboutText->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse|Qt::TextBrowserInteraction|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        QLabel* pLabel = new QLabel();
        pLabel->setText(QString("Version: ") + CInfo::AppVersion() + " - " + QString(UTILSLIB::dateTimeNow()) + " - " + QString(UTILSLIB::gitHash()));

        gridLayout->addWidget(pLabel, 1, 0, 1, 1);
        gridLayout->addWidget(m_textEdit_aboutText, 2, 0, 1, 1);

        m_pAboutWindow->setWindowTitle(QApplication::translate("AboutWindow", "About", 0));
        m_label_splashcreen->setText(QString());
        m_textEdit_aboutText->setHtml(QApplication::translate("AboutWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
            "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
            "p, li { white-space: pre-wrap; }\n"
            "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
            "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt; font-weight:600;\">Copyright © 2012-2020 Lorenz Esch, Limin Sun, Viktor Klüber, Seok Lew,  Gabriel Motta, Ruben Dörfel, Daniel Baumgarten, P. Ellen Grant, Yoshio Okada, Jens Haueisen, Matti S Hämäläinen, Christoph Dinh. All rights reserved.</span></p>\n"
            "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
            "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-s"
                                    "ize:8pt;\">For more information visit the MNE-CPP/MNE Scan project on GitHub:</span></p>\n"
            "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
            "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"https://mne-cpp.github.io\"><span style=\" font-size:8pt; text-decoration: underline; color:#0000ff;\">https://mne-cpp.github.io/</span></a></p>\n"
            "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
            "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOL"
                                    "DERS AND CONTRIBUTORS \\&quot;AS IS\\&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</span></p>\n"
            "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
            "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-righ"
                                    "t:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistribution and use in source and binary forms, with or without modification, are permitted provided tha the following conditions are met:</span></p>\n"
            "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
            "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaime. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or pr"
            "omote products derived from this software without specific prior written permission.</span></p></body></html>", 0));

        m_pAboutWindow->setLayout(gridLayout);
    }

    m_pAboutWindow->show();
}

//=============================================================================================================

void MainWindow::setMinLogLevel()
{
    writeToLog(tr("minimal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvMin;
}

//=============================================================================================================

void MainWindow::setNormalLogLevel()
{
    writeToLog(tr("normal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvNormal;
}

//=============================================================================================================

void MainWindow::setMaxLogLevel()
{
    writeToLog(tr("maximal log level set"), _LogKndMessage, _LogLvMin);
    m_eLogLevelCurrent = _LogLvMax;
}

//=============================================================================================================

void MainWindow::createActions()
{
    //File QMenu
    m_pActionNewConfig = new QAction(QIcon(":/images/new.png"), tr("&New configuration"), this);
    m_pActionNewConfig->setShortcuts(QKeySequence::New);
    m_pActionNewConfig->setStatusTip(tr("Create a new configuration"));
    connect(m_pActionNewConfig.data(), &QAction::triggered,
            this, &MainWindow::newConfiguration);

    m_pActionOpenConfig = new QAction(tr("&Open configuration..."), this);
    m_pActionOpenConfig->setShortcuts(QKeySequence::Open);
    m_pActionOpenConfig->setStatusTip(tr("Open an existing configuration"));
    connect(m_pActionOpenConfig.data(), &QAction::triggered,
            this, &MainWindow::openConfiguration);

    m_pActionSaveConfig = new QAction(QIcon(":/images/save.png"), tr("&Save configuration..."), this);
    m_pActionSaveConfig->setShortcuts(QKeySequence::Save);
    m_pActionSaveConfig->setStatusTip(tr("Save the current configuration"));
    connect(m_pActionSaveConfig.data(), &QAction::triggered,
            this, &MainWindow::saveConfiguration);

    m_pActionExit = new QAction(tr("E&xit"), this);
    m_pActionExit->setShortcuts(QKeySequence::Quit);
    m_pActionExit->setStatusTip(tr("Exit the application"));
    connect(m_pActionExit.data(), &QAction::triggered,
            this, &MainWindow::close);

    //View QMenu
    m_pActionMinLgLv = new QAction(tr("&Minimal"), this);
    m_pActionMinLgLv->setCheckable(true);
    m_pActionMinLgLv->setShortcut(tr("Ctrl+1"));
    m_pActionMinLgLv->setStatusTip(tr("Set log level to minimal"));
    connect(m_pActionMinLgLv.data(), &QAction::triggered,
            this, &MainWindow::setMinLogLevel);

    m_pActionNormLgLv = new QAction(tr("&Normal"), this);
    m_pActionNormLgLv->setCheckable(true);
    m_pActionNormLgLv->setShortcut(tr("Ctrl+2"));
    m_pActionNormLgLv->setStatusTip(tr("Set log level to normal"));
    connect(m_pActionNormLgLv.data(), &QAction::triggered,
            this, &MainWindow::setNormalLogLevel);

    m_pActionMaxLgLv = new QAction(tr("Maximal"), this);
    m_pActionMaxLgLv->setCheckable(true);
    m_pActionMaxLgLv->setShortcut(tr("Ctrl+3"));
    m_pActionMaxLgLv->setStatusTip(tr("Set log level to maximal"));
    connect(m_pActionMaxLgLv.data(), &QAction::triggered,
            this, &MainWindow::setMaxLogLevel);

    m_pActionGroupLgLv = new QActionGroup(this);
    m_pActionGroupLgLv->addAction(m_pActionMinLgLv);
    m_pActionGroupLgLv->addAction(m_pActionNormLgLv);
    m_pActionGroupLgLv->addAction(m_pActionMaxLgLv);
    if (m_eLogLevelCurrent == _LogLvMin) {
        m_pActionMinLgLv->setChecked(true);
    } else if (m_eLogLevelCurrent == _LogLvNormal) {
        m_pActionNormLgLv->setChecked(true);
    } else {
        m_pActionMaxLgLv->setChecked(true);
    }

    //Appearance QMenu
    // Styles
    m_pActionStyleGroup = new QActionGroup(this);

    m_pActionDefaultMode = new QAction("Default");
    m_pActionDefaultMode->setStatusTip(tr("Activate default style"));
    m_pActionDefaultMode->setCheckable(true);
    m_pActionDefaultMode->setChecked(true);
    m_pActionStyleGroup->addAction(m_pActionDefaultMode);
    connect(m_pActionDefaultMode, &QAction::triggered,
        [=]() {
        onStyleChanged("default");
    });

    m_pActionDarkMode = new QAction("Dark");
    m_pActionDarkMode->setStatusTip(tr("Activate dark style"));
    m_pActionDarkMode->setCheckable(true);
    m_pActionDarkMode->setChecked(false);
    m_pActionStyleGroup->addAction(m_pActionDarkMode);
    connect(m_pActionDarkMode, &QAction::triggered,
        [=]() {
        onStyleChanged("dark");
    });

    // Modes
    m_pActionModeGroup = new QActionGroup(this);

    m_pActionResearchMode = new QAction("Research");
    m_pActionResearchMode->setStatusTip(tr("Activate the research GUI mode"));
    m_pActionResearchMode->setCheckable(true);
    m_pActionResearchMode->setChecked(true);
    m_pActionModeGroup->addAction(m_pActionResearchMode);
    connect(m_pActionResearchMode.data(), &QAction::triggered,
            this, &MainWindow::onGuiModeChanged);

    m_pActionClinicalMode = new QAction("Clinical");
    m_pActionClinicalMode->setStatusTip(tr("Activate the clinical GUI mode"));
    m_pActionClinicalMode->setCheckable(true);
    m_pActionClinicalMode->setChecked(false);
    m_pActionModeGroup->addAction(m_pActionClinicalMode);
    connect(m_pActionClinicalMode.data(), &QAction::triggered,
            this, &MainWindow::onGuiModeChanged);

    //Help QMenu
    m_pActionHelpContents = new QAction(tr("Help &Contents"), this);
    m_pActionHelpContents->setShortcuts(QKeySequence::HelpContents);
    m_pActionHelpContents->setStatusTip(tr("Show the help contents"));
    connect(m_pActionHelpContents.data(), &QAction::triggered,
            this, &MainWindow::helpContents);

    m_pActionAbout = new QAction(tr("&About"), this);
    m_pActionAbout->setStatusTip(tr("Show the application's About box"));
    connect(m_pActionAbout.data(), &QAction::triggered,
            this, &MainWindow::about);

    //QToolbar
    m_pActionRun = new QAction(QIcon(":/images/run.png"), tr("Run (F5)"), this);
    m_pActionRun->setShortcut(tr("F5"));
    m_pActionRun->setStatusTip(tr("Runs (F5) ")+CInfo::AppNameShort());
    connect(m_pActionRun.data(), &QAction::triggered,
            this, &MainWindow::startMeasurement);

    m_pActionStop = new QAction(QIcon(":/images/stop.png"), tr("Stop (F6)"), this);
    m_pActionStop->setShortcut(tr("F6"));
    m_pActionStop->setStatusTip(tr("Stops (F6) ")+CInfo::AppNameShort());
    connect(m_pActionStop.data(), &QAction::triggered,
            this, &MainWindow::stopMeasurement);

    //Display Toolbar
    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget"),this);
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            m_pQuickControlView.data(), &QuickControlView::show);
    m_pActionQuickControl->setVisible(false);
}

//=============================================================================================================

void MainWindow::createMenus()
{
    // File menu
    if(!m_pMenuFile) {
        m_pMenuFile = menuBar()->addMenu(tr("&File"));
        m_pMenuFile->addAction(m_pActionNewConfig);
        m_pMenuFile->addAction(m_pActionOpenConfig);
        m_pMenuFile->addAction(m_pActionSaveConfig);
        m_pMenuFile->addSeparator();
        m_pMenuFile->addAction(m_pActionExit);
    }

    // View menu
    if(!m_pMenuView) {
        m_pMenuView = menuBar()->addMenu(tr("&View"));
    }

    m_pMenuView->clear();

    if(m_pDockWidget_Log) {
        m_pMenuView->addAction(m_pDockWidget_Log->toggleViewAction());
    }
    m_pMenuLgLv = m_pMenuView->addMenu(tr("&Log Level"));
    m_pMenuLgLv->addAction(m_pActionMinLgLv);
    m_pMenuLgLv->addAction(m_pActionNormLgLv);
    m_pMenuLgLv->addAction(m_pActionMaxLgLv);
    m_pMenuView->addSeparator();

    if(m_pPluginGuiDockWidget) {
        m_pMenuView->addAction(m_pPluginGuiDockWidget->toggleViewAction());
    }

    for(int i = 0; i < m_qListDynamicDisplayMenuActions.size(); ++i) {
        m_pMenuView->addAction(m_qListDynamicDisplayMenuActions.at(i));
    }

    menuBar()->addSeparator();

    // Help Appearance
    if(!m_pMenuAppearance) {
        m_pMenuAppearance = menuBar()->addMenu(tr("&Appearance"));
        m_pMenuAppearance->addMenu("Styles")->addActions(m_pActionStyleGroup->actions());
        m_pMenuAppearance->addMenu("Modes")->addActions(m_pActionModeGroup->actions());
    }

    // Help menu
    if(!m_pMenuHelp) {
        m_pMenuHelp = menuBar()->addMenu(tr("&Help"));
        m_pMenuHelp->addAction(m_pActionHelpContents);
        m_pMenuHelp->addSeparator();
        m_pMenuHelp->addAction(m_pActionAbout);
    }
}

//=============================================================================================================

void MainWindow::createToolBars()
{
    //Control
    if(!m_pToolBar) {
        m_pToolBar = addToolBar(tr("Control"));
        m_pToolBar->addAction(m_pActionRun);
        m_pToolBar->addAction(m_pActionStop);
        m_pActionStop->setEnabled(false);

        m_pToolBar->addSeparator();

        m_pLabelTime = new QLabel(this);
        m_pToolBar->addWidget(m_pLabelTime);
        m_pLabelTime->setText(QTime(0, 0).toString());
    }

    //Plugin
    if(!m_pDynamicPluginToolBar) {
        m_pDynamicPluginToolBar = addToolBar(tr("Plugin Control"));
    }

    m_pDynamicPluginToolBar->addAction(m_pActionQuickControl);

    if(m_qListDynamicPluginActions.size() > 0) {
        for(qint32 i = 0; i < m_qListDynamicPluginActions.size(); ++i) {
            m_pDynamicPluginToolBar->addAction(m_qListDynamicPluginActions[i]);
        }
    }

    if(m_qListDynamicDisplayActions.size() > 0) {
        for(qint32 i = 0; i < m_qListDynamicDisplayActions.size(); ++i) {
            m_pDynamicPluginToolBar->addAction(m_qListDynamicDisplayActions[i]);
        }
    }
}

//=============================================================================================================

void MainWindow::initStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

//=============================================================================================================

void MainWindow::createPluginDockWindow()
{
    m_pPluginGuiDockWidget = new QDockWidget(tr("Plugins"), this);
    m_pPluginGuiDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_pPluginGui = new PluginGui(m_pPluginManager.data(), m_pPluginSceneManager.data());
    m_pPluginGui->setParent(m_pPluginGuiDockWidget);
    m_pPluginGuiDockWidget->setWidget(m_pPluginGui);

    addDockWidget(Qt::LeftDockWidgetArea, m_pPluginGuiDockWidget);

    connect(m_pPluginGui.data(), &PluginGui::selectedPluginChanged,
            this, &MainWindow::updatePluginSetupWidget);

    if(m_pPluginGui->getCurrentPlugin()) {
        updatePluginSetupWidget(m_pPluginGui->getCurrentPlugin());
    }

    connect(m_pPluginGui.data(), &PluginGui::selectedConnectionChanged,
            this, &MainWindow::updateConnectionWidget);

    m_pMenuView->addAction(m_pPluginGuiDockWidget->toggleViewAction());
}

//=============================================================================================================

void MainWindow::createLogDockWindow()
{
    //Log TextBrowser
    m_pDockWidget_Log = new QDockWidget(tr("Log"), this);

    m_pTextBrowser_Log = new QTextBrowser(m_pDockWidget_Log);

    m_pDockWidget_Log->setWidget(m_pTextBrowser_Log);

    m_pDockWidget_Log->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, m_pDockWidget_Log);

    m_pDockWidget_Log->hide();

    m_pMenuView->addAction(m_pDockWidget_Log->toggleViewAction());
}

//=============================================================================================================

void MainWindow::updatePluginSetupWidget(SCSHAREDLIB::AbstractPlugin::SPtr pPlugin)
{
    m_qListDynamicPluginActions.clear();

    if(!pPlugin.isNull()) {
        // Add Dynamic Plugin Actions
        m_qListDynamicPluginActions.append(pPlugin->getPluginActions());

        if(pPlugin.isNull()) {
            QWidget* pWidget = new QWidget;
            setCentralWidget(pWidget);
        } else {
            if(!m_bIsRunning) {
                setCentralWidget(pPlugin->setupWidget());
            }
        }
    } else {
        QWidget* pWidget = new QWidget;
        setCentralWidget(pWidget);
    }
}

//=============================================================================================================

void MainWindow::initMultiViewWidget(QList<QSharedPointer<SCSHAREDLIB::AbstractPlugin> > lPlugins)
{
    for(int i = 0; i < lPlugins.size(); ++i) {
        if(!lPlugins.at(i).isNull()) {
            // Add Dynamic Plugin Actions
            m_qListDynamicPluginActions.append(lPlugins.at(i)->getPluginActions());

            QString sCurPluginName = lPlugins.at(i)->getName();

            // Check for plugins which share the 3D View
            if(sCurPluginName == "HPI Fitting" ||
               sCurPluginName == "Source Localization" ||
               sCurPluginName == "Connectivity"){
                sCurPluginName = "3D View";
            }

            if(!m_bIsRunning) {
                setCentralWidget(lPlugins.at(i)->setupWidget());
            } else {                
                // Connect plugin controls to GUI mode toggling
                connect(this, &MainWindow::guiModeChanged,
                        lPlugins.at(i).data(), &AbstractPlugin::guiModeChanged);

                // Connect plugin controls to be added to the QuickControlView once available
                connect(lPlugins.at(i).data(), &AbstractPlugin::pluginControlWidgetsChanged,
                        this, &MainWindow::onPluginControlWidgetsChanged);

                // If a view is avialble for the plugin's output data, setup it up here
                if(QWidget* pWidget = m_pDisplayManager->show(lPlugins.at(i)->getOutputConnectors(),
                                                              m_pTime,
                                                              m_qListDynamicDisplayActions)) {                     
                    // Connect the view's controls to be added to the QuickControlView once available
                    for (int i = 0; i < pWidget->layout()->count(); ++i) {
                        if(MeasurementWidget* pMeasWidget = qobject_cast<MeasurementWidget *>(pWidget->layout()->itemAt(i)->widget())) {
                            connect(pMeasWidget, &MeasurementWidget::displayControlWidgetsChanged,
                                    this, &MainWindow::onDisplayControlWidgetsChanged);
                        }
                    }

                    // Plugins which have a RealTimeMultiSampleArray output are always displayed as the most lowest and tabbified vertical widget in the MultiView by default
                    // Please note that the MultiView will take ownership of the MultiViewWindow which inherits from QWidget
                    MultiViewWindow* pMultiViewWinow = Q_NULLPTR;

                    if(lPlugins.at(i)->getName() == "Filter" ||
                       lPlugins.at(i)->getName() == "Fiff Simulator" ||
                       lPlugins.at(i)->getName() == "FtBuffer" ||
                       lPlugins.at(i)->getName() == "Natus" ||
                       lPlugins.at(i)->getName() == "BabyMEG"||
                       lPlugins.at(i)->getName() == "BrainFlow"||
                       lPlugins.at(i)->getName() == "EEGoSports"||
                       lPlugins.at(i)->getName() == "GUSBAmp"||
                       lPlugins.at(i)->getName() == "LSL Adapter"||
                       lPlugins.at(i)->getName() == "TMSI"||
                       lPlugins.at(i)->getName() == "BrainAMP") {
                        pMultiViewWinow = m_pMultiView->addWidgetBottom(pWidget, sCurPluginName);
                    } else {
                        pMultiViewWinow = m_pMultiView->addWidgetTop(pWidget, sCurPluginName);
                    }

                    // Add Toggle Action to list so we can add to the View Menu
                    m_qListDynamicDisplayMenuActions.append(pMultiViewWinow->toggleViewAction());
                }

                m_pMultiView->show();
            }

        } else {
            QWidget* pWidget = new QWidget;
            setCentralWidget(pWidget);
        }
    }

    createToolBars();
    createMenus();
}

//=============================================================================================================

void MainWindow::onDockLocationChanged(QWidget* pWidget)
{
    // Everytime a dock widget in the MultiView changes its floating state we need to update its OpenGL viewport
    if(QVBoxLayout* vBoxLayout = qobject_cast<QVBoxLayout*>(pWidget->layout())) {
        for(int i = 0; i < vBoxLayout->count(); ++i) {
            if(QWidget *widget = pWidget->layout()->itemAt(i)->widget()) {
                if(RealTimeMultiSampleArrayWidget* pView = qobject_cast<RealTimeMultiSampleArrayWidget*>(widget)) {
                    pView->updateOpenGLViewport();
                } else if(RealTimeEvokedSetWidget* pView = qobject_cast<RealTimeEvokedSetWidget*>(widget)) {
                    pView->updateOpenGLViewport();
                }
            }
        }
    } else if(RealTimeMultiSampleArrayWidget* pView = qobject_cast<RealTimeMultiSampleArrayWidget*>(pWidget)) {
        pView->updateOpenGLViewport();
    } else if(RealTimeEvokedSetWidget* pView = qobject_cast<RealTimeEvokedSetWidget*>(pWidget)) {
        pView->updateOpenGLViewport();
    }
}

//=============================================================================================================

void MainWindow::onPluginControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                               const QString& sPluginName)
{
    if(m_pQuickControlView) {
        m_pQuickControlView->addWidgets(lControlWidgets, sPluginName);
    }
}

//=============================================================================================================

void MainWindow::onDisplayControlWidgetsChanged(QList<QWidget*>& lControlWidgets,
                                                const QString& sPluginName)
{
    if(m_pQuickControlView) {
        m_pQuickControlView->addWidgets(lControlWidgets, sPluginName);
    }
}

//=============================================================================================================

void MainWindow::updateConnectionWidget(SCSHAREDLIB::PluginConnectorConnection::SPtr pConnection)
{
    QWidget* pWidget = pConnection->setupWidget();
    setCentralWidget(pWidget);
}

//=============================================================================================================

void MainWindow::writeToLog(const QString& logMsg,
                            LogKind lgknd,
                            LogLevel lglvl)
{
    if(lglvl<=m_eLogLevelCurrent) {
        if(lgknd == _LogKndError) {
            m_pTextBrowser_Log->insertHtml("<font color=red><b>Error:</b> "+logMsg+"</font>");
        } else if(lgknd == _LogKndWarning) {
            m_pTextBrowser_Log->insertHtml("<font color=blue><b>Warning:</b> "+logMsg+"</font>");
        } else {
            m_pTextBrowser_Log->insertHtml(logMsg);
        }
        m_pTextBrowser_Log->insertPlainText("\n"); // new line

        //scroll down to the newest entry
        QTextCursor c = m_pTextBrowser_Log->textCursor();
        c.movePosition(QTextCursor::End);
        m_pTextBrowser_Log->setTextCursor(c);

        m_pTextBrowser_Log->verticalScrollBar()->setValue(m_pTextBrowser_Log->verticalScrollBar()->maximum());
    }
}

//=============================================================================================================

void MainWindow::startMeasurement()
{
    // Save pipeline before starting just in case a crash occurs
    m_pPluginGui->saveConfig(QStandardPaths::writableLocation(QStandardPaths::DataLocation),"default.xml");

    writeToLog(tr("Starting real-time measurement..."), _LogKndMessage, _LogLvMin);

    if(!m_pPluginSceneManager->startPlugins()) {
        QMessageBox::information(0, tr("MNE Scan - Start"), QString(QObject::tr("Not able to start all plugins!")), QMessageBox::Ok);
        m_pPluginSceneManager->stopPlugins();
        return;
    }

    m_pPluginGui->uiSetupRunningState(true);
    uiSetupRunningState(true);
    startTimer(m_iTimeoutMSec);

    delete m_pMultiView;
    m_pMultiView = new MultiView();
    connect(m_pMultiView.data(), &MultiView::dockLocationChanged,
            this, &MainWindow::onDockLocationChanged);
    setCentralWidget(m_pMultiView);

    m_pActionQuickControl->setVisible(true);
    //m_pDynamicPluginToolBar->addAction(m_pActionQuickControl);
    initMultiViewWidget(m_pPluginSceneManager->getPlugins());
}

//=============================================================================================================

void MainWindow::stopMeasurement()
{
    writeToLog(tr("Stopping real-time measurement..."), _LogKndMessage, _LogLvMin);

    //Stop all plugins
    m_pPluginSceneManager->stopPlugins();
    m_pDisplayManager->clean();

    // Hide and clear QuickControlView
    m_pQuickControlView->hide();
    m_pActionQuickControl->setVisible(false);
    m_pQuickControlView->clear();

    // Clear dynamic toolbar holding plugin, dsiplay and the QuickControlView actions
    m_qListDynamicDisplayActions.clear();
    m_qListDynamicDisplayMenuActions.clear();    
    m_qListDynamicPluginActions.clear();
    m_pDynamicPluginToolBar->clear();

    m_pPluginGui->uiSetupRunningState(false);
    uiSetupRunningState(false);
    stopTimer();

    updatePluginSetupWidget(m_pPluginGui->getCurrentPlugin());
}

//=============================================================================================================

void MainWindow::uiSetupRunningState(bool state)
{
    m_pActionRun->setEnabled(!state);
    m_pActionStop->setEnabled(state);
    m_bIsRunning = state;
}

//=============================================================================================================

void MainWindow::startTimer(int msec)
{
    m_pTimer = QSharedPointer<QTimer>(new QTimer(this));
    connect(m_pTimer.data(), &QTimer::timeout, this, &MainWindow::updateTime);
    m_pTimer->start(msec);
    m_pTime->setHMS(0,0,0);
    QString strTime = m_pTime->toString();
    m_pLabelTime->setText(strTime);
}

//=============================================================================================================

void MainWindow::stopTimer()
{
    disconnect(m_pTimer.data(), &QTimer::timeout, this, &MainWindow::updateTime);
}

//=============================================================================================================

void MainWindow::updateTime()
{
     *m_pTime = m_pTime->addMSecs(m_iTimeoutMSec);
    QString strTime = m_pTime->toString();
    m_pLabelTime->setText(strTime);
}
