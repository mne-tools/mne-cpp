//=============================================================================================================
/**
* @file		progressbarwidget.cpp
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the implementation of the ProgressBarWidget class.
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
