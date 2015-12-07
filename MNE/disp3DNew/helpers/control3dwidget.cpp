//=============================================================================================================
/**
* @file     control3dwidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the Control3DWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "control3dwidget.h"
#include "ui_control3dwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Control3DWidget::Control3DWidget(QWidget *parent)
: RoundedEdgesWidget(parent)
, ui(new Ui::Control3DWidget)
{
    ui->setupUi(this);

    //Do connect for internal widget use (non dependent on a view3D)
    connect(ui->m_pushButton_minimize, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &Control3DWidget::onMinimizeWidget);

    connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &Control3DWidget::onOpacityChange);

    this->setWindowFlags(Qt::WindowStaysOnTopHint);
    this->adjustSize();
    this->setWindowOpacity(1/(100.0/90.0));

    //Rename minimize button
    ui->m_pushButton_minimize->setText(QString("Minimize - %1").arg(this->windowTitle()));
}


//*************************************************************************************************************

Control3DWidget::~Control3DWidget()
{
    delete ui;
}


//*************************************************************************************************************

void Control3DWidget::setView3D(View3D::SPtr view3D)
{
    //Do the connects from this control widget to the View3D

    //Add the view3D to the list of connected view3D's
    m_lView3D.append(view3D);
}


//*************************************************************************************************************

void Control3DWidget::onMinimizeWidget(bool state)
{
    if(!state) {
        ui->m_toolBox->hide();
        ui->m_pushButton_minimize->setText(QString("Maximize - %1").arg(this->windowTitle()));
    }
    else {
        ui->m_toolBox->show();
        ui->m_pushButton_minimize->setText(QString("Minimize - %1").arg(this->windowTitle()));
    }

    this->adjustSize();
    this->resize(width(), ui->m_pushButton_minimize->height());
}


//*************************************************************************************************************

void Control3DWidget::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}
