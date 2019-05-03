//=============================================================================================================
/**
* @file     control3dview.cpp
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
* @brief    Definition of the Control3DView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_control3dview.h"
#include "control3dview.h"


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

#include <QItemDelegate>
#include <QStandardItemModel>
#include <QColorDialog>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Control3DView::Control3DView(QWidget* parent,
                             const QStringList& slFlags,
                             Qt::WindowType type)
: QWidget(parent, type)
, ui(new Ui::Control3DViewWidget)
, m_colCurrentSceneColor(QColor(0,0,0))
, m_colCurrentLightColor(QColor(255,255,255))
{
    ui->setupUi(this);

    setFlags(slFlags);

    //Init's
    ui->m_pushButton_sceneColorPicker->setStyleSheet(QString("background-color: rgb(0, 0, 0);"));
    ui->m_pushButton_lightColorPicker->setStyleSheet(QString("background-color: rgb(255, 255, 255);"));

    this->adjustSize();

    //set context menu
    ui->m_treeView_loadedData->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->m_treeView_loadedData, &QWidget::customContextMenuRequested,
            this, &Control3DView::onCustomContextMenuRequested);
}


//*************************************************************************************************************

void Control3DView::setFlags(const QStringList& slFlags)
{
    //Parse flags
    if(slFlags.contains("Data")) {
        ui->m_treeView_loadedData->show();
    } else {
        ui->m_treeView_loadedData->hide();
    }

    if(slFlags.contains("View")) {
        ui->m_groupBox_viewOptions->show();

        connect(ui->m_pushButton_sceneColorPicker, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &Control3DView::onSceneColorPicker);
        connect(ui->m_checkBox_showFullScreen, &QCheckBox::clicked,
                this, &Control3DView::onShowFullScreen);

        connect(ui->m_checkBox_rotate, &QCheckBox::clicked,
                this, &Control3DView::onRotationClicked);

        connect(ui->m_checkBox_coordAxis, &QCheckBox::clicked,
                this, &Control3DView::onCoordAxisClicked);

        connect(ui->m_pushButton_takeScreenshot, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &Control3DView::takeScreenshotChanged);
    } else {
        ui->m_groupBox_viewOptions->hide();
    }

    if(slFlags.contains("Light")) {
        ui->m_groupBox_lightOptions->show();

        connect(ui->m_pushButton_lightColorPicker, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &Control3DView::onLightColorPicker);
        connect(ui->m_doubleSpinBox_colorIntensity, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &Control3DView::onLightIntensityChanged);
    } else {
        ui->m_groupBox_lightOptions->hide();
    }
}


//*************************************************************************************************************

Control3DView::~Control3DView()
{
    delete ui;
}


//*************************************************************************************************************

void Control3DView::setDelegate(QItemDelegate* pItemDelegate)
{
    //Init tree view properties
    ui->m_treeView_loadedData->setItemDelegate(pItemDelegate);
    ui->m_treeView_loadedData->setHeaderHidden(false);
    ui->m_treeView_loadedData->setEditTriggers(QAbstractItemView::CurrentChanged);
}


//*************************************************************************************************************

void Control3DView::setModel(QStandardItemModel* pDataTreeModel)
{
    //Do the connects from this control widget to the View3D
    ui->m_treeView_loadedData->setModel(pDataTreeModel);

    //Set description hidden as default
    //ui->m_treeView_loadedData->setColumnHidden(1, true);
}


//*************************************************************************************************************

void Control3DView::onTreeViewHeaderHide()
{
    if(!ui->m_treeView_loadedData->isHeaderHidden()) {
        ui->m_treeView_loadedData->setHeaderHidden(true);
    } else {
        ui->m_treeView_loadedData->setHeaderHidden(false);
    }
}


//*************************************************************************************************************

void Control3DView::onTreeViewDescriptionHide()
{
    if(ui->m_treeView_loadedData->isColumnHidden(1)) {
        ui->m_treeView_loadedData->setColumnHidden(1, false);
    } else {
        ui->m_treeView_loadedData->setColumnHidden(1, true);
    }
}


//*************************************************************************************************************

void Control3DView::onOpacityChange(qint32 value)
{
    this->setWindowOpacity(1/(100.0/value));
}


//*************************************************************************************************************

void Control3DView::onSceneColorPicker()
{
    QColorDialog* pDialog = new QColorDialog(this);
    pDialog->setCurrentColor(m_colCurrentSceneColor);

    //Update all connected View3D's scene colors
    connect(pDialog, &QColorDialog::currentColorChanged,
            this, &Control3DView::onSceneColorChanged);

    pDialog->exec();
    m_colCurrentSceneColor = pDialog->currentColor();

    //Set color of button new new scene color
    ui->m_pushButton_sceneColorPicker->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentSceneColor.red()).arg(m_colCurrentSceneColor.green()).arg(m_colCurrentSceneColor.blue()));
}


//*************************************************************************************************************

void Control3DView::onCustomContextMenuRequested(QPoint pos)
{
    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //**************** Hide header ****************
    QAction* pHideHeader = menu->addAction(tr("Toggle header"));
    connect(pHideHeader, &QAction::triggered,
            this, &Control3DView::onTreeViewHeaderHide);

//    QAction* pHideDesc = menu->addAction(tr("Toggle description"));
//    connect(pHideDesc, &QAction::triggered,
//            this, &Control3DView::onTreeViewDescriptionHide);

    //show context menu
    menu->popup(ui->m_treeView_loadedData->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void Control3DView::onAlwaysOnTop(bool state)
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

void Control3DView::onSceneColorChanged(const QColor& color)
{
    emit sceneColorChanged(color);
}


//*************************************************************************************************************

void Control3DView::onShowFullScreen(bool checked)
{
    emit showFullScreen(checked);
}


//*************************************************************************************************************

void Control3DView::onRotationClicked(bool checked)
{
    emit rotationChanged(checked);
}


//*************************************************************************************************************

void Control3DView::onCoordAxisClicked(bool checked)
{
    emit showCoordAxis(checked);
}


//*************************************************************************************************************

void Control3DView::onLightColorPicker()
{
    QColorDialog* pDialog = new QColorDialog(this);
    pDialog->setCurrentColor(m_colCurrentLightColor);

    //Update all connected View3D's scene colors
    connect(pDialog, &QColorDialog::currentColorChanged,
            this, &Control3DView::onLightColorChanged);

    pDialog->exec();
    m_colCurrentLightColor = pDialog->currentColor();

    //Set color of button new new scene color
    ui->m_pushButton_lightColorPicker->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentLightColor.red()).arg(m_colCurrentLightColor.green()).arg(m_colCurrentLightColor.blue()));
}


//*************************************************************************************************************

void Control3DView::onLightColorChanged(const QColor &color)
{
    emit lightColorChanged(color);
}


//*************************************************************************************************************

void Control3DView::onLightIntensityChanged(double value)
{
    emit lightIntensityChanged(value);
}

