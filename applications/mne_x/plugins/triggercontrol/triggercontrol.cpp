//=============================================================================================================
/**
* @file     triggercontrol.cpp
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze and Christoph Dinh. All rights reserved.
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
* @brief    Contains the implementation of the TriggerControl class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggercontrol.h"
#include "FormFiles/triggercontrolsetupwidget.h"


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

using namespace TriggerControlPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TriggerControl::TriggerControl()
: m_pTriggerOutput(NULL)
{
}


//*************************************************************************************************************

TriggerControl::~TriggerControl()
{
    stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> TriggerControl::clone() const
{
    QSharedPointer<TriggerControl> pTriggerControlClone(new TriggerControl);
    return pTriggerControlClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void TriggerControl::init()
{
    // Input
//    m_pDummyInput = PluginInputData<NewRealTimeSampleArray>::create(this, "DummyIn", "Dummy input data");
//    connect(m_pDummyInput.data(), &PluginInputConnector::notify, this, &DummyToolbox::update, Qt::DirectConnection);
//    m_inputConnectors.append(m_pDummyInput);

    // Output
    m_pTriggerOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "DummyOut", "Dummy output data");
    m_outputConnectors.append(m_pTriggerOutput);

    m_pTriggerOutput->data()->setName("Dummy Output");
    m_pTriggerOutput->data()->setUnit("");
    m_pTriggerOutput->data()->setMinValue(0);
    m_pTriggerOutput->data()->setMaxValue(2);
    m_pTriggerOutput->data()->setSamplingRate(256.0/1.0);

}


//*************************************************************************************************************

bool TriggerControl::start()
{
    QThread::start();
    return true;
}


//*************************************************************************************************************

bool TriggerControl::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType TriggerControl::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString TriggerControl::getName() const
{
    return "Trigger Control";
}


//*************************************************************************************************************

QWidget* TriggerControl::setupWidget()
{
    TriggerControlSetupWidget* setupWidget = new TriggerControlSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void TriggerControl::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
//    QSharedPointer<NewRealTimeSampleArray> pRTSA = pMeasurement.dynamicCast<NewRealTimeSampleArray>();

//    if(pRTSA)
//    {
//        for(unsigned char i = 0; i < pRTSA->getArraySize(); ++i)
//        {
//            double value = pRTSA->getSampleArray()[i];
//            m_pDummyBuffer->push(value);
//        }
//    }
}



//*************************************************************************************************************

void TriggerControl::run()
{
    int count = 0;
    double v = 0;









    while (true)
    {
        //ToDo: Implement your algorithm here

        if( (count % 5 == 0) )
            v = count % 2;

        m_pTriggerOutput->data()->setValue(v);

        msleep((1.0/256.0)*1000.0);
        ++count;
    }
}

