//=============================================================================================================
/**
* @file     rtsss.cpp
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
* @brief    Contains the implementation of the RtSss class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

//#include "dummytoolbox.h"
//#include "FormFiles/dummysetupwidget.h"
#include "rtsss.h"
#include "FormFiles/rtssssetupwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//using namespace DummyToolboxPlugin;
using namespace RtSssPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSss::RtSss()
: m_pRTSAInput(NULL)
, m_pRTSAOutput(NULL)
, m_pRtSssBuffer(new dBuffer(1024))
/*
: m_pDummyInput(NULL)
, m_pDummyOutput(NULL)
, m_pDummyBuffer(new dBuffer(1024))
*/

{
}


//*************************************************************************************************************

RtSss::~RtSss()
{
    stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> RtSss::clone() const
{
    QSharedPointer<RtSss> pRtSssClone(new RtSss);
    return pRtSssClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void RtSss::init()
{
    // Input
    m_pRTSAInput = PluginInputData<NewRealTimeSampleArray>::create(this, "RtSssIn", "RtSss input data");
    connect(m_pRTSAInput.data(), &PluginInputConnector::notify, this, &RtSss::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSAInput);

    // Output
    m_pRTSAOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "RtSssOut", "RtSss output data");
    m_outputConnectors.append(m_pRTSAOutput);

    m_pRTSAOutput->data()->setName("RtSss Output");
    m_pRTSAOutput->data()->setUnit("mV");
    m_pRTSAOutput->data()->setMinValue(-200);
    m_pRTSAOutput->data()->setMaxValue(360);
    m_pRTSAOutput->data()->setSamplingRate(256.0/1.0);
}


//*************************************************************************************************************

bool RtSss::start()
{
    QThread::start();
    return true;
}


//*************************************************************************************************************

bool RtSss::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    m_pRtSssBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType RtSss::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString RtSss::getName() const
{
    return "RtSss Toolbox";
}


//*************************************************************************************************************

QWidget* RtSss::setupWidget()
{
    RtSssSetupWidget* setupWidget = new RtSssSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void RtSss::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeSampleArray> pRTSA = pMeasurement.dynamicCast<NewRealTimeSampleArray>();

    if(pRTSA)
    {
        for(unsigned char i = 0; i < pRTSA->getArraySize(); ++i)
        {
            double value = pRTSA->getSampleArray()[i];
            m_pRtSssBuffer->push(value);
        }
    }
}



//*************************************************************************************************************

void RtSss::run()
{
    while (true)
    {
        /* Dispatch the inputs */
        double v = m_pRtSssBuffer->pop();

        //ToDo: Implement your algorithm here

        m_pRTSAOutput->data()->setValue(v);
    }
}

