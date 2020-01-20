//=============================================================================================================
/**
 * @file     displaymanager.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "displaymanager.h"

#include <scDisp/realtimesamplearraywidget.h>
#include <scDisp/realtimemultisamplearraywidget.h>
#include <scDisp/realtimesourceestimatewidget.h>
#include <scDisp/realtimeconnectivityestimatewidget.h>
#include <scDisp/realtimeevokedsetwidget.h>
#include <scDisp/realtimecovwidget.h>
#include <scDisp/realtimespectrumwidget.h>

#include <scMeas/realtimesamplearray.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimespectrum.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCDISPLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DisplayManager::DisplayManager(QObject* parent)
: QObject(parent)
{

}


//*************************************************************************************************************

DisplayManager::~DisplayManager()
{
    clean();
}


//*************************************************************************************************************

QWidget* DisplayManager::show(IPlugin::OutputConnectorList &outputConnectorList, QSharedPointer<QTime>& pT, QList< QAction* >& qListActions, QList< QWidget* >& qListWidgets)
{
    QWidget* newDisp = new QWidget;
    QVBoxLayout* vboxLayout = new QVBoxLayout;
    QHBoxLayout* hboxLayout = new QHBoxLayout;

    qListActions.clear();

    foreach (QSharedPointer< PluginOutputConnector > pPluginOutputConnector, outputConnectorList)
    {
        if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSampleArray> >())
        {
            QSharedPointer<RealTimeSampleArray>* pRealTimeSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSampleArray> >()->data();
            RealTimeSampleArrayWidget* rtsaWidget = new RealTimeSampleArrayWidget(*pRealTimeSampleArray, pT, newDisp);

            qListActions.append(rtsaWidget->getDisplayActions());
            qListWidgets.append(rtsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtsaWidget, &RealTimeSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtsaWidget);
            rtsaWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeMultiSampleArray> >())
        {
            QSharedPointer<RealTimeMultiSampleArray>* pRealTimeMultiSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeMultiSampleArray> >()->data();
            RealTimeMultiSampleArrayWidget* rtmsaWidget = new RealTimeMultiSampleArrayWidget(*pRealTimeMultiSampleArray, pT, newDisp);

            qListActions.append(rtmsaWidget->getDisplayActions());
            qListWidgets.append(rtmsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtmsaWidget, &RealTimeMultiSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtmsaWidget);
            rtmsaWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSourceEstimate> >())
        {
            QSharedPointer<RealTimeSourceEstimate>* pRealTimeSourceEstimate = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSourceEstimate> >()->data();
            RealTimeSourceEstimateWidget* rtseWidget = new RealTimeSourceEstimateWidget(*pRealTimeSourceEstimate, newDisp);//new RealTimeSourceEstimateWidget(*pRealTimeSourceEstimate, pT, newDisp);

            qListActions.append(rtseWidget->getDisplayActions());
            qListWidgets.append(rtseWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtseWidget, &RealTimeSourceEstimateWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtseWidget);
            rtseWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeConnectivityEstimate> >())
        {
            QSharedPointer<RealTimeConnectivityEstimate>* pRealTimeConnectivityEstimate = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeConnectivityEstimate> >()->data();
            RealTimeConnectivityEstimateWidget* rtseWidget = new RealTimeConnectivityEstimateWidget(*pRealTimeConnectivityEstimate, newDisp);//new RealTimeConnectivityEstimateWidget(*pRealTimeConnectivityEstimate, pT, newDisp);

            qListActions.append(rtseWidget->getDisplayActions());
            qListWidgets.append(rtseWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtseWidget, &RealTimeConnectivityEstimateWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtseWidget);
            rtseWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeEvokedSet> >())
        {
            QSharedPointer<RealTimeEvokedSet>* pRealTimeEvokedSet = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeEvokedSet> >()->data();

            RealTimeEvokedSetWidget* rtesWidget = new RealTimeEvokedSetWidget(*pRealTimeEvokedSet, pT, newDisp);

            qListActions.append(rtesWidget->getDisplayActions());
            qListWidgets.append(rtesWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtesWidget, &RealTimeEvokedSetWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtesWidget);
            rtesWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeCov> >())
        {
            QSharedPointer<RealTimeCov>* pRealTimeCov = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeCov> >()->data();

            RealTimeCovWidget* rtcWidget = new RealTimeCovWidget(*pRealTimeCov, pT, newDisp);

            qListActions.append(rtcWidget->getDisplayActions());
            qListWidgets.append(rtcWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtcWidget, &RealTimeCovWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtcWidget);
            rtcWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSpectrum> >())
        {
            QSharedPointer<RealTimeSpectrum>* pRealTimeSpectrum = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeSpectrum> >()->data();

            RealTimeSpectrumWidget* fsWidget = new RealTimeSpectrumWidget(*pRealTimeSpectrum, pT, newDisp);

            qListActions.append(fsWidget->getDisplayActions());
            qListWidgets.append(fsWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    fsWidget, &RealTimeSpectrumWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(fsWidget);
            fsWidget->init();
        }
    }

//    // Add all widgets but NumericWidgets to layout and display them
//    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
//    {
//        if(dynamic_cast<NumericWidget*>(pMSRW))
//            continue;
//        vboxLayout->addWidget(pMSRW);
//        pMSRW->show();
//    }

//    foreach(NumericWidget* pNUMW, s_hashNumericWidgets.values())
//    {
//        hboxLayout->addWidget(pNUMW);
//        pNUMW->show();
//    }

    vboxLayout->addLayout(hboxLayout);
    newDisp->setLayout(vboxLayout);

    return newDisp;
}


//*************************************************************************************************************

void DisplayManager::clean()
{
    qDebug() << "DisplayManager::clean()";
}

