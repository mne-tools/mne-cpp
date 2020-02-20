//=============================================================================================================
/**
 * @file     multiview.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
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
 * @brief    MultiView class definition.
 *
 */
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multiview.h"
#include "multiviewwindow.h"

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QHBoxLayout>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultiView::MultiView(QWidget *parent)
: QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;
    m_pSplitterHorizontal = new QSplitter(this);
    m_pSplitterHorizontal->setOrientation(Qt::Horizontal);
    m_pSplitterVertical = new QSplitter(this);
    m_pSplitterVertical->setOrientation(Qt::Vertical);
    m_pSplitterVertical->addWidget(m_pSplitterHorizontal);
    layout->addWidget(m_pSplitterVertical);

    this->setLayout(layout);
}

//*************************************************************************************************************

MultiView::~MultiView()
{
}

//*************************************************************************************************************

MultiViewWindow* MultiView::addWidgetH(QWidget* pWidget,
                                       const QString& sName)
{
    MultiViewWindow* pDockWidget = new MultiViewWindow();
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);
    this->m_pSplitterHorizontal->addWidget(pDockWidget);

    return pDockWidget;
}

//*************************************************************************************************************

MultiViewWindow* MultiView::addWidgetV(QWidget* pWidget,
                                       const QString& sName)
{
    MultiViewWindow* pDockWidget = new MultiViewWindow();
    pDockWidget->setWindowTitle(sName);
    pDockWidget->setWidget(pWidget);
    this->m_pSplitterVertical->addWidget(pDockWidget);

    return pDockWidget;
}
