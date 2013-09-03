//=============================================================================================================
/**
* @file     tmsi.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSI class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi.h"
#include "tmsiproducer.h"

#include "FormFiles/tmsisetupwidget.h"

#include <iostream>


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

using namespace TMSIPlugin;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSI::TMSI()
: m_pRTSA_TMSI_I_new(0)
, m_fSamplingRate(250.0)
, m_iDownsamplingFactor(1)
, m_pInBuffer_I(new dBuffer(1024))
, m_pInBuffer_II(new dBuffer(1024))
, m_pInBuffer_III(new dBuffer(1024))
, m_pTMSIProducer(new TMSIProducer(this, m_pInBuffer_I, m_pInBuffer_II, m_pInBuffer_III))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/ECGSimulator/")
, m_pTMSIChannel_TMSI_I(new TMSIChannel(m_qStringResourcePath+"data/", QString("ECG_I_256_s30661.txt")))
, m_pTMSIChannel_TMSI_II(new TMSIChannel(m_qStringResourcePath+"data/", QString("ECG_II_256_s30661.txt")))
, m_pTMSIChannel_TMSI_III(new TMSIChannel(m_qStringResourcePath+"data/", QString("ECG_III_256_s30661.txt")))
{
}


//*************************************************************************************************************

TMSI::~TMSI()
{
    qWarning() << "TMSI::~TMSI()";
}


//*************************************************************************************************************

QSharedPointer<IPlugin> TMSI::clone() const
{
    QSharedPointer<TMSI> pTMSIClone(new TMSI());
    return pTMSIClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================
void TMSI::init()
{
    if(m_pTMSIChannel_TMSI_I->isEnabled())
    {
        m_pRTSA_TMSI_I_new = PluginOutputData<NewRealTimeSampleArray>::create(this, "ECG I", "ECG I output data");
        m_outputConnectors.append(m_pRTSA_TMSI_I_new);
    }

    if(m_pTMSIChannel_TMSI_II->isEnabled())
    {
        m_pRTSA_TMSI_II_new = PluginOutputData<NewRealTimeSampleArray>::create(this, "ECG II", "ECG II output data");
        m_outputConnectors.append(m_pRTSA_TMSI_II_new);
    }

    if(m_pTMSIChannel_TMSI_III->isEnabled())
    {
        m_pRTSA_TMSI_III_new = PluginOutputData<NewRealTimeSampleArray>::create(this, "ECG III", "ECG III output data");
        m_outputConnectors.append(m_pRTSA_TMSI_III_new);
    }
}


//*************************************************************************************************************

void TMSI::initChannels()
{
    m_pTMSIChannel_TMSI_I->initChannel();
    m_pTMSIChannel_TMSI_II->initChannel();
    m_pTMSIChannel_TMSI_III->initChannel();

    if(m_pTMSIChannel_TMSI_I->isEnabled())
    {
        double diff = m_pTMSIChannel_TMSI_I->getMaximum() - m_pTMSIChannel_TMSI_I->getMinimum();

        m_pRTSA_TMSI_I_new->data()->setName("ECG I");
        m_pRTSA_TMSI_I_new->data()->setUnit("mV");

        m_pRTSA_TMSI_I_new->data()->setMinValue(m_pTMSIChannel_TMSI_I->getMinimum()-diff/10);
        m_pRTSA_TMSI_I_new->data()->setMaxValue(m_pTMSIChannel_TMSI_I->getMaximum()+diff/10);
        m_pRTSA_TMSI_I_new->data()->setArraySize(10);
        m_pRTSA_TMSI_I_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_TMSI_I_new->data()->setVisibility(m_pTMSIChannel_TMSI_I->isVisible());
    }

    if(m_pTMSIChannel_TMSI_II->isEnabled())
    {
        double diff = m_pTMSIChannel_TMSI_II->getMaximum() - m_pTMSIChannel_TMSI_II->getMinimum();

        m_pRTSA_TMSI_II_new->data()->setName("ECG II");
        m_pRTSA_TMSI_II_new->data()->setUnit("mV");

        m_pRTSA_TMSI_II_new->data()->setMinValue(m_pTMSIChannel_TMSI_II->getMinimum()-diff/10);
        m_pRTSA_TMSI_II_new->data()->setMaxValue(m_pTMSIChannel_TMSI_II->getMaximum()+diff/10);
        m_pRTSA_TMSI_II_new->data()->setArraySize(10);
        m_pRTSA_TMSI_II_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_TMSI_II_new->data()->setVisibility(m_pTMSIChannel_TMSI_II->isVisible());
    }

    if(m_pTMSIChannel_TMSI_III->isEnabled())
    {
        double diff = m_pTMSIChannel_TMSI_III->getMaximum() - m_pTMSIChannel_TMSI_III->getMinimum();

        m_pRTSA_TMSI_III_new->data()->setName("ECG III");
        m_pRTSA_TMSI_III_new->data()->setUnit("mV");

        m_pRTSA_TMSI_III_new->data()->setMinValue(m_pTMSIChannel_TMSI_III->getMinimum()-diff/10);
        m_pRTSA_TMSI_III_new->data()->setMaxValue(m_pTMSIChannel_TMSI_III->getMaximum()+diff/10);
        m_pRTSA_TMSI_III_new->data()->setArraySize(10);
        m_pRTSA_TMSI_III_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_TMSI_III_new->data()->setVisibility(m_pTMSIChannel_TMSI_III->isVisible());
    }
}


//*************************************************************************************************************

bool TMSI::start()
{
    initChannels();

    // Start threads
    m_pTMSIProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool TMSI::stop()
{
    // Stop threads
    m_pTMSIProducer->stop();
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pTMSIChannel_TMSI_I->clear();
    m_pTMSIChannel_TMSI_II->clear();
    m_pTMSIChannel_TMSI_III->clear();

    m_pInBuffer_I->clear();
    m_pInBuffer_II->clear();
    m_pInBuffer_III->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType TMSI::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString TMSI::getName() const
{
    return "TMSI EEG";
}


//*************************************************************************************************************

QWidget* TMSI::setupWidget()
{
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init dialog
    widget->initSamplingFactors();
    widget->initSelectedChannelFile();
    widget->initChannelStates();

    return widget;
}


//*************************************************************************************************************

void TMSI::run()
{
    double dValue_I = 0;
    double dValue_II = 0;
    double dValue_III = 0;

    while(true)
    {
        if(m_pTMSIChannel_TMSI_I->isEnabled())
        {
            dValue_I = m_pInBuffer_I->pop();
            m_pRTSA_TMSI_I_new->data()->setValue(dValue_I);
        }
        if(m_pTMSIChannel_TMSI_II->isEnabled())
        {
            dValue_II = m_pInBuffer_II->pop();
            m_pRTSA_TMSI_II_new->data()->setValue(dValue_II);
        }
        if(m_pTMSIChannel_TMSI_III->isEnabled())
        {
            dValue_III = m_pInBuffer_III->pop();
            m_pRTSA_TMSI_III_new->data()->setValue(dValue_III);
        }
    }
}
