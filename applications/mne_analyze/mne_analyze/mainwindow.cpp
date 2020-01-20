//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch. All rights reserved.
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
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "info.h"
#include "mainwindow.h"
#include "mdiview.h"
#include "../libs/anShared/Interfaces/IExtension.h"
#include "../libs/anShared/Management/analyzesettings.h"
#include "../libs/anShared/Management/analyzedata.h"
#include "../libs/anShared/Management/extensionmanager.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QMenu>
#include <QMenuBar>
#include <QDockWidget>
#include <QAction>
#include <QLabel>
#include <QTextEdit>
#include <QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QStandardPaths>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// CONST
//=============================================================================================================

const char* extensionsDir = "/mne_analyze_extensions";        /**< holds path to the extensions.*/


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
, m_pExtensionManager(new ANSHAREDLIB::ExtensionManager(this))
, m_pMdiView(Q_NULLPTR)
{
    fprintf(stderr, "%s - Version %s\n",
            CInfo::AppNameShort().toUtf8().constData(),
            CInfo::AppVersion().toUtf8().constData());

    setWindowState(Qt::WindowMaximized);
    setMinimumSize(400, 400);
    setWindowTitle(CInfo::AppNameShort());

    initGlobalSettings();
    initGlobalData();
    m_pExtensionManager->loadExtension(qApp->applicationDirPath()+extensionsDir);
    m_pExtensionManager->initExtensions(m_analyzeSettings, m_analyzeData);

    createDockWindows();
    createMdiView();

    createActions();
    createMenus();
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{

}


//*************************************************************************************************************

void MainWindow::initGlobalSettings()
{
    m_analyzeSettings = AnalyzeSettings::SPtr(new AnalyzeSettings);
}


//*************************************************************************************************************

void MainWindow::initGlobalData()
{
    m_analyzeData = AnalyzeData::SPtr(new AnalyzeData);
}


//*************************************************************************************************************

void MainWindow::createActions()
{
    //File QMenu
    m_pActionOpenDataFile = new QAction(tr("&Open Fiff File"), this);
    m_pActionOpenDataFile->setShortcuts(QKeySequence::New);
    m_pActionOpenDataFile->setStatusTip(tr("Opens a Fiff File"));
    connect(m_pActionOpenDataFile, &QAction::triggered, this, &MainWindow::openFiffFile);

    m_pActionExit = new QAction(tr("E&xit"), this);
    m_pActionExit->setShortcuts(QKeySequence::Quit);
    m_pActionExit->setStatusTip(tr("Exit the application"));
    connect(m_pActionExit, &QAction::triggered, this, &MainWindow::close);

    //View QMenu
    m_pActionCascade = new QAction(tr("&Cascade"), this);
    m_pActionCascade->setStatusTip(tr("Cascade the windows in the mdi window"));
    connect(m_pActionCascade, &QAction::triggered, this->m_pMdiView, &MdiView::cascadeSubWindows);

    m_pActionTile = new QAction(tr("&Tile"), this);
    m_pActionTile->setStatusTip(tr("Tile the windows in the mdi window"));
    connect(m_pActionTile, &QAction::triggered, this->m_pMdiView, &MdiView::tileSubWindows);

    m_pActionPrint = new QAction(tr("&Print..."), this);
    m_pActionPrint->setStatusTip(tr("Prints the current view."));
    m_pActionPrint->setShortcut(QKeySequence::Print);
    connect(m_pActionPrint, &QAction::triggered, this->m_pMdiView, &MdiView::printCurrentSubWindow);

    //Help QMenu
    m_pActionAbout = new QAction(tr("&About"), this);
    m_pActionAbout->setStatusTip(tr("Show the application's About box"));
    connect(m_pActionAbout, &QAction::triggered, this, &MainWindow::about);
}


//*************************************************************************************************************

void MainWindow::createMenus()
{
    m_pMenuFile = menuBar()->addMenu(tr("&File"));
    m_pMenuFile->addAction(m_pActionOpenDataFile);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(m_pActionExit);

    m_pMenuView = menuBar()->addMenu(tr("&View"));
    m_pMenuView->addAction(m_pActionCascade);
    m_pMenuView->addAction(m_pActionTile);
    m_pMenuView->addSeparator();
    m_pMenuView->addAction(m_pActionPrint);

    menuBar()->addSeparator();

    m_pMenuHelp = menuBar()->addMenu(tr("&Help"));
    m_pMenuHelp->addAction(m_pActionAbout);
}


//*************************************************************************************************************

void MainWindow::createDockWindows()
{
    setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::West);
    setTabPosition(Qt::RightDockWidgetArea,QTabWidget::East);
    setDockOptions(QMainWindow::ForceTabbedDocks);

    //Add Extension views to mdi
    for(int i = 0; i < m_pExtensionManager->getExtensions().size(); ++i) {
        IExtension* extension = m_pExtensionManager->getExtensions()[i];

        qDebug() << "create dock" << extension->getName();

        QDockWidget* control = extension->getControl();
        if(control) {
            addDockWidget(Qt::LeftDockWidgetArea,control);
        }
    }

    tabifyDockWindows();
}


//*************************************************************************************************************

