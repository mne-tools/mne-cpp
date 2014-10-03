//=============================================================================================================
/**
* @file     realtimesourceestimatewidget.cpp
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
* @brief    Implementation of the RealTimeSourceEstimateWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesourceestimatewidget.h"
//#include "annotationwindow.h"

#include <xMeas/realtimesourceestimate.h>

#include <disp3D/geometryview.h>
#include <mne/mne_forwardsolution.h>



#include <mne/mne_inverse_operator.h>
#include <inverse/minimumNorm/minimumnorm.h>


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

#include <QLabel>
#include <QGridLayout>
#include <QSettings>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XDISPLIB;
using namespace DISP3DLIB;
using namespace MNELIB;
using namespace XMEASLIB;


using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimateWidget::RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> &pRTSE, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTSE(pRTSE)
, m_bInitialized(false)
{
    m_pClustStcModel = new ClustStcModel(this);
//    m_pClustStcModel->init(t_annotationSet, t_surfSet);
    m_pClustStcModel->setLoop(false);

    //
    // STC view
    //
    QGridLayout *mainLayoutView = new QGridLayout;

    QLabel * pLabelNormView = new QLabel("Norm");
    m_pSliderNormView = new QSlider(Qt::Vertical);
    QObject::connect(m_pSliderNormView, &QSlider::valueChanged, m_pClustStcModel, &ClustStcModel::setNormalization);
    m_pSliderNormView->setMinimum(1);
    m_pSliderNormView->setMaximum(20000);
    m_pSliderNormView->setValue(2000);

    QLabel* pLabelAverageView = new QLabel("Average");
    m_pSliderAverageView = new QSlider(Qt::Horizontal);
    QObject::connect(m_pSliderAverageView, &QSlider::valueChanged, m_pClustStcModel, &ClustStcModel::setAverage);
    m_pSliderAverageView->setMinimum(1);
    m_pSliderAverageView->setMaximum(500);
    m_pSliderAverageView->setValue(100);

    m_pClustView = new ClustStcView(false, true, QGLView::RedCyanAnaglyph);//(false); (true, QGLView::StretchedLeftRight); (true, QGLView::RedCyanAnaglyph);
    m_pClustView->setModel(m_pClustStcModel);

    if (m_pClustView->stereoType() != QGLView::RedCyanAnaglyph)
        m_pClustView->camera()->setEyeSeparation(0.3f);

    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_pClustView);

    mainLayoutView->addWidget(pWidgetContainer,0,0,2,2);
    mainLayoutView->addWidget(pLabelNormView,0,3);
    mainLayoutView->addWidget(m_pSliderNormView,1,3);
    mainLayoutView->addWidget(pLabelAverageView,3,0);
    mainLayoutView->addWidget(m_pSliderAverageView,3,1);

    this->setLayout(mainLayoutView);

    getData();
}


//*************************************************************************************************************

RealTimeSourceEstimateWidget::~RealTimeSourceEstimateWidget()
{
    //
    // Store Settings
    //
    if(!m_pRTSE->getName().isEmpty())
    {
        QString t_sRTSEName = m_pRTSE->getName();

        QSettings settings;
        settings.setValue(QString("RTSEW/%1/norm").arg(t_sRTSEName), m_pSliderNormView->value());
        settings.setValue(QString("RTSEW/%1/average").arg(t_sRTSEName), m_pSliderAverageView->value());
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::getData()
{
    if(m_bInitialized)
    {
        //
        // Add Data
        //
        m_pClustStcModel->addData(*m_pRTSE->getValue());
    }
    else
    {
        if(m_pRTSE->getAnnotSet() && m_pRTSE->getSurfSet())
        {
            m_pRTSE->m_bStcSend = false;
            init();
            //
            // Add Data
            //
            m_pClustStcModel->addData(*m_pRTSE->getValue());
        }
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::init()
{
    QString t_sRTSEName = m_pRTSE->getName();
    QSettings settings;
    m_pSliderNormView->setValue(settings.value(QString("RTSEW/%1/norm").arg(t_sRTSEName), 2000).toInt());
    m_pSliderAverageView->setValue(settings.value(QString("RTSEW/%1/average").arg(t_sRTSEName), 100).toInt());

    m_pClustStcModel->init(*m_pRTSE->getAnnotSet(), *m_pRTSE->getSurfSet());
    m_bInitialized = true;
    m_pRTSE->m_bStcSend = true;
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
