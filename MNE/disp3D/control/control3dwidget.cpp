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

#include "../model/data3Dtreedelegate.h"
#include "../model/data3Dtreemodel.h"
#include "../view3D.h"


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

Control3DWidget::Control3DWidget(QWidget* parent, const QStringList& slFlags, Qt::WindowType type)
: QWidget(parent, type)/*RoundedEdgesWidget(parent, type)*/
, ui(new Ui::Control3DWidget)
, m_colCurrentSceneColor(QColor(0,0,0))
, m_colCurrentLightColor(QColor(255,255,255))
{
    ui->setupUi(this);

    //Parse flags
    if(slFlags.contains("Minimize")) {
        ui->m_pushButton_minimize->show();
        connect(ui->m_pushButton_minimize, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
             this, &Control3DWidget::onMinimizeWidget);
    } else {
        ui->m_pushButton_minimize->hide();
    }

    if(slFlags.contains("Data")) {
        ui->m_treeView_loadedData->show();
    } else {
        ui->m_treeView_loadedData->hide();
    }

    if(slFlags.contains("Window")) {
        ui->m_groupBox_windowOptions->show();
        connect(ui->m_horizontalSlider_opacity, &QSlider::valueChanged,
                this, &Control3DWidget::onOpacityChange);
    } else {
        ui->m_groupBox_windowOptions->hide();
    }

    if(slFlags.contains("View")) {
        ui->m_groupBox_viewOptions->show();

        connect(ui->m_pushButton_sceneColorPicker, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &Control3DWidget::onSceneColorPicker);
        connect(ui->m_checkBox_alwaysOnTop, static_cast<void (QCheckBox::*)(bool)>(&QCheckBox::clicked),
                this, &Control3DWidget::onAlwaysOnTop);
        connect(ui->m_checkBox_showFullScreen, &QCheckBox::clicked,
                this, &Control3DWidget::onShowFullScreen);

        connect(ui->m_checkBox_rotate, &QCheckBox::clicked,
                this, &Control3DWidget::onRotationClicked);

        connect(ui->m_checkBox_coordAxis, &QCheckBox::clicked,
                this, &Control3DWidget::onCoordAxisClicked);
    } else {
        ui->m_groupBox_viewOptions->hide();
    }

    if(slFlags.contains("Light")) {
        ui->m_groupBox_lightOptions->show();

        connect(ui->m_pushButton_lightColorPicker, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked),
                this, &Control3DWidget::onLightColorPicker);
        connect(ui->m_doubleSpinBox_colorIntensity, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &Control3DWidget::onLightIntensityChanged);
    } else {
        ui->m_groupBox_lightOptions->hide();
    }


    //Init's
    ui->m_pushButton_sceneColorPicker->setStyleSheet(QString("background-color: rgb(0, 0, 0);"));
    ui->m_pushButton_lightColorPicker->setStyleSheet(QString("background-color: rgb(255, 255, 255);"));

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

void Control3DWidget::init(QSharedPointer<Data3DTreeModel> pData3DTreeModel, QSharedPointer<View3D> pView3D)
{
    //Do the connects from this control widget to the View3D
    ui->m_treeView_loadedData->setModel(pData3DTreeModel.data());

    //Do the connects
    connect(this, &Control3DWidget::sceneColorChanged,
            pView3D.data(), &View3D::setSceneColor);

    connect(this, &Control3DWidget::rotationChanged,
            pView3D.data(), &View3D::startStopModelRotation);

    connect(this, &Control3DWidget::showCoordAxis,
            pView3D.data(), &View3D::toggleCoordAxis);

    connect(this, &Control3DWidget::showFullScreen,
            pView3D.data(), &View3D::showFullScreen);

    connect(this, &Control3DWidget::lightColorChanged,
            pView3D.data(), &View3D::setLightColor);

    connect(this, &Control3DWidget::lightIntensityChanged,
            pView3D.data(), &View3D::setLightIntensity);

    //Set description hidden as default
    this->onTreeViewDescriptionHide();
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
    connect(pDialog, &QColorDialog::currentColorChanged,
            this, &Control3DWidget::onSceneColorChanged);

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

void Control3DWidget::onSceneColorChanged(const QColor& color)
{
    emit sceneColorChanged(color);
}


//*************************************************************************************************************

void Control3DWidget::onShowFullScreen(bool checked)
{
    emit showFullScreen(checked);
}


//*************************************************************************************************************

void Control3DWidget::onRotationClicked(bool checked)
{
    emit rotationChanged(checked);
}


//*************************************************************************************************************

void Control3DWidget::onCoordAxisClicked(bool checked)
{
    emit showCoordAxis(checked);
}


//*************************************************************************************************************

void Control3DWidget::onLightColorPicker()
{
    QColorDialog* pDialog = new QColorDialog(this);
    pDialog->setCurrentColor(m_colCurrentLightColor);

    //Update all connected View3D's scene colors
    connect(pDialog, &QColorDialog::currentColorChanged,
            this, &Control3DWidget::onLightColorChanged);

    pDialog->exec();
    m_colCurrentLightColor = pDialog->currentColor();

    //Set color of button new new scene color
    ui->m_pushButton_lightColorPicker->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(m_colCurrentLightColor.red()).arg(m_colCurrentLightColor.green()).arg(m_colCurrentLightColor.blue()));
}


//*************************************************************************************************************

void Control3DWidget::onLightColorChanged(const QColor &color)
{
    emit lightColorChanged(color);
}


//*************************************************************************************************************

void Control3DWidget::onLightIntensityChanged(double value)
{
    emit lightIntensityChanged(value);
}

