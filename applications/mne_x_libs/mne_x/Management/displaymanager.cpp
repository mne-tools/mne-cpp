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

#define NEWRTMSAWIDGET 1

#include "displaymanager.h"


#include <xDisp/newrealtimesamplearraywidget.h>
#ifdef NEWRTMSAWIDGET
#include <xDisp/realtimemultisamplearraywidget.h>
#else
#include <xDisp/newrealtimemultisamplearraywidget.h>
#endif

#if defined(QT3D_LIBRARY_AVAILABLE)
#include <xDisp/realtimesourceestimatewidget.h>
#endif

#include <xMeas/newrealtimesamplearray.h>
#include <xMeas/newrealtimemultisamplearray.h>

#include <xMeas/realtimesourceestimate.h>


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
            QSharedPointer<NewRealTimeSampleArray>* pNewRealTimeSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeSampleArray> >()->data();
            NewRealTimeSampleArrayWidget* rtsaWidget = new NewRealTimeSampleArrayWidget(*pNewRealTimeSampleArray, pT, newDisp);

            qListActions.append(rtsaWidget->getDisplayActions());
            qListWidgets.append(rtsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtsaWidget, &NewRealTimeSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtsaWidget);
            rtsaWidget->init();
        }
        else if(pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >())
        {
            QSharedPointer<NewRealTimeMultiSampleArray>* pNewRealTimeMultiSampleArray = &pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeMultiSampleArray> >()->data();
#ifdef NEWRTMSAWIDGET
            RealTimeMultiSampleArrayWidget* rtmsaWidget = new RealTimeMultiSampleArrayWidget(*pNewRealTimeMultiSampleArray, pT, newDisp);

            qListActions.append(rtmsaWidget->getDisplayActions());
            qListWidgets.append(rtmsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtmsaWidget, &RealTimeMultiSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtmsaWidget);
            rtmsaWidget->init();
#else
            NewRealTimeMultiSampleArrayWidget* rtmsaWidget = new NewRealTimeMultiSampleArrayWidget(*pNewRealTimeMultiSampleArray, pT, newDisp);

            qListActions.append(rtmsaWidget->getDisplayActions());
            qListWidgets.append(rtmsaWidget->getDisplayWidgets());

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify,
                    rtmsaWidget, &NewRealTimeMultiSampleArrayWidget::update, Qt::BlockingQueuedConnection);

            vboxLayout->addWidget(rtmsaWidget);
            rtmsaWidget->init();
#endif
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

