//=============================================================================================================
/**
* @file		runwidget.cpp
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
* @brief	Contains implementation of RunWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "runwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QScrollArea>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;

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


//*************************************************************************************************************

RunWidget::~RunWidget()
{
    qDebug() << "RunCSART destroyed automatically.";
}


//*************************************************************************************************************

int RunWidget::addTab(QWidget* page, const QString& label)
{
    return m_pTabWidgetMain->addTab(page, label);
}


//*************************************************************************************************************

void RunWidget::setStandardZoom()
{
    m_pScrollArea->setWidgetResizable(true);
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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


//*************************************************************************************************************

void RunWidget::closeEvent(QCloseEvent* )
{
    emit displayClosed();
}
