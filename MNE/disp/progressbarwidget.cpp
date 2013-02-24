//=============================================================================================================
/**
* @file     progressbarwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the ProgressBarWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "progressbarwidget.h"
#include <rtMeas/Measurement/progressbar.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintEvent>
#include <QPainter>
#include <QTimer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace RTMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ProgressBarWidget::ProgressBarWidget(ProgressBar* pProgressBar, QWidget *parent)
: MeasurementWidget(parent)
, m_pProgressBar(pProgressBar)
, m_dSegmentSize(0.0)

{
    ui.setupUi(this);
    m_usXPos = ui.m_qFrame->geometry().x();

    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000);

    m_Brush.setStyle(Qt::SolidPattern);
    m_Font.setBold(true);
    m_Font.setPointSizeF(28);
}


//*************************************************************************************************************

ProgressBarWidget::~ProgressBarWidget()
{

}


//*************************************************************************************************************

void ProgressBarWidget::update(Subject*)
{
    m_usXPos = (unsigned short)(ui.m_qFrame->geometry().x()+m_dSegmentSize*m_pProgressBar->getValue());
    m_Text = QString::number(m_pProgressBar->getValue()/10.0f);

    if((m_pProgressBar->getValue() >= 0) && (m_pProgressBar->getValue() <= 40))
        m_Brush.setColor(Qt::green);

    else if((m_pProgressBar->getValue() > 40) && (m_pProgressBar->getValue() <= 80))
        m_Brush.setColor(Qt::yellow);

    else
        m_Brush.setColor(Qt::red);
}


//*************************************************************************************************************

void ProgressBarWidget::init()
{
    ui.m_qLabel_Caption->setText(m_pProgressBar->getName());
    m_dSegmentSize = static_cast<double>(ui.m_qFrame->width())/(m_pProgressBar->getMaxScale()-m_pProgressBar->getMinScale());
}


//*************************************************************************************************************

void ProgressBarWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(ui.m_qFrame->geometry().x(), ui.m_qFrame->geometry().y(), m_usXPos, ui.m_qFrame->geometry().height(), m_Brush);
    painter.setFont(m_Font);
    painter.drawText(ui.m_qFrame->geometry(), Qt::AlignCenter, m_Text);
}
