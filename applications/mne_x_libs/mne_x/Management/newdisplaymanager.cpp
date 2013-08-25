//=============================================================================================================
/**
* @file     newdisplaymanager.cpp
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
* @brief    Implementation of the NewDisplayManager Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "newdisplaymanager.h"
//#include <xDisp/measurementwidget.h>
//#include <xDisp/numericwidget.h>
#include <xDisp/newrealtimesamplearraywidget.h>
//#include <xDisp/realtimemultisamplearraywidget.h>
//#include <xDisp/realtimemultisamplearray_new_widget.h>
//#include <xDisp/realtimesourceestimatewidget.h>
//#include <xDisp/progressbarwidget.h>
//#include <xDisp/textwidget.h>

//#include <xMeas/Measurement/text.h>
//#include <xMeas/Measurement/realtimesourceestimate.h>
//#include <xMeas/Measurement/realtimemultisamplearray_new.h>
//#include <xMeas/Measurement/realtimemultisamplearray.h>
#include <xMeas/Measurement/newrealtimesamplearray.h>
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

NewDisplayManager::NewDisplayManager(QSharedPointer<QTime> pT, QObject* parent)
: QObject(parent)
, m_pT(pT)
{

}


//*************************************************************************************************************

NewDisplayManager::~NewDisplayManager()
{
    clean();
}


//*************************************************************************************************************

void NewDisplayManager::init()
{
//    qDebug() << "DisplayManager::init(): s_hashMeasurementWidgets.size() = " << s_hashMeasurementWidgets.size();

//    foreach(MeasurementWidget* pMSRW, s_hashMeasurementWidgets.values())
//        pMSRW->init();
}


//*************************************************************************************************************

QWidget* NewDisplayManager::show(IPlugin::OutputConnectorList &pOutputConnectorList )
{
    qDebug() << "DisplayManager::show()";

    QWidget* newDisp = new QWidget;

    QVBoxLayout* vboxLayout = new QVBoxLayout;

    QHBoxLayout* hboxLayout = new QHBoxLayout;

    foreach (QSharedPointer< PluginOutputConnector > pPluginOutputConnector, pOutputConnectorList)
    {
        if(pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeSampleArray> >())
        {
            qWarning() << "RTSA found!";

            QSharedPointer<NewRealTimeSampleArray> pRTSA = pPluginOutputConnector.dynamicCast< PluginOutputData<NewRealTimeSampleArray> >()->data();

            NewRealTimeSampleArrayWidget* rtsaWidget = new NewRealTimeSampleArrayWidget(pRTSA, m_pT);

            connect(pPluginOutputConnector.data(), &PluginOutputConnector::notify, rtsaWidget, &NewRealTimeSampleArrayWidget::update);

//            vboxLayout->addWidget(rtsaWidget);
//            rtsaWidget->show();
//            rtsaWidget->init();
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

    // Initialize all MeasurementWidgets
    init();

    return newDisp;
}


//*************************************************************************************************************

void NewDisplayManager::clean()
{
    qDebug() << "DisplayManager::clean()";
}

