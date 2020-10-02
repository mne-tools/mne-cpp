//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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

#include "info.h"
#include "mainwindow.h"

#include <anShared/Plugins/abstractplugin.h>
#include <anShared/Management/pluginmanager.h>
#include <anShared/Management/statusbar.h>

#include <disp/viewers/multiview.h>
#include <disp/viewers/multiviewwindow.h>
#include <disp/viewers/abstractview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QtWidgets>
#include <QMenu>
#include <QMenuBar>
#include <QDockWidget>
#include <QAction>
#include <QLabel>
#include <QTextEdit>
#include <QGridLayout>
#include <QtWidgets/QGridLayout>
#include <QStandardPaths>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;
using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QSharedPointer<ANSHAREDLIB::PluginManager> pPluginManager,
                       QWidget *parent)
: QMainWindow(parent)
, m_pMultiView(Q_NULLPTR)
, m_pGridLayout(Q_NULLPTR)
, m_sSettingsPath("MNEANALYZE/MainWindow")
, m_sCurrentStyle("default")
{
    this->setObjectName("mainwindow");
    setWindowState(Qt::WindowMaximized);
    setMinimumSize(1280, 720);
    setWindowTitle(CInfo::AppNameShort());

    if(!pPluginManager.isNull()) {
        // The order we call these functions is important!
        createActions();
        createPluginMenus(pPluginManager);
        createLogDockWindow();
        createPluginControls(pPluginManager);
        createPluginViews(pPluginManager);
    } else {
        qWarning() << "[MainWindow::MainWindow] Plugin manager is nullptr!";
    }

    StatusBar* statusBar = new StatusBar(this);

    this->setStatusBar(statusBar);

    //Load application icon for linux builds only, mac and win executables have built in icons from .pro file
#ifdef __linux__
    qInfo() << "Loading icon...";
    QMainWindow::setWindowIcon(QIcon(":/images/resources/images/appIcons/icon_mne-analyze_256x256.png"));
#endif

    //Load saved GUI geometry and state
    loadSettings();
    m_pMultiView->loadSettings();
}

//=============================================================================================================

MainWindow::~MainWindow()
{
}

//=============================================================================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    //Save GUI gemoetry and state;
    m_pMultiView->saveSettings();
    saveSettings();

    this->setAttribute(Qt::WA_DeleteOnClose);

    for(QDockWidget* widget : this->findChildren<QDockWidget*>()){
        widget->setAttribute(Qt::WA_DeleteOnClose);
    }

    emit mainWindowClosed();
    QMainWindow::closeEvent(event);
}

//=============================================================================================================

void MainWindow::writeToLog(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg)
{
    Q_UNUSED(context);

    switch (type)
    {
        case QtDebugMsg:
            m_pTextBrowser_Log->insertHtml("<font color=green><b>[DEBUG]</b> "+msg+"</font>");
            break;
        case QtInfoMsg:
            m_pTextBrowser_Log->insertHtml("<font color=green><b>[INFO]</b> "+msg+"</font>");
            break;
        case QtWarningMsg:
            m_pTextBrowser_Log->insertHtml("<font color=purple><b>[WARNING]</b> "+msg+"</font>");
            break;
        case QtCriticalMsg:
            m_pTextBrowser_Log->insertHtml("<font color=red><b>[CRITICAL]</b> "+msg+"</font>");
            break;
        case QtFatalMsg:
            m_pTextBrowser_Log->insertHtml("<font color=purple><b>[FATAL]</b> "+msg+"</font>");
            abort();
            break;
        default:
            m_pTextBrowser_Log->insertHtml("<font color=black><b>[UNKOWN]</b> "+msg+"</font>");
            break;
    }

    // Add new line and scroll down to the newest entry
    m_pTextBrowser_Log->insertPlainText("\n");

    QTextCursor c = m_pTextBrowser_Log->textCursor();
    c.movePosition(QTextCursor::End);
    m_pTextBrowser_Log->setTextCursor(c);

    m_pTextBrowser_Log->verticalScrollBar()->setValue(m_pTextBrowser_Log->verticalScrollBar()->maximum());
}

//=============================================================================================================

void MainWindow::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + "/layout");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();

    settings.setValue(m_sSettingsPath + QString("/styleMode"), m_sCurrentStyle);
}

//=============================================================================================================

