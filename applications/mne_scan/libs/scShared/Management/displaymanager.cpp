//=============================================================================================================
/**
 * @file     displaymanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the DisplayManager Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "displaymanager.h"

#include <scDisp/realtimemultisamplearraywidget.h>
#include <scDisp/realtime3dwidget.h>
#include <scDisp/realtimeevokedsetwidget.h>
#include <scDisp/realtimecovwidget.h>
#include <scDisp/realtimespectrumwidget.h>
#include <scDisp/realtimeneurofeedbackwidget.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimespectrum.h>
#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimeneurofeedbackresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCDISPLIB;
using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DisplayManager::DisplayManager(QObject* parent)
: QObject(parent)
{
}

//=============================================================================================================

DisplayManager::~DisplayManager()
{
    clean();
}

//=============================================================================================================

QWidget* DisplayManager::show(AbstractPlugin::OutputConnectorList &outputConnectorList,
                              QSharedPointer<QTime>& pT,
                              QList< QAction* >& qListActions)
{
    QWidget* newDisp = new QWidget;
    QVBoxLayout* vboxLayout = new QVBoxLayout;

    foreach (QSharedPointer< PluginOutputConnector > pPluginOutputConnector, outputConnectorList) {
        if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeMultiSampleArray> >()) {
            RealTimeMultiSampleArrayWidget* rtmsaWidget = new RealTimeMultiSampleArrayWidget(pT, newDisp);

            qListActions.append(rtmsaWidget->getDisplayActions());

            // We need to use queued connection here because, e.g., the FiffSimulator is dispatching its data from the main thread
            // and a blocking one because the data is deleted immediatley after the signal was emmited
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtmsaWidget, &RealTimeMultiSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtmsaWidget);
            rtmsaWidget->init();
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSourceEstimate> >()) {
            if(!m_pRealTime3DWidget) {
                m_pRealTime3DWidget = new RealTime3DWidget(newDisp);
                vboxLayout->addWidget(m_pRealTime3DWidget);
                m_pRealTime3DWidget->init();
            }

            qListActions.append(m_pRealTime3DWidget->getDisplayActions());

            // We need to use queued connection here because the RealTimeSourceEstimate measurement
            // only holds one measurment and overwrites it immediatley after it emmited notify
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    m_pRealTime3DWidget.data(), &RealTime3DWidget::update, Qt::BlockingQueuedConnection);
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeConnectivityEstimate> >()) {
            if(!m_pRealTime3DWidget) {
                m_pRealTime3DWidget = new RealTime3DWidget(newDisp);
                vboxLayout->addWidget(m_pRealTime3DWidget);
                m_pRealTime3DWidget->init();
            }

            qListActions.append(m_pRealTime3DWidget->getDisplayActions());

            // We need to use queued connection here because the RealTimeConnectivityEstimate measurement
            // only holds one measurment and overwrites it immediatley after it emmited notify
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    m_pRealTime3DWidget.data(), &RealTime3DWidget::update, Qt::BlockingQueuedConnection);
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeHpiResult> >()) {
            if(!m_pRealTime3DWidget) {
                m_pRealTime3DWidget = new RealTime3DWidget(newDisp);
                vboxLayout->addWidget(m_pRealTime3DWidget);
                m_pRealTime3DWidget->init();
            }

            qListActions.append(m_pRealTime3DWidget->getDisplayActions());

            // We need to use queued connection here because the RealTimeHpiResult measurement
            // only holds one measurment and overwrites it immediatley after it emmited notify
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    m_pRealTime3DWidget.data(), &RealTime3DWidget::update, Qt::BlockingQueuedConnection);
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeEvokedSet> >()) {
            RealTimeEvokedSetWidget* rtesWidget = new RealTimeEvokedSetWidget(pT, newDisp);

            qListActions.append(rtesWidget->getDisplayActions());

            // We need to use queued connection here because the RealTimeEvokedSet measurement
            // only holds one measurement (of multiple evoked responses) and overwrites it immediatley after it emmited notify
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtesWidget, &RealTimeEvokedSetWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtesWidget);
            rtesWidget->init();
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeCov> >()) {
            RealTimeCovWidget* rtcWidget = new RealTimeCovWidget(pT, newDisp);

            qListActions.append(rtcWidget->getDisplayActions());

            // We need to use queued connection here because the RealTimeCovWidget measurement
            // only holds one measurement and overwrites it immediatley after it emmited notify
            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtcWidget, &RealTimeCovWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtcWidget);
            rtcWidget->init();
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSpectrum> >()) {
            QSharedPointer<RealTimeSpectrum> pRealTimeSpectrum = pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSpectrum> >()->measurementData();

            RealTimeSpectrumWidget* fsWidget = new RealTimeSpectrumWidget(pRealTimeSpectrum, pT, newDisp);

            qListActions.append(fsWidget->getDisplayActions());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    fsWidget, &RealTimeSpectrumWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(fsWidget);
            fsWidget->init();
        } else if (pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeNeurofeedbackResult> >()) {

            RealTimeNeurofeedbackWidget* nfWidget = new RealTimeNeurofeedbackWidget(pT, newDisp);

            qListActions.append(nfWidget->getDisplayActions());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    nfWidget, &RealTimeNeurofeedbackWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(nfWidget);
            nfWidget->init();
        }
    }

    newDisp->setLayout(vboxLayout);

    // If no display was attached return NULL pointer
    if(vboxLayout->count() == 0) {
        delete newDisp;
        newDisp = Q_NULLPTR;
    }

    return newDisp;
}

//=============================================================================================================

void DisplayManager::clean()
{
    qDebug() << "DisplayManager::clean()";
}

