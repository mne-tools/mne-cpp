//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     multiview.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February 2020
 * @brief    Implementation of the MultiView dock-hosting QMainWindow container.
 */
// INCLUDES
//=============================================================================================================

#include "multiview.h"
#include "multiviewwindow.h"
#include "rtfiffrawview.h"
#include "averagelayoutview.h"
#include "butterflyview.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QHBoxLayout>
#include <QDebug>
#include <QSettings>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiView::MultiView(const QString& sSettingsPath,
                     QWidget *parent,
                     Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{
    m_sSettingsPath = sSettingsPath;
    this->setDockNestingEnabled(true);
    if(QWidget* pCentralWidget = this->centralWidget()) {
        pCentralWidget->hide();
    }
}

//=============================================================================================================

MultiView::~MultiView()
{
}

//=============================================================================================================

MultiViewWindow* MultiView::addWidgetTop(QWidget* pWidget,
                                         const QString& sName)
{
    MultiViewWindow* pDockWidget = new MultiViewWindow(this);
    pDockWidget->setObjectName(sName);
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);

    // Disable floating and editable dock widgets, since the wasm QDockWidget version is buggy
    #ifdef WASMBUILD
    pDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    #endif

    if(pWidget->layout() && pDockWidget->layout()){
        pWidget->layout()->setContentsMargins(0,0,0,0);
        pDockWidget->layout()->setContentsMargins(0,0,0,0);
    }

    this->addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, pDockWidget);

    connect(pDockWidget, &MultiViewWindow::dockLocationChanged, [=, this]() {
        emit dockLocationChanged(pWidget);
    });

    return pDockWidget;
}

//=============================================================================================================

MultiViewWindow* MultiView::addWidgetBottom(QWidget* pWidget,
                                            const QString& sName)
{
    MultiViewWindow* pDockWidget = new MultiViewWindow(this);
    pDockWidget->setObjectName(sName);
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);

    pWidget->setParent(pDockWidget);

    // Disable floating and editable dock widgets, since the wasm QDockWidget version is buggy
    #ifdef WASMBUILD
    pDockWidget->setFeatures(QDockWidget::DockWidgetClosable);
    #endif

    if(pWidget->layout() && pDockWidget->layout()){
        pWidget->layout()->setContentsMargins(0,0,0,0);
        pDockWidget->layout()->setContentsMargins(0,0,0,0);
    }

    if(m_lDockWidgets.isEmpty()) {
        this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pDockWidget);
    }

    for(int i = 0; i < m_lDockWidgets.size(); ++i) {
        this->tabifyDockWidget(m_lDockWidgets.at(i), pDockWidget);
    }

    m_lDockWidgets.append(pDockWidget);

    connect(pDockWidget, &MultiViewWindow::dockLocationChanged, [=, this]() {
        emit dockLocationChanged(pWidget);
    });

    return pDockWidget;
}

//=============================================================================================================

void MultiView::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        qWarning() << "[MultiView::saveSettings] Settings path not set for main window. Cannot save central widget state.";
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/MultiView"));
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
}

//=============================================================================================================

void MultiView::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        qWarning() << "[MultiView::loadSettings] Settings path not set for main window. Cannot load central widget state.";
        return;
    }

    QSettings settings("MNECPP");

    settings.beginGroup(m_sSettingsPath + QString("/MultiView"));
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}
