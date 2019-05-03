//=============================================================================================================
/**
* @file     realtimeconnectivityestimatewidget.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the RealTimeConnectivityEstimateWidget Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeconnectivityestimatewidget.h"

#include <scMeas/realtimeconnectivityestimate.h>

#include <connectivity/network/network.h>

#include <disp/viewers/quickcontrolview.h>

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QElapsedTimer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace DISP3DLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeConnectivityEstimateWidget::RealTimeConnectivityEstimateWidget(QSharedPointer<SCMEASLIB::RealTimeConnectivityEstimate> &pRTCE, QWidget* parent)
: MeasurementWidget(parent)
, m_pRTCE(pRTCE)
, m_bInitialized(false)
, m_pRtItem(Q_NULLPTR)
, m_pAbstractView(new AbstractView())
, m_iNumberBadChannels(0)
{
    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget (F9)"),this);
    m_pActionQuickControl->setShortcut(tr("F9"));
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget (F9)"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeConnectivityEstimateWidget::showQuickControlView);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

    QList<QSharedPointer<QWidget> > lControlWidgets = m_pRTCE->getControlWidgets();
    m_pAbstractView->setQuickControlWidgets(lControlWidgets);

    m_pQuickControlView = m_pAbstractView->getQuickControl();
    m_pQuickControlView->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
    m_pQuickControlView->setDraggable(true);
    m_pQuickControlView->setVisiblityHideOpacityClose(true);

    QGridLayout *mainLayoutView = new QGridLayout();
    mainLayoutView->addWidget(m_pAbstractView.data());
    mainLayoutView->setContentsMargins(0,0,0,0);
    this->setLayout(mainLayoutView);
}


//*************************************************************************************************************

RealTimeConnectivityEstimateWidget::~RealTimeConnectivityEstimateWidget()
{
    // Store Settings
    if(!m_pRTCE->getName().isEmpty()) {
    }
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::update(SCMEASLIB::Measurement::SPtr)
{
    getData();
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::getData()
{
    if(m_pRTCE) {
        if(m_pRTCE->getValue().data()->isEmpty()) {
            return;
        }

        // Add rt brain data
        if(!m_pRtItem) {
            //qDebug()<<"RealTimeConnectivityEstimateWidget::getData - Creating m_pRtItem";
            m_pRtItem = m_pAbstractView->getTreeModel()->addConnectivityData("sample",
                                                                             "Connectivity",
                                                                             *(m_pRTCE->getValue().data()));

            m_pRtItem->setThresholds(QVector3D(0.9f,0.95f,1.0f));

            if(m_pRTCE->getSurfSet() && m_pRTCE->getAnnotSet()) {
                QList<FsSurfaceTreeItem*> lSurfaces = m_pAbstractView->getTreeModel()->addSurfaceSet("sample",
                                                                                                     "MRI",
                                                                                                     *(m_pRTCE->getSurfSet().data()),
                                                                                                     *(m_pRTCE->getAnnotSet().data()));

                for(int i = 0; i < lSurfaces.size(); i++) {
                    lSurfaces.at(i)->setAlpha(0.3f);
                }
            }

            if(m_pRTCE->getSensorSurface() && m_pRTCE->getFiffInfo()) {
                m_pAbstractView->getTreeModel()->addMegSensorInfo("sample",
                                                                  "Sensors",
                                                                  m_pRTCE->getFiffInfo()->chs,
                                                                  *(m_pRTCE->getSensorSurface()),
                                                                  m_pRTCE->getFiffInfo()->bads);
                m_iNumberBadChannels = m_pRTCE->getFiffInfo()->bads.size();
            }
        } else {
            QElapsedTimer time;
            time.start();

            //qDebug()<<"RealTimeConnectivityEstimateWidget::getData - Working with m_pRtItem";
            QPair<float,float> freqs = m_pRTCE->getValue()->getFrequencyRange();
            QString sItemName = QString("%1_%2_%3").arg(m_pRTCE->getValue()->getConnectivityMethod()).arg(QString::number(freqs.first)).arg(QString::number(freqs.second));
            m_pRtItem->setText(sItemName);
            m_pRtItem->addData(*(m_pRTCE->getValue().data()));

            if(m_pRTCE->getSensorSurface() && m_pRTCE->getFiffInfo()) {
                if(m_iNumberBadChannels != m_pRTCE->getFiffInfo()->bads.size()) {
                    m_pAbstractView->getTreeModel()->addMegSensorInfo("sample",
                                                                      "Sensors",
                                                                      m_pRTCE->getFiffInfo()->chs,
                                                                      *(m_pRTCE->getSensorSurface()),
                                                                      m_pRTCE->getFiffInfo()->bads);
                    m_iNumberBadChannels = m_pRTCE->getFiffInfo()->bads.size();
                }
            }

            qInfo() << time.elapsed() << "0" << "RealTimeConnectivityEstimateWidget Time";
        }
    }
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::init()
{
    m_bInitialized = true;
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::showQuickControlView()
{
    if(m_pQuickControlView) {
        m_pQuickControlView->raise();
        m_pQuickControlView->show();
    }
}
