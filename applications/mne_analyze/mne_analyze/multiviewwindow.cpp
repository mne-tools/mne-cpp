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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiviewwindow.h"
#include "mdiview.h"
#include "formfiles/ui_multiviewwindow.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QListView>
#include <QDockWidget>
#include <QDebug>
#include <QCloseEvent>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEANALYZE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiViewWindow::MultiViewWindow(QWidget *parent, QWidget *centralWidget, const QString &sName)
: QWidget(parent)
, m_pUi(new Ui::MultiViewWindowWidget)
{
    m_pUi->setupUi(this);
    m_pUi->m_pLabelName->setText(sName);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
    centralWidget->setSizePolicy(sizePolicy);
    m_pUi->gridLayout->addWidget(centralWidget, 1, 0, 1, 4);

    //this->setWindowFlag(Qt::FramelessWindowHint, true);
    this->setWindowFlags(Qt::WindowMinMaxButtonsHint);
    connect(m_pUi->m_pPushButtonMax, &QPushButton::clicked,
            this, &MultiViewWindow::maximize);
}


//*************************************************************************************************************

MultiViewWindow::~MultiViewWindow()
{

}


//*************************************************************************************************************

void MultiViewWindow::maximize()
{
    if(!this->isWindow()) {
        oldparent = this->parentWidget();
        this->setParent(Q_NULLPTR);
        this->setWindowFlag(Qt::Window, true);
        m_pUi->m_pPushButtonMax->setText("Min.");
    } else if(oldparent) {
        this->setParent(oldparent);
        this->setWindowFlag(Qt::Window, false);
        m_pUi->m_pPushButtonMax->setText("Max.");

    }
    this->show();
}


//*************************************************************************************************************

void MultiViewWindow::closeEvent(QCloseEvent *event)
{
    if(oldparent) {
        //this->setParent(oldparent);
        this->setWindowFlag(Qt::Window, false);
        m_pUi->m_pPushButtonMax->show();
        m_pUi->m_pPushButtonClose->show();
        this->show();
    }

    event->ignore();
}
