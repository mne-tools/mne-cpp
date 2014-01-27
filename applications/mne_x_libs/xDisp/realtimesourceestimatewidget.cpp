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

#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QTime>

#include <QDebug>


#include <QWindow>
#include <QWidget>
#include <QHBoxLayout>
#include <QLineEdit>


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
, m_pRTMSE(pRTSE)
, m_bInitialized(false)
, m_bInitializationStarted(false)
, count(0)
{
    connect(this,&RealTimeSourceEstimateWidget::startInit, this, &RealTimeSourceEstimateWidget::init);
}


//*************************************************************************************************************

RealTimeSourceEstimateWidget::~RealTimeSourceEstimateWidget()
{
//    // Clear sampling rate vector
//    RealTimeSourceEstimateWidget::s_listSamplingRates.clear();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
    if(m_bInitialized)
    {
        if(count % 4 == 0)
        {
            MNESourceEstimate stc = m_pRTMSE->getStc();
            m_pView->pushSourceEstimate(stc);
        }
        ++count;
    }
    else
    {
        if(!m_bInitializationStarted && !m_pRTMSE->getSrc().isEmpty())
        {
            m_pRTMSE->m_bStcSend = false;
            m_bInitializationStarted = true;
            emit startInit();
        }
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::init()
{
    if(this->initOpenGLWidget())
    {
        m_bInitialized = true;
        m_pRTMSE->m_bStcSend = true;
    }
    else
    {
        m_bInitialized = false;
        m_bInitializationStarted = false;
    }
}


//*************************************************************************************************************

bool RealTimeSourceEstimateWidget::initOpenGLWidget()
{
    if(     !m_pRTMSE->getSrc().isEmpty() &&
            !m_pRTMSE->getAnnotSet()->isEmpty() &&
            !m_pRTMSE->getSurfSet()->isEmpty())
    {
        QList<Label> t_qListLabels;
        QList<RowVector4i> t_qListRGBAs;

        m_pRTMSE->getAnnotSet()->toLabels(*m_pRTMSE->getSurfSet().data(), t_qListLabels, t_qListRGBAs);

        QHBoxLayout *layout = new QHBoxLayout(this);

        m_pView = new InverseView(m_pRTMSE->getSrc(), t_qListLabels, t_qListRGBAs, 12, false);

        if (m_pView->stereoType() != QGLView::RedCyanAnaglyph)
            m_pView->camera()->setEyeSeparation(0.3f);

        m_pWidgetView = QWidget::createWindowContainer(m_pView); //widget take owner ship of m_pView
//        m_pContainer->setFocusPolicy(Qt::StrongFocus);
        m_pWidgetView->setFocusPolicy(Qt::TabFocus);

//        layout->addWidget(new QLineEdit(QLatin1String("A QLineEdit")));
        layout->addWidget(m_pWidgetView);
//        layout->addWidget(new QLineEdit(QLatin1String("A QLabel")));

        return true;
    }
    else
        return false;
}

//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
