//=============================================================================================================
/**
* @file     sourcelab.cpp
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
* @brief    Contains the implementation of the SourceLab class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcelab.h"

#include <xMeas/Measurement/sngchnmeasurement.h>

#include <xMeas/Measurement/realtimesamplearray.h>

#include "FormFiles/sourcelabsetupwidget.h"
#include "FormFiles/sourcelabrunwidget.h"


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

using namespace SourceLabPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceLab::SourceLab()
: m_pSourceLab_Output(0)
, m_pSourceLabBuffer(new _double_CircularBuffer_old(1024))
{
    m_PLG_ID = PLG_ID::SOURCELAB;
}


//*************************************************************************************************************

SourceLab::~SourceLab()
{
    stop();

    delete m_pSourceLabBuffer;
}


//*************************************************************************************************************

bool SourceLab::start()
{
    // Initialize displaying widgets
    init();

    QThread::start();
    return true;
}


//*************************************************************************************************************

bool SourceLab::stop()
{
    // Stop threads
    QThread::terminate();
    QThread::wait();

    m_pSourceLabBuffer->clear();

    return true;
}


//*************************************************************************************************************

Type SourceLab::getType() const
{
    return _IRTAlgorithm;
}


//*************************************************************************************************************

const char* SourceLab::getName() const
{
    return "Source Lab";
}


//*************************************************************************************************************

QWidget* SourceLab::setupWidget()
{
    SourceLabSetupWidget* setupWidget = new SourceLabSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

QWidget* SourceLab::runWidget()
{
    SourceLabRunWidget* runWidget = new SourceLabRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return runWidget;
}


//*************************************************************************************************************

void SourceLab::update(Subject* pSubject)
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
            static_cast<DummyBuffer_old*>(getAcceptorMeasurementBuffer(pMeasurement->getID()))->push(pMeasurement->getValue());//if only every (arraysize)th value is necessary
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
                    static_cast<DummyBuffer_old*>(getAcceptorMeasurementBuffer(pRTSA->getID()))
                            ->push(pRTSA->getSampleArray()[i]);
                    //m_pDummyMultiChannelBuffer->push(0,pRTSA->getSampleArray()[i]);
                }
            }
        }
    }
}



//*************************************************************************************************************

void SourceLab::run()
{
    while (true)
    {
//        double v_one = m_pDummyMultiChannelBuffer->pop(0);

//        double v_two = m_pDummyMultiChannelBuffer->pop(1);


        /* Dispatch the inputs */

        double v = m_pSourceLabBuffer->pop();

        //ToDo: Implement here the algorithm

        m_pSourceLab_Output->setValue(v);
    }
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void SourceLab::init()
{
    qDebug() << "#### DummyToolbox Init; MSR_ECG_I: " << MSR_ID::ECGSIM_I;


    this->addPlugin(PLG_ID::ECGSIM); //ToDo This should be obsolete -  measurement ID should be sufficient -> solve this by adding measurement IDs to subject?? attach observers to subjects with corresponding ID
    this->addAcceptorMeasurementBuffer(MSR_ID::ECGSIM_I, m_pSourceLabBuffer);

    m_pSourceLab_Output = addProviderRealTimeSampleArray(MSR_ID::SOURCELAB_OUTPUT);
    m_pSourceLab_Output->setName("Source Lab Output");
    m_pSourceLab_Output->setUnit("mV");
    m_pSourceLab_Output->setMinValue(-200);
    m_pSourceLab_Output->setMaxValue(360);
    m_pSourceLab_Output->setSamplingRate(256.0/1.0);
}