void MainWindow::createMdiView()
{
    m_pMdiView = new MdiView(this);
    setCentralWidget(m_pMdiView);

    //Add Extension views to mdi
    for(int i = 0; i < m_pExtensionManager->getExtensions().size(); ++i) {
        IExtension* extension = m_pExtensionManager->getExtensions()[i];

        qDebug() << "create mdi" << extension->getName();

        QWidget* view = extension->getView();
        if(view) {
            m_pMdiView->addSubWindow(view);
        }
    }

    m_pMdiView->cascadeSubWindows();
}


//*************************************************************************************************************

void MainWindow::tabifyDockWindows()
{
    //
    // get a list of all the docks
    //
    QList<QDockWidget*> docks = findChildren<QDockWidget*>();

    //
    // first, un-float all the tabs
    //
    std::for_each(docks.begin(), docks.end(), std::bind(&QDockWidget::setFloating, std::placeholders::_1 /* the dock widget*/, false));

    //
    // sort them into dockWidget areas
    //
    QVector<QDockWidget*> topArea, leftArea, rightArea, bottomArea;
    QVector<QVector<QDockWidget*>*> dockAreas;

    dockAreas.push_back(&topArea);
    dockAreas.push_back(&leftArea);
    dockAreas.push_back(&rightArea);
    dockAreas.push_back(&bottomArea);

    std::for_each(docks.begin(), docks.end(), [&] (QDockWidget* dock)
    {
        if      (dockWidgetArea(dock) ==  Qt::TopDockWidgetArea     )   {topArea.   push_back(dock);    this->removeDockWidget(dock); dock->resize(dock->minimumSizeHint());    this->addDockWidget(Qt::TopDockWidgetArea   , dock); dock->setVisible(true);}
        else if (dockWidgetArea(dock) ==  Qt::LeftDockWidgetArea    )   {leftArea.  push_back(dock);    this->removeDockWidget(dock); dock->resize(dock->minimumSizeHint());    this->addDockWidget(Qt::LeftDockWidgetArea  , dock); dock->setVisible(true);}
        else if (dockWidgetArea(dock) ==  Qt::RightDockWidgetArea   )   {rightArea. push_back(dock);    this->removeDockWidget(dock); dock->resize(dock->minimumSizeHint());    this->addDockWidget(Qt::RightDockWidgetArea , dock); dock->setVisible(true);}
        else if (dockWidgetArea(dock) ==  Qt::BottomDockWidgetArea  )   {bottomArea.push_back(dock);    this->removeDockWidget(dock); dock->resize(dock->minimumSizeHint());    this->addDockWidget(Qt::BottomDockWidgetArea, dock); dock->setVisible(true);}
    });

    //
    // then, tab them all
    //
    for (QVector<QVector<QDockWidget*>*>::iterator areasItr = dockAreas.begin(); areasItr != dockAreas.end(); areasItr++)
    {
        // within each area, tab all the docks if there are more than 1
        QVector<QDockWidget*> area = **areasItr;
        for (int i = 1; i < area.size(); ++i) {
            this->tabifyDockWidget(area[i-1], area[i]);
        }
    }
}


//*************************************************************************************************************

void MainWindow::openFiffFile()
{
    //Open a FIFF file

    //Get the path
    m_fiffFileName = QFileDialog::getOpenFileName(this,
                                                    ("Open File"),
                                                    "C:/",
                                                    ("fiff File(*.fiff)"));
    //Open file
    QFile m_fiffFile(m_fiffFileName);
}


//*************************************************************************************************************

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
        m_label_splashcreen->setPixmap(QPixmap(QString::fromUtf8(":/images/splashscreen_mne_analyze.png")));
        m_label_splashcreen->setScaledContents(true);

        gridLayout->addWidget(m_label_splashcreen, 0, 0, 1, 1);

        m_textEdit_aboutText = new QTextEdit(m_pAboutWindow.data());
        m_textEdit_aboutText->setObjectName(QStringLiteral("m_textEdit_aboutText"));
        m_textEdit_aboutText->setEnabled(true);
        m_textEdit_aboutText->setReadOnly(true);
        m_textEdit_aboutText->setOverwriteMode(true);
        m_textEdit_aboutText->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse|Qt::TextBrowserInteraction|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        gridLayout->addWidget(m_textEdit_aboutText, 1, 0, 1, 1);

        m_pAboutWindow->setWindowTitle(tr("About"));
        m_label_splashcreen->setText(QString());
        m_textEdit_aboutText->setHtml( tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                                          "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
                                          "p, li { white-space: pre-wrap; }\n"
                                          "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:7.8pt; font-weight:400; font-style:normal;\">\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt; font-weight:600;\">Copyright (C) 2017 Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">For more information visit the MNE-CPP/MNE Analyze project on its homepage:</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><a href=\"http://www.mne-cpp.org\"><span style=\" font-size:8pt; text-decoration: underline; color:#0000ff;\">http://www.mne-cpp.org</span></a></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \&quot;AS IS\&quot; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistribution and use in source and binary forms, with or without modification, are permitted provided tha the following conditions are met:</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaime. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.</span></p></body></html>"));

        m_pAboutWindow->setLayout(gridLayout);
    }

    m_pAboutWindow->show();
}
