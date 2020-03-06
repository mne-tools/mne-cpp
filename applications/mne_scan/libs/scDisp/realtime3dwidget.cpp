//=============================================================================================================
/**
 * @file     realtime3dwidget.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RealTime3DWidget Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtime3dwidget.h"

#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimesourceestimate.h>

#include <connectivity/network/network.h>

#include <disp3D/viewers/networkview.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/items/sourcedata/mnedatatreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/model/items/freesurfer/fssurfacetreeitem.h>
#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/model/data3Dtreemodel.h>
#include <disp3D/engine/delegate/data3Dtreedelegate.h>

#include <disp/viewers/control3dview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace DISP3DLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace CONNECTIVITYLIB;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTime3DWidget::RealTime3DWidget(QWidget* parent)
: MeasurementWidget(parent)
, m_bInitialized(false)
, m_p3DView(new View3D())
, m_pData3DModel(Data3DTreeModel::SPtr::create())
, m_iNumberBadChannels(0)
{
    //Init 3D View
    m_p3DView->setModel(m_pData3DModel);

    createGUI();
}

//=============================================================================================================

RealTime3DWidget::~RealTime3DWidget()
{
}

//=============================================================================================================

void RealTime3DWidget::createGUI()
{
    QWidget *pWidgetContainer = QWidget::createWindowContainer(m_p3DView, this, Qt::Widget);
    pWidgetContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pWidgetContainer->setMinimumSize(400,400);

    QGridLayout* pMainLayoutView = new QGridLayout();
    pMainLayoutView->addWidget(pWidgetContainer,0,0);
    pMainLayoutView->setContentsMargins(0,0,0,0);

    this->setLayout(pMainLayoutView);
}

//=============================================================================================================

void RealTime3DWidget::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(RealTimeConnectivityEstimate::SPtr pRTCE = qSharedPointerDynamicCast<RealTimeConnectivityEstimate>(pMeasurement)) {
        if(pRTCE->getValue().data()->isEmpty()) {
            return;
        }

        // Add rt brain data
        if(!m_pRtConnectivityItem) {
            //qDebug()<<"RealTime3DWidget::getData - Creating m_pRtConnectivityItem";
            m_pRtConnectivityItem = m_pData3DModel->addConnectivityData("sample",
                                                            "Connectivity",
                                                            *(pRTCE->getValue().data()));

            m_pRtConnectivityItem->setThresholds(QVector3D(0.9f,0.95f,1.0f));

            if(pRTCE->getSurfSet() && pRTCE->getAnnotSet()) {
                QList<FsSurfaceTreeItem*> lSurfaces = m_pData3DModel->addSurfaceSet("sample",
                                                                                    "MRI",
                                                                                    *(pRTCE->getSurfSet().data()),
                                                                                    *(pRTCE->getAnnotSet().data()));

                for(int i = 0; i < lSurfaces.size(); i++) {
                    lSurfaces.at(i)->setAlpha(0.3f);
                }
            }

            if(pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                m_pData3DModel->addMegSensorInfo("sample",
                                                 "Sensors",
                                                 pRTCE->getFiffInfo()->chs,
                                                 *(pRTCE->getSensorSurface()),
                                                 pRTCE->getFiffInfo()->bads);
                m_iNumberBadChannels = pRTCE->getFiffInfo()->bads.size();

                init();
            }
        } else {
            //qDebug()<<"RealTime3DWidget::getData - Working with m_pRtConnectivityItem";
            QPair<float,float> freqs = pRTCE->getValue()->getFrequencyRange();
            QString sItemName = QString("%1_%2_%3").arg(pRTCE->getValue()->getConnectivityMethod()).arg(QString::number(freqs.first)).arg(QString::number(freqs.second));
            m_pRtConnectivityItem->setText(sItemName);
            m_pRtConnectivityItem->addData(*(pRTCE->getValue().data()));

            if(pRTCE->getSensorSurface() && pRTCE->getFiffInfo()) {
                if(m_iNumberBadChannels != pRTCE->getFiffInfo()->bads.size()) {
                    m_pData3DModel->addMegSensorInfo("sample",
                                                     "Sensors",
                                                     pRTCE->getFiffInfo()->chs,
                                                     *(pRTCE->getSensorSurface()),
                                                     pRTCE->getFiffInfo()->bads);
                    m_iNumberBadChannels = pRTCE->getFiffInfo()->bads.size();
                }
            }
        }
    } else if(RealTimeSourceEstimate::SPtr pRTSE = qSharedPointerDynamicCast<RealTimeSourceEstimate>(pMeasurement)) {
        QList<MNESourceEstimate::SPtr> lMNEData = pRTSE->getValue();

        // Add source estimate data
        if(!lMNEData.isEmpty()) {
            if(!m_pRtMNEItem && pRTSE->getAnnotSet() && pRTSE->getSurfSet() && pRTSE->getFwdSolution()) {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Creating m_lRtItem list";
                m_pRtMNEItem = m_pData3DModel->addSourceData("Subject", "Data",
                                                             *lMNEData.first(),
                                                             *pRTSE->getFwdSolution(),
                                                             *pRTSE->getSurfSet(),
                                                             *pRTSE->getAnnotSet());

                m_pRtMNEItem->setLoopState(false);
                m_pRtMNEItem->setTimeInterval(17);
                m_pRtMNEItem->setThresholds(QVector3D(0.0,5,10));
                m_pRtMNEItem->setColormapType("Hot");
                m_pRtMNEItem->setVisualizationType("Annotation based");
                m_pRtMNEItem->setNumberAverages(17);
                m_pRtMNEItem->setAlpha(1.0);
                m_pRtMNEItem->setStreamingState(true);
                m_pRtMNEItem->setSFreq(pRTSE->getFiffInfo()->sfreq);
            } else {
                //qDebug()<<"RealTimeSourceEstimateWidget::getData - Working with m_lRtItem list";

                if(m_pRtMNEItem) {
                    m_pRtMNEItem->addData(*lMNEData.first());
                }
            }
        }
    }
}

//=============================================================================================================

void RealTime3DWidget::init()
{
    Data3DTreeDelegate* pData3DTreeDelegate = new Data3DTreeDelegate(this);

    //Init control widgets
    QList<QWidget*> lControlWidgets;

    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    Control3DView* pControl3DView = new Control3DView(Q_NULLPTR, slControlFlags);
    pControl3DView->setObjectName("group_tab_View_General");
    lControlWidgets.append(pControl3DView);

    pControl3DView->setDelegate(pData3DTreeDelegate);
    pControl3DView->setModel(m_pData3DModel.data());

    connect(pControl3DView, &Control3DView::sceneColorChanged,
            m_p3DView.data(), &View3D::setSceneColor);

    connect(pControl3DView, &Control3DView::rotationChanged,
            m_p3DView.data(), &View3D::startStopModelRotation);

    connect(pControl3DView, &Control3DView::showCoordAxis,
            m_p3DView.data(), &View3D::toggleCoordAxis);

    connect(pControl3DView, &Control3DView::showFullScreen,
            m_p3DView.data(), &View3D::showFullScreen);

    connect(pControl3DView, &Control3DView::lightColorChanged,
            m_p3DView.data(), &View3D::setLightColor);

    connect(pControl3DView, &Control3DView::lightIntensityChanged,
            m_p3DView.data(), &View3D::setLightIntensity);

    connect(pControl3DView, &Control3DView::takeScreenshotChanged,
            m_p3DView.data(), &View3D::takeScreenshot);

    emit displayControlWidgetsChanged(lControlWidgets, "3D View");

    m_bInitialized = true;
}
