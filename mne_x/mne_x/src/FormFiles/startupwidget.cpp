//=============================================================================================================
/**
* @file		startupwidget.cpp
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
* @brief	Contains implementation of StartUpWidget class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "startupwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtGui>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StartUpWidget::StartUpWidget(QWidget *parent)
    : QWidget(parent)
{

    QWidget *topFiller = new QWidget;
    topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pLabel_Info = new QLabel(tr("CSA Real-Time - Clinical Sensing and Analysis"));
    m_pLabel_Info->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_pLabel_Info->setAlignment(Qt::AlignCenter);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(5);
    layout->addWidget(topFiller);
    layout->addWidget(m_pLabel_Info);
    layout->addWidget(bottomFiller);

    this->setLayout(layout);
}


//*************************************************************************************************************

StartUpWidget::~StartUpWidget()
{
    qDebug() << "StartUp destroyed automatically.";
}
