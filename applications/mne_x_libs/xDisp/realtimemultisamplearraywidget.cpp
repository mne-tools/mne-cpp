//=============================================================================================================
/**
* @file     realtimesamplearray_new_widget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the RealTimeMultiSampleArrayWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimemultisamplearraywidget.h"
//#include "annotationwindow.h"

#include "roiselectionwidget.h"

#include <xMeas/newrealtimemultisamplearray.h>

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
#include <QTimer>
#include <QTime>
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

RealTimeMultiSampleArrayWidget::RealTimeMultiSampleArrayWidget(QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTMSAModel(NULL)
, m_pRTMSADelegate(NULL)
, m_pTableView(NULL)
, m_fDefaultSectionSize(80.0f)
, m_fZoomFactor(1.0f)
, m_pRTMSA(pRTMSA)
, m_bInitialized(false)
, m_iT(10)
, m_fSamplingRate(1024)
, m_fDesiredSamplingRate(128)
{
    m_pDoubleSpinBoxZoom = new QDoubleSpinBox(this);
    m_pDoubleSpinBoxZoom->setMinimum(0.3);
    m_pDoubleSpinBoxZoom->setMaximum(4.0);
    m_pDoubleSpinBoxZoom->setSingleStep(0.1);
    m_pDoubleSpinBoxZoom->setValue(1.0);
    m_pDoubleSpinBoxZoom->setSuffix(" x");
    connect(m_pDoubleSpinBoxZoom, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &RealTimeMultiSampleArrayWidget::zoomChanged);
    addDisplayWidget(m_pDoubleSpinBoxZoom);

    m_pSpinBoxTimeScale = new QSpinBox(this);
    m_pSpinBoxTimeScale->setMinimum(1);
    m_pSpinBoxTimeScale->setMaximum(20);
    m_pSpinBoxTimeScale->setValue(m_iT);
    m_pSpinBoxTimeScale->setSuffix(" s");
    connect(m_pSpinBoxTimeScale, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &RealTimeMultiSampleArrayWidget::timeWindowChanged);
    addDisplayWidget(m_pSpinBoxTimeScale);

    m_pActionSelectRoi = new QAction(QIcon(":/images/selectRoi.png"), tr("Shows the region selection widget (F12)"),this);
    m_pActionSelectRoi->setShortcut(tr("F12"));
    m_pActionSelectRoi->setStatusTip(tr("Shows the region selection widget (F12)"));
    connect(m_pActionSelectRoi, &QAction::triggered, this, &RealTimeMultiSampleArrayWidget::showRoiSelectionWidget);
    addDisplayAction(m_pActionSelectRoi);

    if(m_pTableView)
        delete m_pTableView;
    m_pTableView = new QTableView;

    //set vertical layout
    QVBoxLayout *rtmsaLayout = new QVBoxLayout(this);

    rtmsaLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(rtmsaLayout);

    init();
}


//*************************************************************************************************************

RealTimeMultiSampleArrayWidget::~RealTimeMultiSampleArrayWidget()
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    if(!m_bInitialized)
    {
        m_qListChInfo = m_pRTMSA->chInfo();
        m_fSamplingRate = m_pRTMSA->getSamplingRate();
        init();
    }
    else
        m_pRTMSAModel->addData(m_pRTMSA->getMultiSampleArray());
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::init()
{
    if(m_qListChInfo.size() > 0)
    {
        if(m_pRTMSAModel)
            delete m_pRTMSAModel;
        m_pRTMSAModel = new RealTimeMultiSampleArrayModel(this);

        m_pRTMSAModel->setChannelInfo(m_qListChInfo);
        m_pRTMSAModel->setSamplingInfo(m_fSamplingRate, m_iT, m_fDesiredSamplingRate);

        if(m_pRTMSADelegate)
            delete m_pRTMSADelegate;
        m_pRTMSADelegate = new RealTimeMultiSampleArrayDelegate(this);

        connect(m_pTableView, &QTableView::doubleClicked, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::toggleFreez);

        m_pTableView->setModel(m_pRTMSAModel);
        m_pTableView->setItemDelegate(m_pRTMSADelegate);

        //set some size settings for m_pTableView
        m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

        m_pTableView->setShowGrid(false);

        m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
        m_pTableView->horizontalHeader()->hide();
        m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height

        m_pTableView->setAutoScroll(false);
        m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

        m_pTableView->resizeColumnsToContents();

        m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

        //set context menu
        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(channelContextMenu(QPoint)));

        //activate kinetic scrolling
        QScroller::grabGesture(m_pTableView,QScroller::MiddleMouseButtonGesture);

        m_bInitialized = true;
    }
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::channelContextMenu(QPoint pos)
{
    //obtain index where index was clicked
    QModelIndex index = m_pTableView->indexAt(pos);

    //get selected items
    QModelIndexList selected = m_pTableView->selectionModel()->selectedIndexes();

//    // Lambda C++11 version
//    QVector<qint32> vecSelection;
//    for(qint32 i = 0; i < selected.size(); ++i)
//        if(selected[i].column() == 1)
//            vecSelection.append(m_pRTMSAModel->getIdxSelMap()[selected[i].row()]);

//    //create custom context menu and actions
//    QMenu *menu = new QMenu(this);

//    //select channels
//    QAction* doSelection = menu->addAction(tr("Apply selection"));
//    connect(doSelection,&QAction::triggered, [=](){
//        m_pRTMSAModel->selectRows(vecSelection);
//    });

    // non C++11 alternative
    m_qVecCurrentSelection.clear();
    for(qint32 i = 0; i < selected.size(); ++i)
        if(selected[i].column() == 1)
            m_qVecCurrentSelection.append(m_pRTMSAModel->getIdxSelMap()[selected[i].row()]);

    //create custom context menu and actions
    QMenu *menu = new QMenu(this);

    //select channels
    QAction* doSelection = menu->addAction(tr("Apply selection"));
    connect(doSelection, &QAction::triggered, this, &RealTimeMultiSampleArrayWidget::applySelection);

    //undo selection
    QAction* resetAppliedSelection = menu->addAction(tr("Reset selection"));
    connect(resetAppliedSelection,&QAction::triggered, m_pRTMSAModel, &RealTimeMultiSampleArrayModel::resetSelection);

    //show context menu
    menu->popup(m_pTableView->viewport()->mapToGlobal(pos));
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::resizeEvent(QResizeEvent*)
{
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::keyPressEvent(QKeyEvent* keyEvent)
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::mousePressEvent(QMouseEvent* mouseEvent)
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::mouseMoveEvent(QMouseEvent* mouseEvent)
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::mouseReleaseEvent(QMouseEvent*)
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::mouseDoubleClickEvent(QMouseEvent*)
{

}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::wheelEvent(QWheelEvent* wheelEvent)
{

}

//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::zoomChanged(double zoomFac)
{
    m_fZoomFactor = zoomFac;

    m_pTableView->verticalHeader()->setDefaultSectionSize(m_fZoomFactor*m_fDefaultSectionSize);//Row Height
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::timeWindowChanged(int T)
{
    m_iT = T;
    m_pRTMSAModel->setSamplingInfo(m_fSamplingRate, T, m_fDesiredSamplingRate);
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::showRoiSelectionWidget()
{
    if(!m_pRoiSelectionWidget)
        m_pRoiSelectionWidget = QSharedPointer<XDISPLIB::RoiSelectionWidget>(new XDISPLIB::RoiSelectionWidget);

    m_pRoiSelectionWidget->show();
}


//*************************************************************************************************************

void RealTimeMultiSampleArrayWidget::applySelection()
{
    m_pRTMSAModel->selectRows(m_qVecCurrentSelection);
}
