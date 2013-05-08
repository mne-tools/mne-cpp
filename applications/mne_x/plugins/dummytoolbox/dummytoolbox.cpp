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

#include <xMeas/Measurement/sngchnmeasurement.h>

#include <xMeas/Measurement/realtimesamplearray.h>

#include "FormFiles/dummysetupwidget.h"
#include "FormFiles/dummyrunwidget.h"


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
: m_pDummy_Output(0)
, m_pDummyBuffer(new CircularBuffer_old<double>(1024))
, m_pDummyMultiChannelBuffer(new CircularMultiChannelBuffer_old<double>(2, 1024))
{
    m_PLG_ID = PLG_ID::DUMMYTOOL;
}


//*************************************************************************************************************

DummyToolbox::~DummyToolbox()
{
    stop();
}


//*************************************************************************************************************

bool DummyToolbox::start()
{
    // Initialize displaying widgets
    init();

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

Type DummyToolbox::getType() const
{
    return _IRTAlgorithm;
}


//*************************************************************************************************************

const char* DummyToolbox::getName() const
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

QWidget* DummyToolbox::runWidget()
{
    DummyRunWidget* runWidget = new DummyRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return runWidget;
}


//*************************************************************************************************************

void DummyToolbox::update(Subject* pSubject)
{
    //donwsampled by arraysize of m_pRTSA -> then pick only the value
    bool downsampled = false;

    if(downsampled)
    {
        SngChnMeasurement* pMeasurement = static_cast<SngChnMeasurement*>(pSubject);

        //Using fast Hash Lookup instead of if then else clause
        if(getAcceptorMeasurementBuffer(pMeasurement->getID()))
        {
                //ToDo: Cast to specific Buffer
            getAcceptorMeasurementBuffer(pMeasurement->getID()).staticCast<DummyBuffer_old>()->push(pMeasurement->getValue());//if only every (arraysize)th value is necessary
        }
    }
    else
    {
        RealTimeSampleArray* pRTSA = static_cast<RealTimeSampleArray*>(pSubject);

        //Using fast Hash Lookup instead of if then else clause
        if(getAcceptorMeasurementBuffer(pRTSA->getID()))
        {
            if(pRTSA->getID() == MSR_ID::ECGSIM_I)
            {
                    //ToDo: Cast to specific Buffer
                for(unsigned char i = 0; i < pRTSA->getArraySize(); ++i)
                {
                    getAcceptorMeasurementBuffer(pRTSA->getID()).staticCast<DummyBuffer_old>()
                            ->push(pRTSA->getSampleArray()[i]);
                    //m_pDummyMultiChannelBuffer->push(0,pRTSA->getSampleArray()[i]);
                }
            }

            if(pRTSA->getID() == MSR_ID::ECGSIM_II)
            {
                    //ToDo: Cast to specific Buffer
                for(unsigned char i = 0; i < pRTSA->getArraySize(); ++i)
                {
                    //static_cast<_vector_double_CircularBuffer*>(m_pDummyMultiChannelBuffer/*getAcceptorMeasurementBuffer(pRTSA->getID())*/) //this is too complicated --> getAcceptorMeasurementBuffer was created to handle a bunch of channels... now with RTSM possible
//                    QVector<double> myVector;
//                    myVector.push_back(pRTSA->getSampleArray()[i]);

//                    qDebug() << myVector.at(0);

                    //m_pDummyMultiChannelBuffer->push(myVector);
                    m_pDummyMultiChannelBuffer->push(0,pRTSA->getSampleArray()[i]);
                    m_pDummyMultiChannelBuffer->push(1,pRTSA->getSampleArray()[i]);
                }
            }
		}
	}

}



//*************************************************************************************************************

void DummyToolbox::run()
{
    while (true)
    {
//        double v_one = m_pDummyMultiChannelBuffer->pop(0);

//        double v_two = m_pDummyMultiChannelBuffer->pop(1);

        QVector<double> vec = m_pDummyMultiChannelBuffer->pop();

        m_pDummy_MSA_Output->setVector(vec);

        /* Dispatch the inputs */

        double v = m_pDummyBuffer->pop();

        //ToDo: Implement here the algorithm

        m_pDummy_Output->setValue(v);
    }
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void DummyToolbox::init()
{
    qDebug() << "#### DummyToolbox Init; MSR_ECG_I: " << MSR_ID::ECGSIM_I;


    this->addPlugin(PLG_ID::ECGSIM); //ToDo This should be obsolete -  measurement ID should be sufficient -> solve this by adding measurement IDs to subject?? attach observers to subjects with corresponding ID
    Buffer::SPtr t_buf = m_pDummyBuffer.staticCast<Buffer>(); //unix fix
    this->addAcceptorMeasurementBuffer(MSR_ID::ECGSIM_I, t_buf);

    m_pDummy_Output = addProviderRealTimeSampleArray(MSR_ID::DUMMYTOOL_OUTPUT);
    m_pDummy_Output->setName("Dummy Output");
    m_pDummy_Output->setUnit("mV");
    m_pDummy_Output->setMinValue(-200);
    m_pDummy_Output->setMaxValue(360);
    m_pDummy_Output->setSamplingRate(256.0/1.0);


    t_buf = m_pDummyMultiChannelBuffer.staticCast<Buffer>(); //unix fix
    this->addAcceptorMeasurementBuffer(MSR_ID::ECGSIM_II, t_buf);

    m_pDummy_MSA_Output = addProviderRealTimeMultiSampleArray(MSR_ID::DUMMYTOOL_OUTPUT_II, 2);
    m_pDummy_MSA_Output->setName("Dummy Output II");
    m_pDummy_MSA_Output->setUnit("mV");
    m_pDummy_MSA_Output->setMinValue(-200);
    m_pDummy_MSA_Output->setMaxValue(360);
    m_pDummy_MSA_Output->setSamplingRate(256.0/1.0);

}
