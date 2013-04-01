//=============================================================================================================
/**
* @file     rtserver.cpp
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
* @brief    Contains the implementation of the RTServer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtserver.h"
#include "rtserverproducer.h"

#include "FormFiles/rtserversetupwidget.h"
#include "FormFiles/rtserverrunwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTServerPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RTServer::RTServer()
: m_pRTSA_RTServer_I(0)
, m_pRTSA_RTServer_II(0)
, m_pRTSA_RTServer_III(0)
, m_fSamplingRate(250.0)
, m_iDownsamplingFactor(1)
, m_pInBuffer_I(new ECGBuffer_old(1024))
, m_pInBuffer_II(new ECGBuffer_old(1024))
, m_pInBuffer_III(new ECGBuffer_old(1024))
, m_pRTServerProducer(new RTServerProducer(this, m_pInBuffer_I, m_pInBuffer_II, m_pInBuffer_III))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/RTServer/")
, m_pRTServerChannel_ECG_I(new RTServerChannel(m_qStringResourcePath+"data/", QString("ECG_I_256_s30661.txt")))
, m_pRTServerChannel_ECG_II(new RTServerChannel(m_qStringResourcePath+"data/", QString("ECG_II_256_s30661.txt")))
, m_pRTServerChannel_ECG_III(new RTServerChannel(m_qStringResourcePath+"data/", QString("ECG_III_256_s30661.txt")))
{
    m_MDL_ID = MDL_ID::ECGSIM;



    m_pRtClient = new RtClient("127.0.0.1", this);

    // Start RtClient - ToDo just perform a rtserver check
    m_pRtClient->start();

}


//*************************************************************************************************************

RTServer::~RTServer()
{
    delete m_pInBuffer_I;
    delete m_pInBuffer_II;
    delete m_pInBuffer_III;
    delete m_pRTServerProducer;
}


//*************************************************************************************************************

bool RTServer::start()
{

    m_pRTServerChannel_ECG_I->initChannel();
    m_pRTServerChannel_ECG_II->initChannel();
    m_pRTServerChannel_ECG_III->initChannel();

    // Initialize real time measurements
    init();

    // Start threads
    m_pRTServerProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RTServer::stop()
{
    // Stop threads
    m_pRTServerProducer->stop();
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pRTServerChannel_ECG_I->clear();
    m_pRTServerChannel_ECG_II->clear();
    m_pRTServerChannel_ECG_III->clear();

    m_pInBuffer_I->clear();
    m_pInBuffer_II->clear();
    m_pInBuffer_III->clear();

    return true;
}


//*************************************************************************************************************

Type RTServer::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

const char* RTServer::getName() const
{
    return "RT Server";
}


//*************************************************************************************************************

QWidget* RTServer::setupWidget()
{
    RTServerSetupWidget* widget = new RTServerSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog
    widget->initSamplingFactors();
    widget->initSelectedChannelFile();
    widget->initChannelStates();

    return widget;
}


//*************************************************************************************************************

QWidget* RTServer::runWidget()
{
    RTServerRunWidget* widget = new RTServerRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return widget;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void RTServer::init()
{
    qDebug() << "RTServer::init()";

    if(m_pRTServerChannel_ECG_I->isEnabled())
    {
        double diff = m_pRTServerChannel_ECG_I->getMaximum() - m_pRTServerChannel_ECG_I->getMinimum();

        m_pRTSA_RTServer_I = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_I);
        m_pRTSA_RTServer_I->setName("ECG I");
        m_pRTSA_RTServer_I->setUnit("mV");
        m_pRTSA_RTServer_I->setMinValue(m_pRTServerChannel_ECG_I->getMinimum()-diff/10);
        m_pRTSA_RTServer_I->setMaxValue(m_pRTServerChannel_ECG_I->getMaximum()+diff/10);
        m_pRTSA_RTServer_I->setArraySize(10);
        m_pRTSA_RTServer_I->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_RTServer_I->setVisibility(m_pRTServerChannel_ECG_I->isVisible());
    }

    if(m_pRTServerChannel_ECG_II->isEnabled())
    {
        double diff = m_pRTServerChannel_ECG_II->getMaximum() - m_pRTServerChannel_ECG_II->getMinimum();
        m_pRTSA_RTServer_II = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_II);
        m_pRTSA_RTServer_II->setName("ECG II");
        m_pRTSA_RTServer_II->setUnit("mV");
        m_pRTSA_RTServer_II->setMinValue(m_pRTServerChannel_ECG_II->getMinimum()-diff/10);
        m_pRTSA_RTServer_II->setMaxValue(m_pRTServerChannel_ECG_II->getMaximum()+diff/10);
        m_pRTSA_RTServer_II->setArraySize(10);
        m_pRTSA_RTServer_II->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_RTServer_II->setVisibility(m_pRTServerChannel_ECG_II->isVisible());
    }

    if(m_pRTServerChannel_ECG_III->isEnabled())
    {
        double diff = m_pRTServerChannel_ECG_III->getMaximum() - m_pRTServerChannel_ECG_III->getMinimum();
        m_pRTSA_RTServer_III = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_III);
        m_pRTSA_RTServer_III->setName("ECG III");
        m_pRTSA_RTServer_III->setUnit("mV");
        m_pRTSA_RTServer_III->setMinValue(m_pRTServerChannel_ECG_III->getMinimum()-diff/10);
        m_pRTSA_RTServer_III->setMaxValue(m_pRTServerChannel_ECG_III->getMaximum()+diff/10);
        m_pRTSA_RTServer_III->setArraySize(10);
        m_pRTSA_RTServer_III->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_RTServer_III->setVisibility(m_pRTServerChannel_ECG_III->isVisible());
    }
}


//*************************************************************************************************************

void RTServer::run()
{
    double dValue_I = 0;
    double dValue_II = 0;
    double dValue_III = 0;

    while(true)
    {

        if(m_pRTServerChannel_ECG_I->isEnabled())
        {
            dValue_I = m_pInBuffer_I->pop();
            m_pRTSA_RTServer_I->setValue(dValue_I);
        }
        if(m_pRTServerChannel_ECG_II->isEnabled())
        {
            dValue_II = m_pInBuffer_II->pop();
            m_pRTSA_RTServer_II->setValue(dValue_II);
        }
        if(m_pRTServerChannel_ECG_III->isEnabled())
        {
            dValue_III = m_pInBuffer_III->pop();
            m_pRTSA_RTServer_III->setValue(dValue_III);
        }
    }
}