void MainWindow::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + "/layout");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();

    m_sCurrentStyle = settings.value(m_sSettingsPath + QString("/styleMode"), "default").toString();
    if(m_sCurrentStyle == "dark") {
        m_pActionDarkMode->setChecked(true);
    } else {
        m_pActionDarkMode->setChecked(false);
    }

    onStyleChanged(m_sCurrentStyle);
}

//=============================================================================================================

void MainWindow::createActions()
{
    m_pActionExit = new QAction(tr("Exit"), this);
    m_pActionExit->setShortcuts(QKeySequence::Quit);
    m_pActionExit->setStatusTip(tr("Exit the application"));
    connect(m_pActionExit.data(), &QAction::triggered, this, &MainWindow::close);

    //Help QMenu
    m_pActionAbout = new QAction(tr("About"), this);
    m_pActionAbout->setStatusTip(tr("Show the application's About box"));
    connect(m_pActionAbout.data(), &QAction::triggered, this, &MainWindow::about);
}

//=============================================================================================================

void MainWindow::onStyleChanged(const QString& sStyle)
{
    if(QApplication *pApp = qobject_cast<QApplication *>(QApplication::instance())) {
        if(sStyle == "default") {
            m_sCurrentStyle = "default";
            pApp->setStyleSheet("");
        } else if (sStyle == "dark") {
            m_sCurrentStyle = "dark";
            QFile file(":/dark.qss");
            file.open(QFile::ReadOnly);
            QTextStream stream(&file);
            pApp->setStyleSheet(stream.readAll());
        }

        // Set default font
        int id = QFontDatabase::addApplicationFont(":/fonts/Roboto-Light.ttf");
        pApp->setFont(QFont(QFontDatabase::applicationFontFamilies(id).at(0)));
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

void MainWindow::createLogDockWindow()
{
    //Log TextBrowser
    QDockWidget* pDockWidget_Log = new QDockWidget(tr("Log"), this);
    pDockWidget_Log->setObjectName("Log");

    // Disable floating and editable dock widgets, since the wasm QDockWidget version is buggy
    #ifdef WASMBUILD
    pDockWidget_Log->setFeatures(QDockWidget::DockWidgetClosable);
    #endif

    m_pTextBrowser_Log = new QTextBrowser(pDockWidget_Log);

    pDockWidget_Log->setWidget(m_pTextBrowser_Log);

    pDockWidget_Log->setAllowedAreas(Qt::BottomDockWidgetArea);
    addDockWidget(Qt::BottomDockWidgetArea, pDockWidget_Log);

    m_pMenuView->addAction(pDockWidget_Log->toggleViewAction());

    pDockWidget_Log->hide();
}

//=============================================================================================================

void MainWindow::createPluginMenus(QSharedPointer<ANSHAREDLIB::PluginManager> pPluginManager)
{
    // File menu
    m_pMenuFile = menuBar()->addMenu(tr("File"));
    m_pMenuFile->addAction(m_pActionExit);

    // View menu
    m_pMenuView = menuBar()->addMenu(tr("View"));

    // Control menu
    m_pMenuControl = menuBar()->addMenu(tr("Control"));

    //Appearance QMenu
    // Styles
    QActionGroup* pActionStyleGroup = new QActionGroup(this);

    QAction* pActionDefaultStyle = new QAction("Default");
    pActionDefaultStyle->setStatusTip(tr("Activate default style"));
    pActionDefaultStyle->setCheckable(true);
    pActionDefaultStyle->setChecked(true);
    pActionStyleGroup->addAction(pActionDefaultStyle);
    connect(pActionDefaultStyle, &QAction::triggered,
        [=]() {
        onStyleChanged("default");
    });

    m_pActionDarkMode = new QAction("Dark");
    m_pActionDarkMode->setStatusTip(tr("Activate dark style"));
    m_pActionDarkMode->setCheckable(true);
    m_pActionDarkMode->setChecked(false);
    pActionStyleGroup->addAction(m_pActionDarkMode);
    connect(m_pActionDarkMode.data(), &QAction::triggered,
        [=]() {
        onStyleChanged("dark");
    });

    // Modes
    QActionGroup* pActionModeGroup = new QActionGroup(this);

    m_pActionResearchMode = new QAction("Research");
    m_pActionResearchMode->setStatusTip(tr("Activate the research GUI mode"));
    m_pActionResearchMode->setCheckable(true);
    m_pActionResearchMode->setChecked(true);
    pActionModeGroup->addAction(m_pActionResearchMode);
    connect(m_pActionResearchMode.data(), &QAction::triggered,
            this, &MainWindow::onGuiModeChanged);

    m_pActionClinicalMode = new QAction("Clinical");
    m_pActionClinicalMode->setStatusTip(tr("Activate the clinical GUI mode"));
    m_pActionClinicalMode->setCheckable(true);
    m_pActionClinicalMode->setChecked(false);
    pActionModeGroup->addAction(m_pActionClinicalMode);
    connect(m_pActionClinicalMode.data(), &QAction::triggered,
            this, &MainWindow::onGuiModeChanged);

    if(!m_pMenuAppearance) {
        m_pMenuAppearance = menuBar()->addMenu(tr("&Appearance"));
        m_pMenuAppearance->addMenu("Styles")->addActions(pActionStyleGroup->actions());
        m_pMenuAppearance->addMenu("Modes")->addActions(pActionModeGroup->actions());
    }

    // Help menu
    m_pMenuHelp = menuBar()->addMenu(tr("Help"));
    m_pMenuHelp->addAction(m_pActionAbout);

    // add plugins menus
    for(AbstractPlugin* pPlugin : pPluginManager->getPlugins()) {
        pPlugin->setObjectName(pPlugin->getName());
        if(pPlugin) {
            if (QMenu* pMenu = pPlugin->getMenu()) {
                // Check if the menu already exists. If it does add the actions to the exisiting menu.
                if(pMenu->title() == "File") {
                    for(QAction* pAction : pMenu->actions()) {
                        m_pMenuFile->insertAction(m_pActionExit, pAction);
                    }
                } else if(pMenu->title() == "View") {
                    for(QAction* pAction : pMenu->actions()) {
                        m_pMenuView->addAction(pAction);
                    }
                } else if(pMenu->title() == "Help") {
                    for(QAction* pAction : pMenu->actions()) {
                        m_pMenuHelp->insertAction(m_pActionAbout, pAction);
                    }
                } else {
                    menuBar()->addMenu(pMenu);
                }
            }
        }
    }
}

//=============================================================================================================

void MainWindow::createPluginControls(QSharedPointer<ANSHAREDLIB::PluginManager> pPluginManager)
{
    setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::West);
    setTabPosition(Qt::RightDockWidgetArea,QTabWidget::East);
    setDockOptions(QMainWindow::ForceTabbedDocks);

    //Add Plugin controls to the MainWindow
    for(AbstractPlugin* pPlugin : pPluginManager->getPlugins()) {
        if(QDockWidget* pControl = pPlugin->getControl()) {
            addDockWidget(Qt::LeftDockWidgetArea, pControl);
            qInfo() << "[MainWindow::createPluginControls] Found and added dock widget for " << pPlugin->getName();
            QAction* pAction = pControl->toggleViewAction();
            pAction->setText(pPlugin->getName());
            m_pMenuControl->addAction(pAction);
            qInfo() << "[MainWindow::createPluginControls] Added" << pPlugin->getName() << "controls to View menu";

            // Connect plugin controls to GUI mode toggling
            connect(this, &MainWindow::guiModeChanged,
                    pPlugin, &AbstractPlugin::guiModeChanged);

            // Disable floating and editable dock widgets, since the wasm QDockWidget version is buggy
            #ifdef WASMBUILD
            pControl->setFeatures(QDockWidget::DockWidgetClosable);
            #endif
        }
    }

    tabifyDockWindows();
}

//=============================================================================================================

void MainWindow::createPluginViews(QSharedPointer<PluginManager> pPluginManager)
{
    m_pGridLayout = new QGridLayout(this);
    m_pMultiView = new MultiView(m_sSettingsPath);
    m_pGridLayout->addWidget(m_pMultiView);
    m_pMultiView->show();
    m_pMultiView->setObjectName("multiview");
    setCentralWidget(m_pMultiView);

    QString sCurPluginName;

    //Add Plugin views to the MultiView, which is the central widget
    for(AbstractPlugin* pPlugin : pPluginManager->getPlugins()) {
        QWidget* pView = pPlugin->getView();
        if(pView) {
            sCurPluginName = pPlugin->getName();
            MultiViewWindow* pWindow = Q_NULLPTR;

            if(sCurPluginName == "RawDataViewer") {
                pWindow = m_pMultiView->addWidgetBottom(pView, sCurPluginName);
            } else {
                pWindow = m_pMultiView->addWidgetTop(pView, sCurPluginName);
            }

            QAction* pAction = pWindow->toggleViewAction();
            pAction->setText(pPlugin->getName());
            m_pMenuView->addAction(pAction);

            qInfo() << "[MainWindow::createPluginViews] Found and added subwindow for " << pPlugin->getName();
        }
    }
    //m_pMultiView->loadSettings();
}

//=============================================================================================================

void MainWindow::tabifyDockWindows()
{
    // get a list of all the docks
    QList<QDockWidget*> docks = findChildren<QDockWidget*>();

    // first, un-float all the tabs
    for (QDockWidget* pDockWidget : docks) pDockWidget->setFloating(false);

    // sort them into dockWidget areas
    QVector<QDockWidget*> topArea, leftArea, rightArea, bottomArea;
    QVector<QVector<QDockWidget*>*> dockAreas{&topArea, &leftArea, &rightArea, &bottomArea};

    for (QDockWidget* pDockWidget : docks) {        
        // Disable floating and editable dock widgets, since the wasm QDockWidget version is buggy
        #ifdef WASMBUILD
        pDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
        #endif

        // default with left area
        Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
        switch (dockWidgetArea(pDockWidget))
        {
        case Qt::TopDockWidgetArea:
            topArea.push_back(pDockWidget);
            area = Qt::TopDockWidgetArea;
            break;
        case Qt::LeftDockWidgetArea:
            leftArea.push_back(pDockWidget);
            area = Qt::LeftDockWidgetArea;
            break;
        case Qt::RightDockWidgetArea:
            rightArea.push_back(pDockWidget);
            area = Qt::RightDockWidgetArea;
            break;
        case Qt::BottomDockWidgetArea:
            bottomArea.push_back(pDockWidget);
            area = Qt::BottomDockWidgetArea;
            break;
        default:
            qDebug() << "[MainWindow::tabifyDockWindows] Unhandled dock widget area";
            break;
        }
        removeDockWidget(pDockWidget);
        pDockWidget->resize(pDockWidget->minimumSizeHint());
        addDockWidget(area, pDockWidget);
        pDockWidget->setVisible(true);
    }

    // then, tab them all
    for (const QVector<QDockWidget*>* pArea : dockAreas) {
        // within each area, tab all the docks if there are more than 1
        for (int i = 1; i < pArea->size(); ++i) {
            tabifyDockWidget((*pArea)[i - 1], (*pArea)[i]);
        }
    }
}

//=============================================================================================================

void MainWindow::about()
{
    if(!m_pAboutWindow) {
        m_pAboutWindow = QSharedPointer<QWidget>(new QWidget(this, Qt::Window));

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
        m_label_splashcreen->setPixmap(QPixmap(QString::fromUtf8(":/images/splashscreen_mne_analyze.png")));
        m_label_splashcreen->setScaledContents(true);

        gridLayout->addWidget(m_label_splashcreen, 0, 0, 1, 1);

        m_textEdit_aboutText = new QTextEdit(m_pAboutWindow.data());
        m_textEdit_aboutText->setObjectName(QStringLiteral("m_textEdit_aboutText"));
        m_textEdit_aboutText->setEnabled(true);
        m_textEdit_aboutText->setReadOnly(true);
        m_textEdit_aboutText->setOverwriteMode(true);
        m_textEdit_aboutText->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse|Qt::TextBrowserInteraction|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        QLabel* pLabel = new QLabel();
        pLabel->setText(QString("Version: ") + CInfo::AppVersion());

        gridLayout->addWidget(pLabel, 1, 0, 1, 1);

        gridLayout->addWidget(m_textEdit_aboutText, 2, 0, 1, 1);

        m_pAboutWindow->setWindowTitle(tr("About"));
        m_label_splashcreen->setText(QString());
        m_textEdit_aboutText->setHtml( tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                                          "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
                                          "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:7.8pt; font-weight:400; font-style:normal;\">"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">For more information please visit the MNE-CPP/MNE Analyze project on its homepage:</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://www.mne-cpp.org\"><span style=\" font-size:8pt; text-decoration: underline; color:#0000ff;\">http://www.mne-cpp.org</span></a></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS &quot;AS IS&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistribution and use in source and binary forms, with or without modification, are permitted provided tha the following conditions are met:</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaime. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.</span></p></body></html>"));

        m_pAboutWindow->setLayout(gridLayout);
    }

    m_pAboutWindow->show();
}
