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
: m_pRMTSA_TMSI(0)
, m_iSamplingFreq(2048)
, m_iNumberOfChannels(5)
, m_iSamplesPerBlock(5)
, m_iBufferSize(5000)
, m_pRawMatrixBuffer_In(0)
, m_pTMSIProducer(new TMSIProducer(this))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/tmsi/")
{
}


//*************************************************************************************************************

TMSI::~TMSI()
{
    std::cout << "TMSI::~TMSI()" << std::endl;

//    if(this->isRunning())
//        stop();

    //TODO: Destruktor is not getting called when mne_x is closed
    //-> This is a problem because the REfa device is not shut down from the sampling mode
    //-> uninit Fkt is not getting called
    //-> Beim abbrechen des programms bevor stop gedrückt wurde, muss das gleiche passieren als wäre stop gedrückt worden.
    //-> Plugin destructors are not called when exiting the glmainwindow
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
    m_pRMTSA_TMSI = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");

    m_pRMTSA_TMSI->data()->setVisibility(true);
    m_outputConnectors.append(m_pRMTSA_TMSI);
}


//*************************************************************************************************************

bool TMSI::start()
{
    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_TMSI->data()->init(m_iNumberOfChannels);

    // Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_iNumberOfChannels, m_iSamplesPerBlock));

    // Start threads
    m_pTMSIProducer->start(m_iNumberOfChannels, m_iSamplingFreq, m_iSamplesPerBlock);

    //if the producer could not be started (the driver is still sampling because it was not closed correctly) stop the producer (close the driver) and try to start the producer again
    if(m_pTMSIProducer->isRunning())
    {
        QThread::start();
        return true;
    }
    else
    {
        m_pTMSIProducer->stop();
        m_pTMSIProducer->start(m_iNumberOfChannels, m_iSamplingFreq, m_iSamplesPerBlock);
        if(m_pTMSIProducer->isRunning())
        {
           QThread::start();
           return true;
        }
    }

    std::cout << "Plugin TMSI - ERROR - TMSIProducer thread could not be started" << endl;
    return false;
}


//*************************************************************************************************************

bool TMSI::stop()
{
    // Stop threads
    m_pTMSIProducer->stop();

    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pRawMatrixBuffer_In->clear();

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
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initSamplingProperties();

    return widget;
}


//*************************************************************************************************************

void TMSI::run()
{
    MatrixXf matValue;

    while(true)
    {
        //pop matrix
        matValue = m_pRawMatrixBuffer_In->pop();
        std::cout << "matValue " << matValue.block(0,0,m_iNumberOfChannels,m_iSamplesPerBlock) << std::endl;

        //TODO: make matValue's size dynamic depending on the samples received by the tmsidriver class
        //      -> dynamically set the size of the m_pRMTSA_TMSI multi array

        //emit values to real time multi sample array
        for(qint32 i = 0; i < matValue.cols(); ++i)
            m_pRMTSA_TMSI->data()->setValue(matValue.col(i).cast<double>());
    }
}
