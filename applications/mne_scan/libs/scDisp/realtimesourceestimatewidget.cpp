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
* @brief    Definition of the RealTimeSourceEstimateWidget Class.
*
*/

//ToDo Paint to render area

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesourceestimatewidget.h"

#include <scMeas/realtimesourceestimate.h>

#include <disp3D/engine/model/items/sourcedata/mneestimatetreeitem.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/control/control3dwidget.h>
#include <disp3D/engine/model/data3Dtreemodel.h>

#include <mne/mne_forwardsolution.h>
#include <mne/mne_inverse_operator.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <inverse/minimumNorm/minimumnorm.h>

#include <math.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSlider>
#include <QAction>
#include <QLabel>
#include <QGridLayout>
#include <QSettings>
#include <QDebug>
#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace DISP3DLIB;
using namespace MNELIB;
using namespace SCMEASLIB;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimateWidget::RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> &pRTSE, QWidget* parent)
: NewMeasurementWidget(parent)
, m_pRTSE(pRTSE)
, m_bInitialized(false)
, m_pRtItem(Q_NULLPTR)
{
    m_pAction3DControl = new QAction(QIcon(":/images/3DControl.png"), tr("Shows the 3D control widget (F9)"),this);
    m_pAction3DControl->setShortcut(tr("F9"));
    m_pAction3DControl->setToolTip(tr("Shows the 3D control widget (F9)"));
    connect(m_pAction3DControl, &QAction::triggered,
            this, &RealTimeSourceEstimateWidget::show3DControlWidget);
    addDisplayAction(m_pAction3DControl);
    m_pAction3DControl->setVisible(true);

    m_p3DView = View3D::SPtr(new View3D());
    m_pData3DModel = Data3DTreeModel::SPtr(new Data3DTreeModel());

    m_p3DView->setModel(m_pData3DModel);

    m_pControl3DView = Control3DWidget::SPtr(new Control3DWidget(this,
                                                                 QStringList() << "Minimize" << "Data" << "Window" << "View" << "Light",
                                                                 Qt::Window));
    m_pControl3DView->init(m_pData3DModel, m_p3DView);

    QGridLayout *mainLayoutView = new QGridLayout;
    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_p3DView.data());
    mainLayoutView->addWidget(pWidgetContainer);

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
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::update(SCMEASLIB::NewMeasurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::getData()
{
    if(m_bInitialized)
    {
        //
        // Add rt brain data
        //
        if(!m_pRtItem) {
            //qDebug()<<"RealTimeSourceEstimateWidget::getData - Creating m_lRtItem list";
            m_pRtItem = m_pData3DModel->addSourceData("Subject", "Data",
                                                      *m_pRTSE->getValue(),
                                                      *m_pRTSE->getFwdSolution(),
                                                      m_surfSet,
                                                      m_annotationSet,
                                                      m_p3DView->format());

            m_pRtItem->setLoopState(false);
            m_pRtItem->setTimeInterval(17);
            m_pRtItem->setThresholds(QVector3D(0.0,5,10));
            m_pRtItem->setColormapType("Hot");
            m_pRtItem->setVisualizationType("Annotation based");
            m_pRtItem->setNumberAverages(1);
            m_pRtItem->setStreamingState(true);
            m_pRtItem->setSFreq(m_pRTSE->getFiffInfo()->sfreq);
        } else {
            //qDebug()<<"RealTimeSourceEstimateWidget::getData - Working with m_lRtItem list";

            if(m_pRtItem) {
                m_pRtItem->addData(*m_pRTSE->getValue());
            }
        }
    }
    else
    {
        if(m_pRTSE->getAnnotSet() && m_pRTSE->getSurfSet())
        {
            m_pRTSE->m_bStcSend = false;
            init();

            //
            // Add brain data
            //
            m_pData3DModel->addSurfaceSet("Subject", "MRI", *m_pRTSE->getSurfSet(), *m_pRTSE->getAnnotSet());
        }
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::init()
{
    m_bInitialized = true;
    m_pRTSE->m_bStcSend = true;
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::show3DControlWidget()
{
    if(m_pControl3DView->isActiveWindow())
        m_pControl3DView->hide();
    else {
        m_pControl3DView->activateWindow();
        m_pControl3DView->show();
    }
}


//*************************************************************************************************************
//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================
