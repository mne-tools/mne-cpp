//=============================================================================================================
/**
* @file     dummytoolbox.cpp
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
* @brief    Contains the implementation of the DummyToolbox class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummytoolbox.h"
#include "FormFiles/dummysetupwidget.h"


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

using namespace DummyToolboxPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DummyToolbox::DummyToolbox()
: m_pDummyInput(NULL)
, m_pDummyOutput(NULL)
, m_pDummyBuffer(new dBuffer(1024))
{
}


//*************************************************************************************************************

DummyToolbox::~DummyToolbox()
{
    stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> DummyToolbox::clone() const
{
    QSharedPointer<DummyToolbox> pDummyToolboxClone(new DummyToolbox);
    return pDummyToolboxClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void DummyToolbox::init()
{
    // Input
    m_pDummyInput = PluginInputData<NewRealTimeSampleArray>::create(this, "DummyIn", "Dummy input data");
    connect(m_pDummyInput.data(), &PluginInputConnector::notify, this, &DummyToolbox::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pDummyInput);

    // Output
    m_pDummyOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "DummyOut", "Dummy output data");
    m_outputConnectors.append(m_pDummyOutput);

    m_pDummyOutput->data()->setName("Dummy Output");
    m_pDummyOutput->data()->setUnit("mV");
    m_pDummyOutput->data()->setMinValue(-200);
    m_pDummyOutput->data()->setMaxValue(360);
    m_pDummyOutput->data()->setSamplingRate(256.0/1.0);
}


//*************************************************************************************************************

bool DummyToolbox::start()
{
    QThread::start();
    return true;
}


//*************************************************************************************************************

bool DummyToolbox::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    m_pDummyBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType DummyToolbox::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString DummyToolbox::getName() const
{
    return "Dummy Toolbox";
}


//*************************************************************************************************************

QWidget* DummyToolbox::setupWidget()
{
    DummySetupWidget* setupWidget = new DummySetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void DummyToolbox::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeSampleArray> pRTSA = pMeasurement.dynamicCast<NewRealTimeSampleArray>();

    if(!pRTSA.isNull())
    {
        for(unsigned char i = 0; i < pRTSA->getArraySize(); ++i)
        {
            double value = pRTSA->getSampleArray()[i];
            m_pDummyBuffer->push(value);
        }
    }
}



//*************************************************************************************************************

void DummyToolbox::run()
{
    while (true)
    {
        /* Dispatch the inputs */
        double v = m_pDummyBuffer->pop();

        //ToDo: Implement your algorithm here

        m_pDummyOutput->data()->setValue(v);
    }
}

