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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimateWidget::RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> &pRTSE, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTMSE(pRTSE)
, m_bInitialized(false)
{

}


//*************************************************************************************************************

RealTimeSourceEstimateWidget::~RealTimeSourceEstimateWidget()
{
//    // Clear sampling rate vector
//    RealTimeSourceEstimateWidget::s_listSamplingRates.clear();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::actualize()
{
    if(m_bInitialized)
    {
//        m_dPosY = ui.m_qFrame->pos().y()+0.5*ui.m_qFrame->height();


//        // Compute scaling factor
//        m_fScaleFactor = ui.m_qFrame->height()/static_cast<float>(m_pRTMSA_New->chInfo()[0].getMaxValue()-m_pRTMSA_New->chInfo()[0].getMinValue());

//        // Compute the middle of RTSA values
//        m_dMiddle = 0.5*(m_pRTMSA_New->chInfo()[0].getMinValue()+m_pRTMSA_New->chInfo()[0].getMaxValue())*m_fScaleFactor;

//        //*********************************************************************************************************
//        //=========================================================================================================
//        // Compute new sample width in order to synchronize all RTSA
//        //=========================================================================================================

//        if((m_pRTMSA_New->getSamplingRate() == 0) || (DisplayManager::getRTSAWidgets().size() == 0))
//            return;

//        // Add current sampling rate to s_listSamplingRates
//        RealTimeSourceEstimateWidget::s_listSamplingRates << m_pRTMSA_New->getSamplingRate();

//        // Find maximal sampling rate in s_listSamplingRates
//        double dMax = 0;
//        foreach (double value, s_listSamplingRates)
//            dMax = value > dMax ? value : dMax;

//        // Set new sample widths
//        foreach(RealTimeSourceEstimateWidget* pRTMSAW, DisplayManager::getRTMSANewWidgets().values())
//            pRTMSAW->m_dSampleWidth = dMax/pRTMSAW->m_pRTMSA_New->getSamplingRate();
    }
    else
        this->init();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::maxValueChanged(double maxValue)
{
////    m_pRTMSA_New->setMaxValue(maxValue);
//    for(quint32 i = 0; i < m_pRTMSA_New->getNumChannels(); ++i)
//            m_pRTMSA_New->chInfo()[i].setMaxValue(maxValue);

////    ui.m_qLabel_MaxValue->setText(QString::number(maxValue));
//    actualize();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::minValueChanged(double minValue)
{
////    m_pRTMSA_New->setMinValue(minValue);
//    for(quint32 i = 0; i < m_pRTMSA_New->getNumChannels(); ++i)
//        m_pRTMSA_New->chInfo()[i].setMaxValue(minValue);

////    ui.m_qLabel_MinValue->setText(QString::number(minValue));
//    actualize();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::update(XMEASLIB::NewMeasurement::SPtr)
{
//    VectorXd vecValue = VectorXd::Zero(m_uiNumChannels);
//    double dPositionDifference = 0.0;
    MNESourceEstimate stc = m_pRTMSE->getStc();

    std::cout << "#### Update display ####";

}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::init()
{
    if(this->initOpenGLWidget())
    {


//    ui.m_qLabel_Caption->setText(m_pRTMSA_New->getName());
////    ui.m_qLabel_MinValue->setText(QString::number(m_pRTSM->getMinValue()));
////    ui.m_qLabel_MaxValue->setText(QString::number(m_pRTSM->getMaxValue()));

//    m_dMinValue_init = m_pRTMSA_New->chInfo()[0].getMinValue();
//    m_dMaxValue_init = m_pRTMSA_New->chInfo()[0].getMaxValue();


//    m_bStartFlag = true;

        m_bInitialized = true;

//    m_pTimeCurrentDisplay = QSharedPointer<QTime>(new QTime(0, 0));

//    actualize();

    }
    else
        m_bInitialized = false;
}


//*************************************************************************************************************

bool RealTimeSourceEstimateWidget::initOpenGLWidget()
{
    if(!m_pRTMSE->getAnnotSet()->isEmpty() && !m_pRTMSE->getSurfSet()->isEmpty())
    {
        QList<Label> t_qListLabels;
        QList<RowVector4i> t_qListRGBAs;

        m_pRTMSE->getAnnotSet()->toLabels(*m_pRTMSE->getSurfSet().data(), t_qListLabels, t_qListRGBAs);

        QHBoxLayout *layout = new QHBoxLayout(this);

        m_pView = new InverseView(m_pRTMSE->getSrc(), t_qListLabels, t_qListRGBAs);

        if (m_pView->stereoType() != QGLView::RedCyanAnaglyph)
            m_pView->camera()->setEyeSeparation(0.3f);

        m_pWidgetView = QWidget::createWindowContainer(m_pView);
//        m_pContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

void RealTimeSourceEstimateWidget::paintEvent(QPaintEvent*)
{
//    QPainter painter(this);

}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::resizeEvent(QResizeEvent*)
{
//    actualize();
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
