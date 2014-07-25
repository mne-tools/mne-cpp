//=============================================================================================================
/**
* @file     displaymanager.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2013
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
* @brief    Implementation of the DisplayManager Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "displaymanager.h"


#include <xDisp/realtimesamplearraywidget.h>
#include <xDisp/realtimemultisamplearraywidget.h>
#if defined(QT3D_LIBRARY_AVAILABLE)
#include <xDisp/realtimesourceestimatewidget.h>
#endif
#include <xDisp/realtimeevokedwidget.h>
#include <xDisp/noiseestimationwidget.h>

#include <xMeas/newrealtimesamplearray.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/realtimesourceestimate.h>
#include <xMeas/realtimeevoked.h>
#include <xMeas/noiseestimation.h>


//#include <xDisp/measurementwidget.h>
//#include <xDisp/numericwidget.h>
//#include <xDisp/realtimemultisamplearraywidget.h>
//#include <xDisp/realtimemultisamplearray_new_widget.h>
//#include <xDisp/realtimesourceestimatewidget.h>
//#include <xDisp/progressbarwidget.h>
//#include <xDisp/textwidget.h>

//#include <xMeas/Measurement/text.h>
//#include <xMeas/Measurement/realtimesourceestimate.h>
//#include <xMeas/Measurement/realtimemultisamplearray_new.h>
//#include <xMeas/Measurement/realtimemultisamplearray.h>
//#include <xMeas/Measurement/progressbar.h>
//#include <xMeas/Measurement/numeric.h>


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

using namespace MNEX;
using namespace XDISPLIB;
using namespace XMEASLIB;


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
        if(pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeSampleArray> >())
        {
            QSharedPointer<NewRealTimeSampleArray>* pRealTimeSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeSampleArray> >()->data();
            RealTimeSampleArrayWidget* rtsaWidget = new RealTimeSampleArrayWidget(*pRealTimeSampleArray, pT, newDisp);

            qListActions.append(rtsaWidget->getDisplayActions());
            qListWidgets.append(rtsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtsaWidget, &RealTimeSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtsaWidget);
            rtsaWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >())
        {
            QSharedPointer<NewRealTimeMultiSampleArray>* pNewRealTimeMultiSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >()->data();
            RealTimeMultiSampleArrayWidget* rtmsaWidget = new RealTimeMultiSampleArrayWidget(*pNewRealTimeMultiSampleArray, pT, newDisp);

            qListActions.append(rtmsaWidget->getDisplayActions());
            qListWidgets.append(rtmsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtmsaWidget, &RealTimeMultiSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtmsaWidget);
            rtmsaWidget->init();
        }
    #if defined(QT3D_LIBRARY_AVAILABLE)
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
    #endif
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeEvoked> >())
        {
            QSharedPointer<RealTimeEvoked>* pRealTimeEvoked = &pPluginOutputConnector.dynamicCast< PluginOutputData<RealTimeEvoked> >()->data();

            RealTimeEvokedWidget* rteWidget = new RealTimeEvokedWidget(*pRealTimeEvoked, pT, newDisp);

            qListActions.append(rteWidget->getDisplayActions());
            qListWidgets.append(rteWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rteWidget, &RealTimeEvokedWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rteWidget);
            rteWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<NoiseEstimation> >())
        {
            QSharedPointer<NoiseEstimation>* pNoiseEstimation = &pPluginOutputConnector.dynamicCast< PluginOutputData<NoiseEstimation> >()->data();

            NoiseEstimationWidget* neWidget = new NoiseEstimationWidget(*pNoiseEstimation, pT, newDisp);

            qListActions.append(neWidget->getDisplayActions());
            qListWidgets.append(neWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    neWidget, &NoiseEstimationWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(neWidget);
            neWidget->init();
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

