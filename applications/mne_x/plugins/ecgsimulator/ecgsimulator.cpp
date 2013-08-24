//=============================================================================================================
/**
* @file     ecgsimulator.cpp
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
* @brief    Contains the implementation of the ECGSimulator class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgsimulator.h"
#include "ecgproducer.h"

#include "FormFiles/ecgsetupwidget.h"
#include "FormFiles/ecgrunwidget.h"


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

using namespace ECGSimulatorModule;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECGSimulator::ECGSimulator()
: m_pRTSA_ECG_I(0)
, m_pRTSA_ECG_II(0)
, m_pRTSA_ECG_III(0)
, m_fSamplingRate(250.0)
, m_iDownsamplingFactor(1)
, m_pInBuffer_I(new ECGBuffer_old(1024))
, m_pInBuffer_II(new ECGBuffer_old(1024))
, m_pInBuffer_III(new ECGBuffer_old(1024))
, m_pECGProducer(new ECGProducer(this, m_pInBuffer_I, m_pInBuffer_II, m_pInBuffer_III))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/ECGSimulator/")
, m_pECGChannel_ECG_I(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_I_256_s30661.txt")))
, m_pECGChannel_ECG_II(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_II_256_s30661.txt")))
, m_pECGChannel_ECG_III(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_III_256_s30661.txt")))
{
    m_PLG_ID = PLG_ID::ECGSIM;
}


//*************************************************************************************************************

ECGSimulator::~ECGSimulator()
{
    delete m_pInBuffer_I;
    delete m_pInBuffer_II;
    delete m_pInBuffer_III;
    delete m_pECGProducer;
}


//*************************************************************************************************************

bool ECGSimulator::start()
{

    m_pECGChannel_ECG_I->initChannel();
    m_pECGChannel_ECG_II->initChannel();
    m_pECGChannel_ECG_III->initChannel();

    // Initialize real time measurements
    init();

    // Start threads
    m_pECGProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool ECGSimulator::stop()
{
    // Stop threads
	m_pECGProducer->stop();
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pECGChannel_ECG_I->clear();
    m_pECGChannel_ECG_II->clear();
    m_pECGChannel_ECG_III->clear();

    m_pInBuffer_I->clear();
    m_pInBuffer_II->clear();
    m_pInBuffer_III->clear();

    return true;
}


//*************************************************************************************************************

Type ECGSimulator::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

const char* ECGSimulator::getName() const
{
    return "ECG Simulator";
}


//*************************************************************************************************************

QWidget* ECGSimulator::setupWidget()
{
    ECGSetupWidget* widget = new ECGSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog
    widget->initSamplingFactors();
    widget->initSelectedChannelFile();
    widget->initChannelStates();

    return widget;
}


//*************************************************************************************************************

QWidget* ECGSimulator::runWidget()
{
    ECGRunWidget* widget = new ECGRunWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return widget;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void ECGSimulator::init()
{
    qDebug() << "ECGSimulator::init()";

    if(m_pECGChannel_ECG_I->isEnabled())
    {
        double diff = m_pECGChannel_ECG_I->getMaximum() - m_pECGChannel_ECG_I->getMinimum();

        m_pRTSA_ECG_I = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_I);
        m_pRTSA_ECG_I->setName("ECG I");
        m_pRTSA_ECG_I->setUnit("mV");
        m_pRTSA_ECG_I->setMinValue(m_pECGChannel_ECG_I->getMinimum()-diff/10);
        m_pRTSA_ECG_I->setMaxValue(m_pECGChannel_ECG_I->getMaximum()+diff/10);
        m_pRTSA_ECG_I->setArraySize(10);
        m_pRTSA_ECG_I->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_I->setVisibility(m_pECGChannel_ECG_I->isVisible());
    }

    if(m_pECGChannel_ECG_II->isEnabled())
    {
        double diff = m_pECGChannel_ECG_II->getMaximum() - m_pECGChannel_ECG_II->getMinimum();
        m_pRTSA_ECG_II = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_II);
        m_pRTSA_ECG_II->setName("ECG II");
        m_pRTSA_ECG_II->setUnit("mV");
        m_pRTSA_ECG_II->setMinValue(m_pECGChannel_ECG_II->getMinimum()-diff/10);
        m_pRTSA_ECG_II->setMaxValue(m_pECGChannel_ECG_II->getMaximum()+diff/10);
        m_pRTSA_ECG_II->setArraySize(10);
        m_pRTSA_ECG_II->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_II->setVisibility(m_pECGChannel_ECG_II->isVisible());
    }

    if(m_pECGChannel_ECG_III->isEnabled())
    {
        double diff = m_pECGChannel_ECG_III->getMaximum() - m_pECGChannel_ECG_III->getMinimum();
        m_pRTSA_ECG_III = addProviderRealTimeSampleArray(MSR_ID::ECGSIM_III);
        m_pRTSA_ECG_III->setName("ECG III");
        m_pRTSA_ECG_III->setUnit("mV");
        m_pRTSA_ECG_III->setMinValue(m_pECGChannel_ECG_III->getMinimum()-diff/10);
        m_pRTSA_ECG_III->setMaxValue(m_pECGChannel_ECG_III->getMaximum()+diff/10);
        m_pRTSA_ECG_III->setArraySize(10);
        m_pRTSA_ECG_III->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_III->setVisibility(m_pECGChannel_ECG_III->isVisible());
    }
}


//*************************************************************************************************************

void ECGSimulator::run()
{
    double dValue_I = 0;
    double dValue_II = 0;
    double dValue_III = 0;

    while(true)
    {

        if(m_pECGChannel_ECG_I->isEnabled())
        {
            dValue_I = m_pInBuffer_I->pop();
            m_pRTSA_ECG_I->setValue(dValue_I);
        }
        if(m_pECGChannel_ECG_II->isEnabled())
        {
            dValue_II = m_pInBuffer_II->pop();
            m_pRTSA_ECG_II->setValue(dValue_II);
        }
        if(m_pECGChannel_ECG_III->isEnabled())
        {
            dValue_III = m_pInBuffer_III->pop();
            m_pRTSA_ECG_III->setValue(dValue_III);
        }
    }
}
