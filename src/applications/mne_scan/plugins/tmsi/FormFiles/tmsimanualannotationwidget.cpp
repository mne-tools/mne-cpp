//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsimanualannotationwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2014*
 * Copyright (C) 014*, Christoph Dinh, Lorenz Esch. All rights reserved.
 * @brief    Contains the implementation of the TMSISetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsimanualannotationwidget.h"
#include "../tmsi.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIManualAnnotationWidget::TMSIManualAnnotationWidget(TMSI* pTMSI, QWidget *parent)
: QWidget(parent)
, m_pTMSI(pTMSI)
{
    ui.setupUi(this);
}

//=============================================================================================================

TMSIManualAnnotationWidget::~TMSIManualAnnotationWidget()
{
}

//=============================================================================================================

void TMSIManualAnnotationWidget::initGui()
{
}

//=============================================================================================================

void TMSIManualAnnotationWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Up:
            //std::cout<<"Up"<<endl;
            break;
        case Qt::Key_Down:
            //std::cout<<"Down"<<endl;
            break;
        case Qt::Key_Control://Qt::Key_Left:
            //std::cout<<"Left"<<endl;
            m_pTMSI->setKeyboardTriggerType(253);
            ui.m_pushButton_Left->click();//setStyleSheet("background-color: green");
            break;
        case Qt::Key_Enter://Qt::Key_Right:
            //std::cout<<"right"<<endl;
            m_pTMSI->setKeyboardTriggerType(254);
            ui.m_pushButton_Right->click();//->setStyleSheet("background-color: green");
            break;
        default:
            QWidget::keyPressEvent(event);
     }
}
