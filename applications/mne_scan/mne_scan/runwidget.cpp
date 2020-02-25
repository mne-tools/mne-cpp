//=============================================================================================================
/**
 * @file     runwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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
 * @brief    Contains Definition of RunWidget class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "runwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

RunWidget::RunWidget(QWidget *dispWidget, QWidget *parent)
    : QWidget(parent)
{

    m_pScrollArea = new QScrollArea;
    //    m_pScrollArea->setBackgroundRole(QPalette::Base);
    m_pScrollArea->setWidget(dispWidget);

    m_pScrollArea->setWidgetResizable(true);

    m_pTabWidgetMain = new QTabWidget;

    m_pTabWidgetMain->addTab(m_pScrollArea, tr("Dis&play"));

    QVBoxLayout *pVBoxLayout = new QVBoxLayout;
    pVBoxLayout->addWidget(m_pTabWidgetMain);

    setLayout(pVBoxLayout);

    //setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}

//=============================================================================================================

RunWidget::~RunWidget()
{
//    qDebug() << "RunCSART destroyed automatically.";

//    if(m_pTabWidgetMain)
//        delete m_pTabWidgetMain;

//    if(m_pScrollArea)
//        delete m_pScrollArea;
}

//=============================================================================================================

int RunWidget::addTab(QWidget* page, const QString& label)
{
    return m_pTabWidgetMain->addTab(page, label);
}

//=============================================================================================================

void RunWidget::setStandardZoom()
{
    m_pScrollArea->setWidgetResizable(true);
}

//=============================================================================================================

void RunWidget::zoomVert(float factor)
{
    m_pScrollArea->setWidgetResizable(false);

    QSize size = m_pScrollArea->widget()->size();

    if(m_pScrollArea->size().height()>size.height()*factor)
        size.setWidth(m_pScrollArea->size().width()-2);
    else
        size.setWidth(m_pScrollArea->size().width()-20);

    m_pScrollArea->widget()->resize((int)(size.width()),(int)(size.height()*factor));

}

//=============================================================================================================

void RunWidget::resizeEvent(QResizeEvent* )
{
    if(!m_pScrollArea->widgetResizable())
    {
        QSize size = m_pScrollArea->widget()->size();

        if(m_pScrollArea->size().height()>size.height())
            size.setWidth(m_pScrollArea->size().width()-2);
        else
            size.setWidth(m_pScrollArea->size().width()-20);

        m_pScrollArea->widget()->resize(size.width(),size.height());
    }
}

//=============================================================================================================

void RunWidget::closeEvent(QCloseEvent* )
{
    emit displayClosed();
}
