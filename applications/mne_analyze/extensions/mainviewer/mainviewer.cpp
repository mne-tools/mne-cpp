//=============================================================================================================
/**
* @file     mainviewer.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    MainViewer class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainviewer.h"
#include "../../libs/anShared/Utils/metatypes.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MAINVIEWEREXTENSION;
using namespace ANSHAREDLIB;
using namespace Qt3DCore;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainViewer::MainViewer()
    : m_pModel(Q_NULLPTR),
      m_pDock(Q_NULLPTR),
      m_pMainViewerControl(Q_NULLPTR),
      m_pView(Q_NULLPTR),
      m_pContainer(Q_NULLPTR),
      m_pSubWindow(Q_NULLPTR),
      m_bDisplayCreated(false)
{

}


//*************************************************************************************************************

MainViewer::~MainViewer()
{
    delete m_pMainViewerControl;
}


//*************************************************************************************************************

QSharedPointer<IExtension> MainViewer::clone() const
{
    // cloning the main viewer might cause a lot of trouble, so let us not even try to
    return Q_NULLPTR;
}


//*************************************************************************************************************

void MainViewer::init()
{
    if(m_bDisplayCreated == false) {
        createDisplay();
    }

    // create our QEntity model and register it in AnalyzeData
    m_pModel = m_pAnalyzeData->createQEntityListModel(QString("MAINVIEWER"));
    // do the necessary connects so that we get notified when new 3D stuff needs to be added / removed
    QObject::connect(m_pModel.data(), &QEntityListModel::entityTreeAdded, this, &MainViewer::onEntityTreeAdded);
    // direct connection in case somebody directly deletes the entity tree after calling "removeEntityTree".
    QObject::connect(m_pModel.data(), &QEntityListModel::entityTreeRemoved, this, &MainViewer::onEntityTreeRemoved);

    // build control widget
    if (! m_pMainViewerControl) {
        m_pMainViewerControl = new MainViewerControl;

        connect(m_pMainViewerControl,
                &MainViewerControl::visibilityChanged,
                this,
                &MainViewer::onVisibilityChanged);
    }
}


//*************************************************************************************************************

void MainViewer::unload()
{
    // we need to call this to prevent double frees during shutdown
    m_pView->shutdown();
}


//*************************************************************************************************************

QString MainViewer::getName() const
{
    return "MainViewer";
}


//*************************************************************************************************************

QMenu *MainViewer::getMenu()
{
    /*
    if(!m_pMenu)
    {
        m_pMenu = new QMenu(getName().toStdString().c_str());
        // Action for toggling visibility
        m_pToggleVisibility = new QAction(tr((std::string("Show ") + getName().toStdString()).c_str()));
        m_pToggleVisibility->setCheckable(true);
        // we assume that mainviewer is visible on start
        m_pToggleVisibility->setChecked(true);
        m_pToggleVisibility->setStatusTip(tr(QString("Toggle " + getName() + " visibility").toStdString().c_str()));
        connect(m_pToggleVisibility, &QAction::triggered, this, &MainViewer::toggleVisibility);
        m_pMenu->addAction(m_pToggleVisibility);
    }

    return m_pMenu;
    */
    return Q_NULLPTR;
}


//*************************************************************************************************************

QDockWidget *MainViewer::getControl()
{
    if(!m_pDock) {
        m_pDock = new QDockWidget(tr("MainViewer"));
        m_pDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

        if (m_pMainViewerControl) {
            m_pDock->setWidget(m_pMainViewerControl);
        }
    }

    return m_pDock;
}


//*************************************************************************************************************

QWidget *MainViewer::getView()
{
    if (m_bDisplayCreated == false)
    {
        createDisplay();
    }

    return m_pSubWindow;
}


//*************************************************************************************************************

void MainViewer::handleEvent(QSharedPointer<Event> e)
{
    Q_UNUSED(e);
}


//*************************************************************************************************************

QVector<EVENT_TYPE> MainViewer::getEventSubscriptions(void) const
{
    return QVector<EVENT_TYPE>();
}


//*************************************************************************************************************

void MainViewer::onEntityTreeAdded(QSharedPointer<QEntity> pEntity)
{
    // simply pass on to view
    m_pView->addEntity(pEntity);
    m_pContainer->update();
}


//*************************************************************************************************************

void MainViewer::onEntityTreeRemoved(QSharedPointer<QEntity> pEntity)
{
    // simply pass on to view:
    m_pView->removeEntity(pEntity);
    m_pContainer->update();
}


//*************************************************************************************************************

void MainViewer::onVisibilityChanged(bool visible)
{
    if(visible) {
        // window should become visible
        if(m_pSubWindow->isVisible() == false) {
            show();
        }
    }
    else {
        // window should become invisible, depending on consistency of window visibility and checked-ness of
        // the toggleVisibility component in the MainViewers Dock Widget
        if(m_pSubWindow->isVisible()) {
            // normal case, simply hide
            hide();
        }
        else {
            // user has probably clicked 'x'-button of MainViewers window. This leads to a minor inconsistency
            // in checked-ness of the respective GUI-component. In order to restore consistency, we ignore this case
        }
    }
}


//*************************************************************************************************************

void MainViewer::hide()
{
    m_pSubWindow->hide();
}


//*************************************************************************************************************

void MainViewer::show()
{
    m_pSubWindow->show();
    // container state is not considered when this is run through a QtConnection, we need to call it explicitly
    m_pContainer->show();
}


//*************************************************************************************************************

void MainViewer::createDisplay()
{
    // a 3D window is created
    m_pView = new CentralView();

    // a container is created to contain the Qt3DWindow, then a minimum size is set
    m_pContainer = QWidget::createWindowContainer(m_pView);
    m_pContainer->setMinimumSize(256, 256);
    m_pContainer->setFocusPolicy(Qt::TabFocus);
    m_pContainer->setAttribute(Qt::WA_DeleteOnClose, false);

    // we need this additional window since the top-level main window runs "QMdiView::addSubWindow()", which requires
    // a subwindow to be passed (if a non-window widget would be passed, QMdiView would silently create a new QMidSubWindow )
    m_pSubWindow = new QMdiSubWindow();
    m_pSubWindow->setWidget(m_pContainer);
    m_pSubWindow->setWindowTitle(QString("Main Display"));
    m_pSubWindow->setAttribute(Qt::WA_DeleteOnClose, false);

    // let Qt know that we want the MainViewer maximized
    m_pSubWindow->showMaximized();

    // remember that the display was built
    m_bDisplayCreated = true;
}
