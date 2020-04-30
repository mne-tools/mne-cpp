//=============================================================================================================
/**
 * @file     tmsimanualannotationwidget.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March 2014*
 * @section  LICENSE
 *
 * Copyright (C) 014*, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the implementation of the TMSISetupWidget class.
 *
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
