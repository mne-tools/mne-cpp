//=============================================================================================================
/**
 * @file     multiviewwindow.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
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
 * @brief    MultiViewWindow class declaration.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewwindow.h"
#include "multiview.h"

//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QListView>
#include <QDockWidget>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiViewWindow::MultiViewWindow(QWidget *parent,
                                 Qt::WindowFlags flags)
: QDockWidget(parent, flags)
, m_bWindowMode(false)
{
    this->setWidget(parent);
//    m_pFloatingWindow = new QWidget(this->parentWidget(), Qt::Window);
//    QHBoxLayout *layout = new QHBoxLayout;
//    m_pFloatingWindow->setLayout(layout);
//    m_pFloatingWindow->setWindowFlags(Qt::WindowMinMaxButtonsHint);
//    m_pFloatingWindow->hide();

//    this->setFeatures(QDockWidget::DockWidgetFloatable);

//    connect(this, &QDockWidget::topLevelChanged,
//            this, &MultiViewWindow::onTopLevelChanged);
}

//=============================================================================================================

MultiViewWindow::~MultiViewWindow()
{

}

//=============================================================================================================

//void MultiViewWindow::onTopLevelChanged(bool bFlag)
//{
//    Q_UNUSED(bFlag)

//    if(!m_bWindowMode) {
//        m_pParent = this->parentWidget();
//        m_pFloatingWindow->layout()->addWidget(this);
//        this->setParent(m_pFloatingWindow);
//        m_pFloatingWindow->show();
//        m_bWindowMode = true;
//    } else if(m_pParent) {
//        m_pFloatingWindow->layout()->removeWidget(this);
//        this->setParent(m_pParent);
//        m_pFloatingWindow->hide();
//        m_bWindowMode = false;
//    }
//}
