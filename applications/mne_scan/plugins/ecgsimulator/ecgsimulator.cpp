//=============================================================================================================
/**
 * @file     ecgsimulator.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     August, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the ECGSimulator class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecgsimulator.h"
#include "ecgproducer.h"

#include "FormFiles/ecgsetupwidget.h"

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

using namespace ECGSIMULATORPLUGIN;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ECGSimulator::ECGSimulator()
: m_pRTSA_ECG_I_new(0)
, m_fSamplingRate(250.0)
, m_iDownsamplingFactor(1)
, m_pInBuffer_I(new dBuffer(1024))
, m_pInBuffer_II(new dBuffer(1024))
, m_pInBuffer_III(new dBuffer(1024))
, m_pECGProducer(new ECGProducer(this, m_pInBuffer_I, m_pInBuffer_II, m_pInBuffer_III))
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/ECGSimulator/")
, m_pECGChannel_ECG_I(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_I_256_s30661.txt")))
, m_pECGChannel_ECG_II(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_II_256_s30661.txt")))
, m_pECGChannel_ECG_III(new ECGSimChannel(m_qStringResourcePath+"data/", QString("ECG_III_256_s30661.txt")))
, m_bIsRunning(false)
{
}


//*************************************************************************************************************

ECGSimulator::~ECGSimulator()
{
    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> ECGSimulator::clone() const
{
    QSharedPointer<ECGSimulator> pECGSimulatorClone(new ECGSimulator());
    return pECGSimulatorClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void ECGSimulator::init()
{
    if(m_pECGChannel_ECG_I->isEnabled())
    {
        m_pRTSA_ECG_I_new = PluginOutputData<RealTimeSampleArray>::create(this, "ECG I", "ECG I output data");
        m_outputConnectors.append(m_pRTSA_ECG_I_new);
    }

    if(m_pECGChannel_ECG_II->isEnabled())
    {
        m_pRTSA_ECG_II_new = PluginOutputData<RealTimeSampleArray>::create(this, "ECG II", "ECG II output data");
        m_outputConnectors.append(m_pRTSA_ECG_II_new);
    }

    if(m_pECGChannel_ECG_III->isEnabled())
    {
        m_pRTSA_ECG_III_new = PluginOutputData<RealTimeSampleArray>::create(this, "ECG III", "ECG III output data");
        m_outputConnectors.append(m_pRTSA_ECG_III_new);
    }
}


//*************************************************************************************************************

void ECGSimulator::unload()
{

}


//*************************************************************************************************************

void ECGSimulator::initChannels()
{
    m_pECGChannel_ECG_I->initChannel();
    m_pECGChannel_ECG_II->initChannel();
    m_pECGChannel_ECG_III->initChannel();

    if(m_pECGChannel_ECG_I->isEnabled())
    {
        double diff = m_pECGChannel_ECG_I->getMaximum() - m_pECGChannel_ECG_I->getMinimum();

        m_pRTSA_ECG_I_new->data()->setName("ECG I");
        m_pRTSA_ECG_I_new->data()->setUnit("mV");

        m_pRTSA_ECG_I_new->data()->setMinValue(m_pECGChannel_ECG_I->getMinimum()-diff/10);
        m_pRTSA_ECG_I_new->data()->setMaxValue(m_pECGChannel_ECG_I->getMaximum()+diff/10);
        m_pRTSA_ECG_I_new->data()->setArraySize(10);
        m_pRTSA_ECG_I_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_I_new->data()->setVisibility(m_pECGChannel_ECG_I->isVisible());
    }

    if(m_pECGChannel_ECG_II->isEnabled())
    {
        double diff = m_pECGChannel_ECG_II->getMaximum() - m_pECGChannel_ECG_II->getMinimum();

        m_pRTSA_ECG_II_new->data()->setName("ECG II");
        m_pRTSA_ECG_II_new->data()->setUnit("mV");

        m_pRTSA_ECG_II_new->data()->setMinValue(m_pECGChannel_ECG_II->getMinimum()-diff/10);
        m_pRTSA_ECG_II_new->data()->setMaxValue(m_pECGChannel_ECG_II->getMaximum()+diff/10);
        m_pRTSA_ECG_II_new->data()->setArraySize(10);
        m_pRTSA_ECG_II_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_II_new->data()->setVisibility(m_pECGChannel_ECG_II->isVisible());
    }

    if(m_pECGChannel_ECG_III->isEnabled())
    {
        double diff = m_pECGChannel_ECG_III->getMaximum() - m_pECGChannel_ECG_III->getMinimum();

        m_pRTSA_ECG_III_new->data()->setName("ECG III");
        m_pRTSA_ECG_III_new->data()->setUnit("mV");

        m_pRTSA_ECG_III_new->data()->setMinValue(m_pECGChannel_ECG_III->getMinimum()-diff/10);
        m_pRTSA_ECG_III_new->data()->setMaxValue(m_pECGChannel_ECG_III->getMaximum()+diff/10);
        m_pRTSA_ECG_III_new->data()->setArraySize(10);
        m_pRTSA_ECG_III_new->data()->setSamplingRate(m_fSamplingRate/m_iDownsamplingFactor);
        m_pRTSA_ECG_III_new->data()->setVisibility(m_pECGChannel_ECG_III->isVisible());
    }
}


//*************************************************************************************************************

bool ECGSimulator::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.

//    qDebug() << "-------------------------------------------- THREAD ID start " << QThread::currentThreadId();
//    if(this->isRunning()) {
//        m_bIsRunning = false;
//        QThread::wait();
//    }

    initChannels();

    // Start threads
    m_pECGProducer->start();

    if(m_pECGProducer->isRunning())
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "Plugin TMSI - ERROR - TMSIProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (TMSiSDK.dll / TMSiSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}


//*************************************************************************************************************

bool ECGSimulator::stop()
{
    // Stop threads
    m_pECGProducer->stop();

    //Wait until this thread (TMSI) is stopped
    m_bIsRunning = false;


    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pInBuffer_I->releaseFromPop();
    m_pInBuffer_I->clear();
    m_pInBuffer_II->releaseFromPop();
    m_pInBuffer_II->clear();
    m_pInBuffer_III->releaseFromPop();
    m_pInBuffer_III->clear();


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

IPlugin::PluginType ECGSimulator::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString ECGSimulator::getName() const
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

void ECGSimulator::run()
{
    double dValue_I = 0;
    double dValue_II = 0;
    double dValue_III = 0;

    while(m_bIsRunning)
    {
        //pop matrix only if the producer thread is running
        if(m_pECGProducer->isRunning())
        {
            if(m_pECGChannel_ECG_I->isEnabled())
            {
                dValue_I = m_pInBuffer_I->pop();
                m_pRTSA_ECG_I_new->data()->setValue(dValue_I);
            }
            if(m_pECGChannel_ECG_II->isEnabled())
            {
                dValue_II = m_pInBuffer_II->pop();
                m_pRTSA_ECG_II_new->data()->setValue(dValue_II);
            }
            if(m_pECGChannel_ECG_III->isEnabled())
            {
                dValue_III = m_pInBuffer_III->pop();
                m_pRTSA_ECG_III_new->data()->setValue(dValue_III);
            }
        }
    }
}
