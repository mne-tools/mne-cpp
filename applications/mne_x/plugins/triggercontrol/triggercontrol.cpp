//=============================================================================================================
/**
* @file     triggercontrol.cpp
* @author   Tim Kunze <tim.kunze@tu-ilmenau.de>
*           Luise Lang <luise.lang@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Tim Kunze, Luise Lang and Christoph Dinh. All rights reserved.
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
* @brief    Contains the implementation of the TriggerControlclass.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "triggercontrol.h"
#include "FormFiles/triggercontrolsetupwidget.h"

#include "serialport.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

#include <iostream>


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
, m_iBaud(115000)
, m_pSerialPort(new SerialPort) // initialize a new serial port
, m_iNumChs(0)
, m_bIsRunning(false)
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
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "TriggerControlInI", "TriggerControl input data I");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &TriggerControl::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

//    // Output
//    m_pTriggerOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "DummyOut", "Dummy output data");
//    m_outputConnectors.append(m_pTriggerOutput);

//    m_pTriggerOutput->data()->setName("Dummy Output");
//    m_pTriggerOutput->data()->setUnit("");
//    m_pTriggerOutput->data()->setMinValue(0);
//    m_pTriggerOutput->data()->setMaxValue(2);
//    m_pTriggerOutput->data()->setSamplingRate(256.0/1.0);

}


//*************************************************************************************************************

bool TriggerControl::start()
{
    QThread::start();
    m_bIsRunning = true;

    m_qTime.start();

    if(m_pSerialPort->open())   // open Serial Port
    {
        QByteArray t_data;
        t_data.append(0x01);
//        t_data[0] = t_data[0]|0x01;
        m_pSerialPort->sendData(t_data);
        std::cout << "data sent" << std::endl;
    }
    else
    {
        std::cout << "Sending not possible, please check settings" << std::endl;
    }


    return true;
}


//*************************************************************************************************************

bool TriggerControl::stop()
{
    m_pSerialPort->close();

    m_bIsRunning = false;

//    // Stop threads
//    QThread::terminate();
//    QThread::wait();

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
//    TriggerControlSetupWidget* setupWidget = new TriggerControlSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    QWidget* setupWidget = new QWidget;
    return setupWidget;
}


//*************************************************************************************************************

void TriggerControl::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        m_qMutex.lock();
        m_iNumChs = pRTMSA->getNumChannels();
        qint32 t_iSize = pRTMSA->getMultiSampleArray().size();
        for(qint32 i = 0; i < t_iSize; ++i)
            m_pData.append(pRTMSA->getMultiSampleArray()[i]);//Append sample wise
        m_qMutex.unlock();
    }
}



//*************************************************************************************************************

void TriggerControl::run()
{

    while(m_bIsRunning)
    {
        m_qMutex.lock();

        while(m_pData.size() > 0)
        {
            if(m_pData.first()[m_iNumChs-2] > 9999)
                qDebug() << "Time elpased: " << m_qTime.elapsed();

            m_pData.pop_front();
        }

        m_qMutex.unlock();
    }








//    int count = 0;
//    double v = 0;

//    while (true)
//    {
//        //ToDo: Implement your algorithm here

//        if( (count % 5 == 0) )
//            v = count % 2;

//        m_pTriggerOutput->data()->setValue(v);

//        msleep((1.0/256.0)*1000.0);
//        ++count;
//    }
}
