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
, m_fSamplingRate(1024)
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

    m_pActionSelectSensors = new QAction(QIcon(":/images/selectSensors.png"), tr("Shows the region selection widget (F12)"),this);
    m_pActionSelectSensors->setShortcut(tr("F12"));
    m_pActionSelectSensors->setStatusTip(tr("Shows the region selection widget (F12)"));
    m_pActionSelectSensors->setVisible(false);
    connect(m_pActionSelectSensors, &QAction::triggered, this, &RealTimeEvokedWidget::showSensorSelectionWidget);
    addDisplayAction(m_pActionSelectSensors);

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
            m_qListChColors = m_pRTE->chColor();
            m_fSamplingRate = 0;//m_pRTE->getSamplingRate();

            QFile file(m_pRTE->getXMLLayoutFile());
            if (!file.open(QFile::ReadOnly | QFile::Text))
            {
                qDebug() << QString("Cannot read file %1:\n%2.").arg(m_pRTE->getXMLLayoutFile()).arg(file.errorString());
                m_pSensorModel = new SensorModel(this);
                m_pSensorModel->mapChannelInfo(m_qListChInfo);
            }
            else
            {
                m_pSensorModel = new SensorModel(&file, this);
                m_pSensorModel->mapChannelInfo(m_qListChInfo);
                m_pActionSelectSensors->setVisible(true);
            }

            init();
        }
        if(m_pRTE->containsValues())
            m_pRTEModel->addData(m_pRTE->getValue());
    }
    else
        m_pRTEModel->addData(m_pRTE->getValue());
}

//*************************************************************************************************************

void RealTimeEvokedWidget::init()
{
    if(m_qListChInfo.size() > 0)
    {
        if(m_pRTEModel)
            delete m_pRTEModel;
        m_pRTEModel = new RealTimeEvokedModel(this);

        m_pRTEModel->setChannelInfo(m_qListChInfo, m_qListChColors);
        m_pRTEModel->setSamplingInfo(m_fSamplingRate);

        m_pButterflyPlot->setModel(m_pRTEModel);




//        if(m_pRTMSADelegate)
//            delete m_pRTMSADelegate;
//        m_pRTMSADelegate = new RealTimeMultiSampleArrayDelegate(this);

//        connect(m_pTableView, &QTableView::doubleClicked, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::toggleFreeze);

//        //set some size settings for m_pTableView
//        m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

//        m_pTableView->setShowGrid(false);

//        m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
//        m_pTableView->horizontalHeader()->hide();
//        m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height

//        m_pTableView->setAutoScroll(false);
//        m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

//        m_pTableView->resizeColumnsToContents();

//        m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

//        //set context menu
//        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
//        connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(channelContextMenu(QPoint)));

//        //activate kinetic scrolling
//        QScroller::grabGesture(m_pTableView,QScroller::MiddleMouseButtonGesture);

        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeEvokedWidget::channelContextMenu(QPoint pos)
{
//    //obtain index where index was clicked
//    QModelIndex index = m_pTableView->indexAt(pos);

//    //get selected items
//    QModelIndexList selected = m_pTableView->selectionModel()->selectedIndexes();

////    // Lambda C++11 version
////    QVector<qint32> vecSelection;
////    for(qint32 i = 0; i < selected.size(); ++i)
////        if(selected[i].column() == 1)
////            vecSelection.append(m_pRTMSAModel->getIdxSelMap()[selected[i].row()]);

////    //create custom context menu and actions
////    QMenu *menu = new QMenu(this);

////    //select channels
////    QAction* doSelection = menu->addAction(tr("Apply selection"));
////    connect(doSelection,&QAction::triggered, [=](){
////        m_pRTMSAModel->selectRows(vecSelection);
////    });

//    // non C++11 alternative
//    m_qListCurrentSelection.clear();
//    for(qint32 i = 0; i < selected.size(); ++i)
//        if(selected[i].column() == 1)
//            m_qListCurrentSelection.append(m_pRTMSAModel->getIdxSelMap()[selected[i].row()]);

//    //create custom context menu and actions
//    QMenu *menu = new QMenu(this);

//    //select channels
//    QAction* doSelection = menu->addAction(tr("Apply selection"));
//    connect(doSelection, &QAction::triggered, this, &RealTimeEvokedWidget::applySelection);

//    //undo selection
//    QAction* resetAppliedSelection = menu->addAction(tr("Reset selection"));
//    connect(resetAppliedSelection,&QAction::triggered, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::resetSelection);
//    connect(resetAppliedSelection,&QAction::triggered, this, &RealTimeEvokedWidget::resetSelection);

//    //show context menu
//    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void RealTimeEvokedWidget::resizeEvent(QResizeEvent* resizeEvent)
{
    Q_UNUSED(resizeEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::keyPressEvent(QKeyEvent* keyEvent)
{
    Q_UNUSED(keyEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::mousePressEvent(QMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::mouseMoveEvent(QMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::mouseDoubleClickEvent(QMouseEvent* mouseEvent)
{
    Q_UNUSED(mouseEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::wheelEvent(QWheelEvent* wheelEvent)
{
    Q_UNUSED(wheelEvent)
}


//*************************************************************************************************************

void RealTimeEvokedWidget::zoomChanged(double zoomFac)
{
    m_fZoomFactor = zoomFac;

//    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
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

