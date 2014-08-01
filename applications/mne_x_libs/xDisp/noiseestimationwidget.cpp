//=============================================================================================================
/**
* @file     noiseestimationwidget.cpp
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
* @brief    Implementation of the NoiseEstimationWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noiseestimationwidget.h"
//#include "annotationwindow.h"

#include <xMeas/noiseestimation.h>


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

#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>


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

NoiseEstimationWidget::NoiseEstimationWidget(QSharedPointer<NoiseEstimation> pNE, QSharedPointer<QTime> &pTime, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pNEModel(Q_NULLPTR)
, m_pNEDelegate(Q_NULLPTR)
, m_pTableView(Q_NULLPTR)
, m_pNE(pNE)
, m_bInitialized(false)
{
    Q_UNUSED(pTime)

    if(m_pTableView)
        delete m_pTableView;
    m_pTableView = new QTableView;

    //set vertical layout
    QVBoxLayout *neLayout = new QVBoxLayout(this);

//    QLabel *t_pLabelNoiseEstimation = new QLabel;
//    t_pLabelNoiseEstimation->setText("Noise Estimation Widget");
//    neLayout->addWidget(t_pLabelNoiseEstimation);

    neLayout->addWidget(m_pTableView);

    //set layouts
    this->setLayout(neLayout);

    getData();
}


//*************************************************************************************************************

NoiseEstimationWidget::~NoiseEstimationWidget()
{

}


//*************************************************************************************************************

void NoiseEstimationWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void NoiseEstimationWidget::getData()
{
//    qDebug() << "get Data" << m_pNE->getValue()(0,1) << "Cols" << m_pNE->getValue().cols();

    if(!m_bInitialized)
    {
        if(m_pNE->isInit())
        {
            init();
        }
    }
    else
        m_pNEModel->addData(m_pNE->getValue());
}

//*************************************************************************************************************

void NoiseEstimationWidget::init()
{
    if(m_pNE->getFiffInfo())
    {
        if(m_pNEModel)
            delete m_pNEModel;
        m_pNEModel = new NoiseEstimationModel(this);

        m_pNEModel->setInfo(m_pNE->getFiffInfo());

        if(m_pNEDelegate)
            delete m_pNEDelegate;
        m_pNEDelegate = new NoiseEstimationDelegate(this);

        connect(m_pTableView, &QTableView::doubleClicked, m_pNEModel, &NoiseEstimationModel::toggleFreeze);

        m_pTableView->setModel(m_pNEModel);
        m_pTableView->setItemDelegate(m_pNEDelegate);

        //set some size settings for m_pTableView
        m_pTableView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

        m_pTableView->setShowGrid(false);

        m_pTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch); //Stretch 2 column to maximal width
        m_pTableView->horizontalHeader()->hide();
        m_pTableView->verticalHeader()->setDefaultSectionSize(140);//m_fZoomFactor*m_fDefaultSectionSize);//Row Height

        m_pTableView->setAutoScroll(false);
        m_pTableView->setColumnHidden(0,true); //because content is plotted jointly with column=1

        m_pTableView->resizeColumnsToContents();

        m_pTableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

        //set context menu
        m_pTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_pTableView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(channelContextMenu(QPoint)));

        m_bInitialized = true;
    }
}
