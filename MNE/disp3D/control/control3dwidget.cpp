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
#include "disp/helpers/roundededgeswidget.h"
#include "../3DObjects/data3Dtreedelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMenu>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "ui_control3dwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Control3DWidget::Control3DWidget(QWidget* parent, Qt::WindowType type)
: QWidget(parent, type)/*RoundedEdgesWidget(parent, type)*/
, ui(new Ui::Control3DWidget)
, m_colCurrentSceneColor(QColor(0,0,0))
{
    ui->setupUi(this);

    //Do connect for internal widget use (non dependent on a view3D)
    connect(ui->m_pushButton_minimize, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &Control3DWidget::onMinimizeWidget);

    connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
            this, &Control3DWidget::onOpacityChange);

    connect(ui->m_pushButton_sceneColorPicker, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
            this, &Control3DWidget::onSceneColorPicker);

    connect(ui->m_checkBox_alwaysOnTop, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
            this, &Control3DWidget::onAlwaysOnTop);

    //Connect animation and fullscreen and coord axis
    connect(ui->m_checkBox_showFullScreen, &QCheckBox::clicked,
            this, &Control3DWidget::onShowFullScreen);

    connect(ui->m_checkBox_rotate, &QCheckBox::clicked,
            this, &Control3DWidget::onRotationClicked);

    connect(ui->m_checkBox_coordAxis, &QCheckBox::clicked,
            this, &Control3DWidget::onCoordAxisClicked);

    //Init's
    ui->m_pushButton_sceneColorPicker->setStyleSheet(QString("background-color: rgb(0, 0, 0);"));

    this->adjustSize();
    this->setWindowOpacity(1/(100.0/90.0));

    //Rename minimize button
    ui->m_pushButton_minimize->setText(QString("Minimize - %1").arg(this->windowTitle()));

    //Init tree view properties
    Data3DTreeDelegate* pData3DTreeDelegate = new Data3DTreeDelegate(this);
    ui->m_treeView_loadedData->setItemDelegate(pData3DTreeDelegate);
    ui->m_treeView_loadedData->setHeaderHidden(false);
    ui->m_treeView_loadedData->setEditTriggers(QAbstractItemView::CurrentChanged);

    //set context menu
    ui->m_treeView_loadedData->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->m_treeView_loadedData, &QWidget::customContextMenuRequested,
            this, &Control3DWidget::onCustomContextMenuRequested);

    //Set on top as default
    onAlwaysOnTop(ui->m_checkBox_alwaysOnTop->isChecked());
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
    ui->m_treeView_loadedData->setModel(view3D->getData3DTreeModel());

    //Add the view3D to the list of connected view3D's
    m_lView3D.append(view3D);

    //Set description hidden as default
    this->onTreeViewDescriptionHide();
}


//*************************************************************************************************************

void Control3DWidget::onMinimizeWidget(bool state)
{
    if(!state) {
        ui->m_treeView_loadedData->hide();
        ui->m_groupBox_viewOptions->hide();
        ui->m_groupBox_windowOptions->hide();
        ui->m_pushButton_minimize->setText(QString("Maximize - %1").arg(this->windowTitle()));        
        this->resize(width(), ui->m_pushButton_minimize->height());
    }
    else {
        ui->m_treeView_loadedData->show();
        ui->m_groupBox_viewOptions->show();
        ui->m_groupBox_windowOptions->show();
        ui->m_pushButton_minimize->setText(QString("Minimize - %1").arg(this->windowTitle()));
    }

    this->adjustSize();
}


//*************************************************************************************************************

void Control3DWidget::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}


//*************************************************************************************************************

void Control3DWidget::onSceneColorPicker()
{
    QColorDialog* pDialog = new QColorDialog(this);
    pDialog->setCurrentColor(m_colCurrentSceneColor);

    //Update all connected View3D's scene colors
    for(int i = 0; i<m_lView3D.size(); i++) {
        connect(pDialog, &QColorDialog::currentColorChanged,
                m_lView3D.at(i).data(), &View3D::setSceneColor);
    }

    pDialog->exec();
    m_colCurrentSceneColor = pDialog->currentColor();

    //Set color of button new new scene color
    ui->m_pushButton_sceneColorPicker->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentSceneColor.red()).arg(m_colCurrentSceneColor.green()).arg(m_colCurrentSceneColor.blue()));
}


//*************************************************************************************************************

void Control3DWidget::onCustomContextMenuRequested(QPoint pos)
{
    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Hide header ****************
    QAction* pHideHeader = menu->addAction(tr("Toggle header"));
    connect(pHideHeader, &QAction::triggered,
            this, &Control3DWidget::onTreeViewHeaderHide);

    QAction* pHideDesc = menu->addAction(tr("Toggle description"));
    connect(pHideDesc, &QAction::triggered,
            this, &Control3DWidget::onTreeViewDescriptionHide);

    //show context menu
    menu->popup(ui->m_treeView_loadedData->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void Control3DWidget::onTreeViewHeaderHide()
{
    if(!ui->m_treeView_loadedData->isHeaderHidden()) {
        ui->m_treeView_loadedData->setHeaderHidden(true);
    } else {
        ui->m_treeView_loadedData->setHeaderHidden(false);
    }
}


//*************************************************************************************************************

void Control3DWidget::onTreeViewDescriptionHide()
{
    if(ui->m_treeView_loadedData->isColumnHidden(1)) {
        ui->m_treeView_loadedData->setColumnHidden(1, false);
    } else {
        ui->m_treeView_loadedData->setColumnHidden(1, true);
    }
}


//*************************************************************************************************************

void Control3DWidget::onAlwaysOnTop(bool state)
{
    if(state) {
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
        this->show();
    } else {
        this->setWindowFlags(this->windowFlags() & ~Qt::WindowStaysOnTopHint);
        this->show();
    }
}


//*************************************************************************************************************

void Control3DWidget::onShowFullScreen(bool checked)
{
    //Update all connected View3D's scene colors
    for(int i = 0; i < m_lView3D.size(); ++i) {
        if(checked) {
            m_lView3D.at(i)->showFullScreen();
        } else {
            m_lView3D.at(i)->showNormal();
        }
    }
}


//*************************************************************************************************************

void Control3DWidget::onRotationClicked(bool checked)
{
    //Update all connected View3D's scene colors
    for(int i = 0; i<m_lView3D.size(); i++) {
        if(checked) {
            m_lView3D.at(i)->startModelRotation();
        } else {
            m_lView3D.at(i)->stopModelRotation();
        }
    }
}


//*************************************************************************************************************

void Control3DWidget::onCoordAxisClicked(bool checked)
{
    //Update all connected View3D's scene colors
    for(int i = 0; i<m_lView3D.size(); i++) {
        m_lView3D.at(i)->toggleCoordAxis(checked);
    }
}


