//=============================================================================================================
/**
* @file     quickcontrolview.cpp
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
* @brief    Definition of the QuickControlView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_quickcontrolview.h"

#include "quickcontrolview.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGroupBox>
#include <QTabWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QuickControlView::QuickControlView(const QString& name,
                                   Qt::WindowFlags flags,
                                   QWidget *parent,
                                   bool bDraggable)
: DraggableFramelessWidget(parent, flags, false, bDraggable)
, ui(new Ui::QuickControlViewWidget)
, m_sName(name)
{
    ui->setupUi(this);

    //Init opacity slider
    connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &QuickControlView::onOpacityChange);

    //Init and connect hide all group (minimize) button
    connect(ui->m_pushButton_hideAll, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlView::onToggleHideAll);

    connect(ui->m_pushButton_close, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &QuickControlView::hide);

    this->adjustSize();
}


//*************************************************************************************************************

QuickControlView::~QuickControlView()
{
    delete ui;
}


//*************************************************************************************************************

void QuickControlView::addWidget(QWidget* pWidget)
{
    ui->m_gridLayout_groupBoxes->addWidget(pWidget,
                                 ui->m_gridLayout_groupBoxes->rowCount(),
                                 0);
}


//*************************************************************************************************************

void QuickControlView::addGroupBox(QWidget* pWidget,
                                     QString sGroupBoxName)
{
    QGroupBox* pGroupBox = new QGroupBox(sGroupBoxName);
    pGroupBox->setObjectName(sGroupBoxName);

    QVBoxLayout *pVBox = new QVBoxLayout;

    pVBox->setContentsMargins(0,0,0,0);
    pVBox->addWidget(pWidget);
    pGroupBox->setLayout(pVBox);

    ui->m_gridLayout_groupBoxes->addWidget(pGroupBox,
                                 ui->m_gridLayout_groupBoxes->rowCount(),
                                 0);
}


//*************************************************************************************************************

void QuickControlView::addGroupBoxWithTabs(QWidget* pWidget,
                                             QString sGroupBoxName,
                                             QString sTabName)
{
    QGroupBox* pGroupBox = ui->m_widget_groupBoxes->findChild<QGroupBox *>(sGroupBoxName);

    if(!pGroupBox) {
        pGroupBox = new QGroupBox(sGroupBoxName);
        pGroupBox->setObjectName(sGroupBoxName);

        ui->m_gridLayout_groupBoxes->addWidget(pGroupBox,
                                     ui->m_gridLayout_groupBoxes->rowCount(),
                                     0);

        QVBoxLayout *pVBox = new QVBoxLayout;
        QTabWidget* pTabWidget = new QTabWidget;
        pTabWidget->setObjectName(sGroupBoxName + "TabWidget");

        pTabWidget->addTab(pWidget, sTabName);
        pVBox->setContentsMargins(4,2,4,4);
        pVBox->addWidget(pTabWidget);
        pGroupBox->setLayout(pVBox);
    } else {
        QTabWidget* pTabWidget = pGroupBox->findChild<QTabWidget *>(sGroupBoxName + "TabWidget");
        if(pTabWidget) {
            pTabWidget->addTab(pWidget, sTabName);
        }
    }
}


//*************************************************************************************************************

void QuickControlView::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}


//*************************************************************************************************************

void QuickControlView::setOpacityValue(int opactiy)
{
    ui->m_horizontalSlider_opacity->setValue(opactiy);

    onOpacityChange(opactiy);
}


//*************************************************************************************************************

int QuickControlView::getOpacityValue()
{
    return ui->m_horizontalSlider_opacity->value();
}


//*************************************************************************************************************

void QuickControlView::setVisiblityHideOpacityClose(bool bVisibility)
{
    ui->m_pushButton_close->setVisible(bVisibility);
    ui->m_pushButton_hideAll->setVisible(bVisibility);
    ui->m_horizontalSlider_opacity->setVisible(bVisibility);
    ui->m_label_opacity->setVisible(bVisibility);
}


//*************************************************************************************************************

void QuickControlView::onToggleHideAll(bool state)
{
    ui->m_widget_groupBoxes->setVisible(state);
    ui->m_widget_opacity->setVisible(state);
    this->adjustSize();
}

