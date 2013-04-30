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

#include "rtsss.h"

#include <xMeas/Measurement/sngchnmeasurement.h>
#include <xMeas/Measurement/realtimesamplearray.h>
#include <xMeas/Measurement/realtimemultisamplearray_new.h>

#include <fiff/fiff_evoked.h>

#include "FormFiles/rtssssetupwidget.h"
#include "FormFiles/rtsssrunwidget.h"


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

using namespace RtSssPlugin;
using namespace FIFFLIB;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSss::RtSss()
: m_bIsRunning(false)
, m_bReceiveData(false)
{
    m_PLG_ID = PLG_ID::RTSSS;
}


//*************************************************************************************************************

RtSss::~RtSss()
{
    stop();
}


//*************************************************************************************************************

bool RtSss::start()
{
    // Initialize displaying widgets
    init();

    QThread::start();
    return true;
}


//*************************************************************************************************************

bool RtSss::stop()
{
    m_bIsRunning = false;

    // Stop threads
    QThread::terminate();
    QThread::wait();

    m_bReceiveData = false;

    return true;
}


//*************************************************************************************************************

Type RtSss::getType() const
{
    return _IRTAlgorithm;
}


//*************************************************************************************************************

const char* RtSss::getName() const
{
    return "Real-Time SSS/SSP";
}


//*************************************************************************************************************

QWidget* RtSss::setupWidget()
{
    RtSssSetupWidget* setupWidget = new RtSssSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

QWidget* RtSss::runWidget()
{
    RtSssRunWidget* runWidget = new RtSssRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return runWidget;
}


//*************************************************************************************************************

void RtSss::update(Subject* pSubject)
{
    Measurement* meas = static_cast<Measurement*>(pSubject);

    //MEG
    if(!meas->isSingleChannel() && m_bReceiveData)
    {
        RealTimeMultiSampleArrayNew* pRTMSANew = static_cast<RealTimeMultiSampleArrayNew*>(pSubject);


        if(pRTMSANew->getID() == MSR_ID::MEGRTSERVER_OUTPUT)
        {
            //Check if buffer initialized
            if(!m_pRtSssBuffer)
            {
                m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSANew->getNumChannels(), pRTMSANew->getMultiArraySize()));
                Buffer::SPtr t_buf = m_pRtSssBuffer.staticCast<Buffer>();// unix fix
                setAcceptorMeasurementBuffer(pRTMSANew->getID(), t_buf);
            }

            //Fiff information
            if(!m_pFiffInfo)
                m_pFiffInfo = pRTMSANew->getFiffInfo();

            MatrixXd t_mat(pRTMSANew->getNumChannels(), pRTMSANew->getMultiArraySize());

            //ToDo: Cast to specific Buffer
            for(unsigned char i = 0; i < pRTMSANew->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSANew->getMultiSampleArray()[i];

            getAcceptorMeasurementBuffer(pRTMSANew->getID()).staticCast<CircularMatrixBuffer<double> >()
                    ->push(&t_mat);
        }

    }
}


//*************************************************************************************************************

void RtSss::run()
{
    m_bIsRunning = true;

    //
    // start receiving data
    //
//    m_bReceiveData = true;

    //
    // Read Fiff Info
    //
//    while(!m_pFiffInfo)
//    {
//        msleep(10);
//        qDebug() << "Wait for fiff Info";
//    }

    //
    // Main thread loop
    //
    while(m_bIsRunning)
    {
//        qint32 nrows = m_pRtSssBuffer->rows();

//        if(nrows > 0) // check if init
//        {
//            /* Dispatch the inputs */
//            MatrixXd t_mat = m_pRtSssBuffer->pop();
//        }

        msleep(1000);//DEBUG
    }
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void RtSss::init()
{
    //Delete Buffer - will be initailzed with first incoming data
    if(m_pRtSssBuffer)
        m_pRtSssBuffer = CircularMatrixBuffer<double>::SPtr();

    qDebug() << "#### SourceLab Init; MEGRTSERVER_OUTPUT: " << MSR_ID::MEGRTSERVER_OUTPUT;

    this->addPlugin(PLG_ID::RTSERVER);
    Buffer::SPtr t_buf = m_pRtSssBuffer.staticCast<Buffer>(); //unix fix
    this->addAcceptorMeasurementBuffer(MSR_ID::MEGRTSERVER_OUTPUT, t_buf);

//    m_pDummy_MSA_Output = addProviderRealTimeMultiSampleArray(MSR_ID::DUMMYTOOL_OUTPUT_II, 2);
//    m_pDummy_MSA_Output->setName("Dummy Output II");
//    m_pDummy_MSA_Output->setUnit("mV");
//    m_pDummy_MSA_Output->setMinValue(-200);
//    m_pDummy_MSA_Output->setMaxValue(360);
//    m_pDummy_MSA_Output->setSamplingRate(256.0/1.0);

}
