//=============================================================================================================
/**
* @file     mainwindow.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh, Lorenz Esch Lars Debor, Simon Heinke and Matti Hamalainen. All rights reserved.
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
#include "../libs/anShared/Management/extensionmanager.h"
#include "../libs/anShared/Management/statusbar.h"


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;
using namespace ANSHAREDLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QSharedPointer<ANSHAREDLIB::ExtensionManager> pExtensionManager, QWidget *parent)
    : QMainWindow(parent),
      m_pMdiView(Q_NULLPTR),
      m_pGridLayout(Q_NULLPTR)
{
    setWindowState(Qt::WindowMaximized);
    setMinimumSize(400, 400);
    setWindowTitle(CInfo::AppNameShort());

    if(!pExtensionManager.isNull()) {
        createDockWindows(pExtensionManager);
        createMdiView(pExtensionManager);
        createActions();
        createMenus(pExtensionManager);
    }
    else {
        qDebug() << "[MainWindow::MainWindow] CRITICAL ! Extension manager is nullptr";
    }

    this->setStatusBar(new StatusBar());
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{

}


//*************************************************************************************************************

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit mainWindowClosed();
    // default implementation does this, so its probably a good idea
    event->accept();
}


//*************************************************************************************************************

void MainWindow::createActions()
{
    m_pActionExit = new QAction(tr("Exit"), this);
    m_pActionExit->setShortcuts(QKeySequence::Quit);
    m_pActionExit->setStatusTip(tr("Exit the application"));
    connect(m_pActionExit, &QAction::triggered, this, &MainWindow::close);

    //View QMenu
    m_pActionCascade = new QAction(tr("Cascade"), this);
    m_pActionCascade->setStatusTip(tr("Cascade the windows in the mdi window"));
    connect(m_pActionCascade, &QAction::triggered, this->m_pMdiView, &MdiView::cascadeSubWindows);

    m_pActionTile = new QAction(tr("Tile"), this);
    m_pActionTile->setStatusTip(tr("Tile the windows in the mdi window"));
    connect(m_pActionTile, &QAction::triggered, this->m_pMdiView, &MdiView::tileSubWindows);

    m_pActionPrint = new QAction(tr("Print..."), this);
    m_pActionPrint->setStatusTip(tr("Prints the current view."));
    m_pActionPrint->setShortcut(QKeySequence::Print);
    connect(m_pActionPrint, &QAction::triggered, this->m_pMdiView, &MdiView::printCurrentSubWindow);

    //Help QMenu
    m_pActionAbout = new QAction(tr("About"), this);
    m_pActionAbout->setStatusTip(tr("Show the application's About box"));
    connect(m_pActionAbout, &QAction::triggered, this, &MainWindow::about);
}


//*************************************************************************************************************

void MainWindow::createMenus(QSharedPointer<ANSHAREDLIB::ExtensionManager> pExtensionManager)
{
    m_pMenuFile = menuBar()->addMenu(tr("File"));
    m_pMenuFile->addAction(m_pActionExit);

    m_pMenuView = menuBar()->addMenu(tr("View"));
    m_pMenuView->addAction(m_pActionCascade);
    m_pMenuView->addAction(m_pActionTile);
    m_pMenuView->addSeparator();
    m_pMenuView->addAction(m_pActionPrint);

    menuBar()->addSeparator();

    m_pMenuHelp = menuBar()->addMenu(tr("Help"));
    m_pMenuHelp->addAction(m_pActionAbout);

    // add extensions menus
    for(IExtension* ex : pExtensionManager->getExtensions())
    {
        QMenu* pMenu = ex->getMenu();
        if(pMenu)
        {
            menuBar()->addMenu(pMenu);
        }
    }
}


//*************************************************************************************************************

void MainWindow::createDockWindows(QSharedPointer<ANSHAREDLIB::ExtensionManager> pExtensionManager)
{
    setTabPosition(Qt::LeftDockWidgetArea,QTabWidget::West);
    setTabPosition(Qt::RightDockWidgetArea,QTabWidget::East);
    setDockOptions(QMainWindow::ForceTabbedDocks);

    //Add Extension views to mdi
    for(IExtension* pExtension : pExtensionManager->getExtensions()) {
        QDockWidget* pControl = pExtension->getControl();
        if(pControl) {
            addDockWidget(Qt::LeftDockWidgetArea, pControl);
            qDebug() << "[MainWindow::createDockWindows] Found and added dock widget for " << pExtension->getName();
        }
    }

    tabifyDockWindows();
}


//*************************************************************************************************************

void MainWindow::createMdiView(QSharedPointer<ExtensionManager> pExtensionManager)
{
    m_pGridLayout = new QGridLayout(this);
    m_pMdiView = new MdiView(this);
    m_pGridLayout->addWidget(m_pMdiView);
    setCentralWidget(m_pMdiView);

    //Add Extension views to mdi
    for(IExtension* pExtension : pExtensionManager->getExtensions()) {
        QWidget* pView = pExtension->getView();
        if(pView) {
            m_pMdiView->addSubWindow(pView);
            qDebug() << "[MainWindow::createMdiView] Found and added subwindow for " << pExtension->getName();
        }
    }

    m_pMdiView->cascadeSubWindows();
}


//*************************************************************************************************************

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
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt; font-weight:600;\">Copyright (C) 2017 Christoph Dinh, Lorenz Esch, Lars Debor, Simon Heinke, Matti Hamalainen. All rights reserved.</span></p>\n"
                                          "<p align=\"justify\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-size:8pt;\"><br /></p>\n"
                                          "<p align=\"justify\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">For more information visit the MNE-CPP/MNE Analyze project on its homepage:</span></p>\n"
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
