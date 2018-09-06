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

#include <mne/mne_forwardsolution.h>

#include <disp/viewers/quickcontrolview.h>

#include <disp3D/engine/view/view3D.h>
#include <disp3D/engine/control/control3dwidget.h>
#include <disp3D/engine/model/items/network/networktreeitem.h>
#include <disp3D/engine/model/data3Dtreemodel.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QGridLayout>


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
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeConnectivityEstimateWidget::RealTimeConnectivityEstimateWidget(QSharedPointer<SCMEASLIB::RealTimeConnectivityEstimate> &pRTCE, QWidget* parent)
: MeasurementWidget(parent)
, m_pRTCE(pRTCE)
, m_bInitialized(false)
, m_pRtItem(Q_NULLPTR)
, m_p3DView(View3D::SPtr(new View3D()))
, m_pData3DModel(Data3DTreeModel::SPtr(new Data3DTreeModel()))
, m_pQuickControlView(QuickControlView::SPtr::create("Connectivity Control", this))
{
    m_pActionQuickControl = new QAction(QIcon(":/images/quickControl.png"), tr("Show quick control widget"),this);
    m_pActionQuickControl->setStatusTip(tr("Show quick control widget"));
    connect(m_pActionQuickControl.data(), &QAction::triggered,
            this, &RealTimeConnectivityEstimateWidget::showQuickControlView);
    addDisplayAction(m_pActionQuickControl);
    m_pActionQuickControl->setVisible(true);

    //Init 3D View
    m_p3DView->setModel(m_pData3DModel);

    //Add 3D control to Quick control widget
    QStringList slControlFlags;
    slControlFlags << "Data" << "View" << "Light";
    m_pControl3DView = Control3DWidget::SPtr(new Control3DWidget(this, slControlFlags));
    m_pControl3DView->init(m_pData3DModel, m_p3DView);
    m_pQuickControlView->addGroupBox(m_pControl3DView.data(), "3D View");

    //Add other settings widgets
    QList<QWidget*> lControlWidgets = m_pRTCE->getControlWidgets();
    for(int i = 0; i < lControlWidgets.size(); i++) {
        m_pQuickControlView->addGroupBox(lControlWidgets.at(i), "Connectivity");
    }

    QGridLayout *mainLayoutView = new QGridLayout;
    QWidget *pWidgetContainer = this->createWindowContainer(m_p3DView.data(), this, Qt::Widget);

    mainLayoutView->addWidget(pWidgetContainer,0,0);

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
    if(m_bInitialized) {
        // Add rt brain data
        if(!m_pRtItem) {
            //qDebug()<<"RealTimeConnectivityEstimateWidget::getData - Creating m_pRtItem list";
            Network networkData = *(m_pRTCE->getValue().data());
            m_pRtItem = m_pData3DModel->addConnectivityData("sample",
                                                            networkData.getConnectivityMethod(),
                                                            networkData);

            if(m_pRTCE->getSurfSet() && m_pRTCE->getAnnotSet()) {
                m_pData3DModel->addSurfaceSet("sample",
                                              "MRI",
                                              *(m_pRTCE->getSurfSet().data()),
                                              *(m_pRTCE->getAnnotSet().data()));
            }

            if(m_pRTCE->getSensorSurface() && m_pRTCE->getFiffInfo()) {
                m_pData3DModel->addMegSensorInfo("sample",
                                                 "Sensors",
                                                 m_pRTCE->getFiffInfo()->chs,
                                                 *(m_pRTCE->getSensorSurface()));

            }
        } else {
            //qDebug()<<"RealTimeConnectivityEstimateWidget::getData - Working with m_pRtItem list";

            if(m_pRtItem) {
                m_pRtItem->addData(*(m_pRTCE->getValue().data()));
            }
        }
    } else {
        init();
    }
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::init()
{
    if(m_pRTCE->getAnnotSet() && m_pRTCE->getSurfSet() && m_pRTCE->getSensorSurface() && m_pRTCE->getFiffInfo()) {
        // Add brain data
        m_pData3DModel->addSurfaceSet("Subject", "MRI", *(m_pRTCE->getSurfSet()), *(m_pRTCE->getAnnotSet()));
        m_pData3DModel->addMegSensorInfo("Sensors", "VectorView", m_pRTCE->getFiffInfo()->chs, *(m_pRTCE->getSensorSurface()));
    } else {
        qDebug()<<"RealTimeConnectivityEstimateWidget::init - Could not open 3D surface information.";
    }

    m_bInitialized = true;
}


//*************************************************************************************************************

void RealTimeConnectivityEstimateWidget::showQuickControlView()
{
    if(m_pQuickControlView->isActiveWindow()) {
        m_pQuickControlView->hide();
    } else {
        m_pQuickControlView->activateWindow();
        m_pQuickControlView->show();
    }
}
