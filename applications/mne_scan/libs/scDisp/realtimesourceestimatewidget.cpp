//=============================================================================================================
/**
* @file     realtimesourceestimatewidget.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimesourceestimatewidget.h"

#include <scMeas/realtimesourceestimate.h>

#include <disp/viewers/quickcontrolview.h>

#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>
#include <disp3D/viewers/sourceestimateview.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QVector3D>
#include <QElapsedTimer>


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
using namespace DISPLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSourceEstimateWidget::RealTimeSourceEstimateWidget(QSharedPointer<RealTimeSourceEstimate> &pRTSE, QWidget* parent)
: MeasurementWidget(parent)
, m_pRTSE(pRTSE)
, m_bInitialized(false)
, m_pRtItem(Q_NULLPTR)
, m_pSourceEstimateView(SourceEstimateView::SPtr::create())
{
    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeSourceEstimateWidget::showQuickControlView);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

    QGridLayout *mainLayoutView = new QGridLayout;
    mainLayoutView->addWidget(m_pSourceEstimateView.data(),0,0);

    QList<QSharedPointer<QWidget> > lControlWidgets = m_pRTSE->getControlWidgets();
    m_pSourceEstimateView->setQuickControlWidgets(lControlWidgets);

    m_pQuickControlView = m_pSourceEstimateView->getQuickControl();
    m_pQuickControlView->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
    m_pQuickControlView->setDraggable(true);
    m_pQuickControlView->setVisiblityHideOpacityClose(true);

    mainLayoutView->setContentsMargins(0,0,0,0);
    this->setLayout(mainLayoutView);

    getData();
}


//*************************************************************************************************************

RealTimeSourceEstimateWidget::~RealTimeSourceEstimateWidget()
{
    // Store Settings
    if(!m_pRTSE->getName().isEmpty()) {
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::update(SCMEASLIB::Measurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::getData()
{
    if(m_bInitialized) {
        QList<MNESourceEstimate::SPtr> lMNEData = m_pRTSE->getValue();

        // Add source estimate data
        if(!lMNEData.isEmpty()) {
            QElapsedTimer time;
            time.start();
            if(!m_pRtItem && m_pRTSE->getAnnotSet() && m_pRTSE->getSurfSet() && m_pRTSE->getFwdSolution()) {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Creating m_lRtItem list";
                m_pRtItem = m_pSourceEstimateView->addData("Subject", "Data",
                                                          *lMNEData.first(),
                                                          *m_pRTSE->getFwdSolution(),
                                                          *m_pRTSE->getSurfSet(),
                                                          *m_pRTSE->getAnnotSet());

                m_pRtItem->setLoopState(false);
                m_pRtItem->setTimeInterval(17);
                m_pRtItem->setThresholds(QVector3D(0.0,5,10));
                m_pRtItem->setColormapType("Hot");
                m_pRtItem->setVisualizationType("Annotation based");
                m_pRtItem->setNumberAverages(17);
                m_pRtItem->setAlpha(1.0);
                m_pRtItem->setStreamingState(true);
                m_pRtItem->setSFreq(m_pRTSE->getFiffInfo()->sfreq);
            } else {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Working with m_lRtItem list";

                if(m_pRtItem) {
                    m_pRtItem->addData(*lMNEData.first());
                }
            }
            qInfo() << time.elapsed() << "0" << "RealTimeSourceEstimateWidget Time";
        }
    } else {
        init();
    }
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::init()
{
    m_bInitialized = true;
}


//*************************************************************************************************************

void RealTimeSourceEstimateWidget::showQuickControlView()
{
    if(m_pQuickControlView) {
        m_pQuickControlView->raise();
        m_pQuickControlView->show();
    }
}
