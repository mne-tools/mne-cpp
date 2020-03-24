//=============================================================================================================
/**
 * @file     multiview.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    MultiView class definition.
 *
 */
//=============================================================================================================
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

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiView::MultiView(QWidget *parent,
                     Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{
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
    MultiViewWindow* pDockWidget = new MultiViewWindow();
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);
    pWidget->layout()->setContentsMargins(0,0,0,0);
    pDockWidget->layout()->setContentsMargins(0,0,0,0);

    this->addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, pDockWidget);

    connect(pDockWidget, &MultiViewWindow::dockLocationChanged, [=]() {
        emit dockLocationChanged(pWidget);
    });

    return pDockWidget;
}

//=============================================================================================================

MultiViewWindow* MultiView::addWidgetBottom(QWidget* pWidget,
                                            const QString& sName)
{
    MultiViewWindow* pDockWidget = new MultiViewWindow();
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);
    pWidget->layout()->setContentsMargins(0,0,0,0);
    pDockWidget->layout()->setContentsMargins(0,0,0,0);

    if(m_lDockWidgets.isEmpty()) {
        this->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, pDockWidget);
    }

    for(int i = 0; i < m_lDockWidgets.size(); ++i) {
        this->tabifyDockWidget(m_lDockWidgets.at(i), pDockWidget);
    }

    m_lDockWidgets.append(pDockWidget);

    connect(pDockWidget, &MultiViewWindow::dockLocationChanged, [=]() {
        emit dockLocationChanged(pWidget);
    });

    return pDockWidget;
}
