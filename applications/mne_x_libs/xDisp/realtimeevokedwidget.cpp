//=============================================================================================================
/**
* @file     realtimeevokedwidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
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
* @brief    Implementation of the RealTimeEvokedWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeevokedwidget.h"
//#include "annotationwindow.h"

#include <xMeas/realtimeevoked.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>

#include <QScroller>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;
using namespace XMEASLIB;


//=============================================================================================================
/**
* Tool enumeration.
*/
enum Tool
{
    Freeze     = 0,     /**< Freezing tool. */
    Annotation = 1      /**< Annotation tool. */
};


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeEvokedWidget::RealTimeEvokedWidget(QSharedPointer<RealTimeEvoked> pRTE, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTEModel(NULL)
, m_pButterflyPlot(NULL)
, m_fZoomFactor(1.0f)
, m_pRTE(pRTE)
, m_bInitialized(false)
, m_pSensorModel(NULL)
{
    Q_UNUSED(pTime)

    m_pDoubleSpinBoxZoom = new QDoubleSpinBox(this);
    m_pDoubleSpinBoxZoom->setMinimum(0.3);
    m_pDoubleSpinBoxZoom->setMaximum(4.0);
    m_pDoubleSpinBoxZoom->setSingleStep(0.1);
    m_pDoubleSpinBoxZoom->setValue(1.0);
    m_pDoubleSpinBoxZoom->setSuffix(" x");
    connect(m_pDoubleSpinBoxZoom, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RealTimeEvokedWidget::zoomChanged);
    addDisplayWidget(m_pDoubleSpinBoxZoom);

//    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Shows the region selection widget (F12)"),this);
//    m_pActionSelectSensors->setShortcut(tr("F12"));
//    m_pActionSelectSensors->setStatusTip(tr("Shows the region selection widget (F12)"));
//    m_pActionSelectSensors->setVisible(false);
//    connect(m_pActionSelectSensors, &QAction::triggered, this, &RealTimeEvokedWidget::showSensorSelectionWidget);
//    addDisplayAction(m_pActionSelectSensors);

    if(m_pButterflyPlot)
        delete m_pButterflyPlot;
    m_pButterflyPlot = new RealTimeButterflyPlot;

    //set vertical layout
    QVBoxLayout *rteLayout = new QVBoxLayout(this);

    rteLayout->addWidget(m_pButterflyPlot);

    //set layouts
    this->setLayout(rteLayout);

    getData();
}


//*************************************************************************************************************

RealTimeEvokedWidget::~RealTimeEvokedWidget()
{

}


//*************************************************************************************************************

void RealTimeEvokedWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::getData()
{
    if(!m_bInitialized)
    {
        if(m_pRTE->isChInit())
        {
            m_qListChInfo = m_pRTE->chInfo();

//            QFile file(m_pRTE->getXMLLayoutFile());
//            if (!file.open(QFile::ReadOnly | QFile::Text))
//            {
//                qDebug() << QString("Cannot read file %1:\n%2.").arg(m_pRTE->getXMLLayoutFile()).arg(file.errorString());
//                m_pSensorModel = new SensorModel(this);
//                m_pSensorModel->mapChannelInfo(m_qListChInfo);
//            }
//            else
//            {
//                m_pSensorModel = new SensorModel(&file, this);
//                m_pSensorModel->mapChannelInfo(m_qListChInfo);
//                m_pActionSelectSensors->setVisible(true);
//            }

            init();
        }
        if(m_pRTE->containsValues())
            m_pRTEModel->updateData();
    }
    else
        m_pRTEModel->updateData();
}

//*************************************************************************************************************

void RealTimeEvokedWidget::init()
{
    if(m_qListChInfo.size() > 0)
    {
        if(m_pRTEModel)
            delete m_pRTEModel;
        m_pRTEModel = new RealTimeEvokedModel(this);

        m_pRTEModel->setRTE(m_pRTE);

        m_pButterflyPlot->setModel(m_pRTEModel);

        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::zoomChanged(double zoomFac)
{
    m_fZoomFactor = zoomFac;
}


//*************************************************************************************************************

void RealTimeEvokedWidget::showSensorSelectionWidget()
{
    if(!m_pSensorSelectionWidget)
    {
        m_pSensorSelectionWidget = QSharedPointer<SensorWidget>(new SensorWidget);

        m_pSensorSelectionWidget->setWindowTitle("Channel Selection");

        if(m_pSensorModel)
        {
            m_pSensorSelectionWidget->setModel(m_pSensorModel);

//            connect(m_pSensorModel, &SensorModel::newSelection, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::selectRows);
        }

    }
    m_pSensorSelectionWidget->show();
}


//*************************************************************************************************************

void RealTimeEvokedWidget::applySelection()
{
//    m_pRTMSAModel->selectRows(m_qListCurrentSelection);

    m_pSensorModel->silentUpdateSelection(m_qListCurrentSelection);
}


//*************************************************************************************************************

void RealTimeEvokedWidget::resetSelection()
{
    // non C++11 alternative
    m_qListCurrentSelection.clear();
    for(qint32 i = 0; i < m_qListChInfo.size(); ++i)
        m_qListCurrentSelection.append(i);

    applySelection();
}

