//=============================================================================================================
/**
* @file     sensorwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Implementation of the SensorWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorwidget.h"
#include "sensoritem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorWidget::SensorWidget(QWidget *parent, Qt::WindowFlags f)
: QWidget(parent, f)
, m_pSensorModel(NULL)
{
    m_pGraphicsView = new QGraphicsView(this);

    m_pGraphicsScene = new QGraphicsScene(this);

    m_pGraphicsView->setScene(m_pGraphicsScene);

    createUI();
}


//*************************************************************************************************************

void SensorWidget::contextUpdate(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles)
{
    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)
    contextUpdate();
}


//*************************************************************************************************************

void SensorWidget::contextUpdate()
{
    drawChannels();
}


//*************************************************************************************************************

void SensorWidget::createUI()
{
    if(m_pSensorModel)
    {
        // Sensor Selection
        QButtonGroup *qBGSensorSelection = new QButtonGroup;
        qBGSensorSelection->setExclusive(true);

        QVBoxLayout *VBoxSensorSelection = new QVBoxLayout;
        for(qint32 i = 0; i < m_pSensorModel->getSensorGroups().size(); ++i)
        {
            QToolButton *sensorSelectionButton = new QToolButton;
            sensorSelectionButton->setText(m_pSensorModel->getSensorGroups()[i].getGroupName());
            qBGSensorSelection->addButton(sensorSelectionButton,i);
            VBoxSensorSelection->addWidget(sensorSelectionButton);
        }

        connect(qBGSensorSelection, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), m_pSensorModel, &SensorModel::applySensorGroup);


        // Layout Selection
        QButtonGroup *qBGLayout = new QButtonGroup;
        qBGLayout->setExclusive(true);

        QHBoxLayout *HBoxButtonGroupLayout = new QHBoxLayout;

        for(qint32 i = 0; i < m_pSensorModel->getNumLayouts(); ++i)
        {
            QToolButton *buttonLayout = new QToolButton;
            buttonLayout->setText(m_pSensorModel->getSensorLayouts()[i].getName());
            buttonLayout->setCheckable(true);

            if(i == 0)
                buttonLayout->setChecked(true);
            else
                buttonLayout->setChecked(false);

            qBGLayout->addButton(buttonLayout, i);

            HBoxButtonGroupLayout->addWidget(buttonLayout);
        }

        connect(qBGLayout, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), m_pSensorModel, &SensorModel::setCurrentLayout);


        QGridLayout *topLayout = new QGridLayout;
        topLayout->addWidget(m_pGraphicsView, 0, 0);
        topLayout->addLayout(VBoxSensorSelection, 0, 1);
        topLayout->addLayout(HBoxButtonGroupLayout, 1, 0);

        setLayout(topLayout);
    }
}


//*************************************************************************************************************

void SensorWidget::setModel(SensorModel *model)
{
    m_pSensorModel = model;

    drawChannels();

    connect(m_pSensorModel, &QAbstractTableModel::dataChanged, this, static_cast<void (SensorWidget::*)(const QModelIndex &, const QModelIndex &, const QVector<int> &)>(&SensorWidget::contextUpdate));
    connect(m_pSensorModel, &QAbstractTableModel::modelReset, this, static_cast<void (SensorWidget::*)(void)>(&SensorWidget::contextUpdate));

    connect(m_pSensorModel, &SensorModel::newLayout, this, &SensorWidget::drawChannels);

    createUI();
}


//*************************************************************************************************************

void SensorWidget::drawChannels()
{
    if(m_pGraphicsScene)
    {
        m_pGraphicsScene->clear();

        for(qint32 i = 0; i < m_pSensorModel->rowCount(); ++i)
        {
            QString dispChName = m_pSensorModel->data(i, 0).toString();
            QString fullChName = m_pSensorModel->data(i, 1).toString();
            QPointF loc = m_pSensorModel->data(i, 2).toPointF();
            qint32 chNum = m_pSensorModel->getNameIdMap()[fullChName];
            SensorItem *item = new SensorItem(dispChName, chNum, loc, QSizeF(28, 16));
            item->setSelected(m_pSensorModel->data(i, 3).toBool());
            item->setPos(loc);

            connect(item, &SensorItem::itemChanged, m_pSensorModel, &SensorModel::updateChannelState);
            m_pGraphicsScene->addItem(item);
        }
    }
}
