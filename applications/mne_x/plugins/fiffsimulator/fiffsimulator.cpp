//=============================================================================================================
/**
* @file     fiffsimulator.cpp
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
* @brief    Contains the implementation of the FiffSimulator class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"
#include "fiffsimulatorproducer.h"

#include "FormFiles/fiffsimulatorsetupwidget.h"

#include <utils/ioutils.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QList>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FiffSimulatorPlugin;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_sFiffSimulatorClientAlias("mne-x")
, m_pRtCmdClient(NULL)
, m_bCmdClientIsConnected(false)
, m_sFiffSimulatorIP("127.0.0.1")//("172.21.16.88")//("127.0.0.1")
, m_pFiffSimulatorProducer(new FiffSimulatorProducer(this))
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(0)
{

}


//*************************************************************************************************************

FiffSimulator::~FiffSimulator()
{
    if(this->isRunning())
        stop();
    if(m_pFiffSimulatorProducer->isRunning())
        m_pFiffSimulatorProducer->stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> FiffSimulator::clone() const
{
    QSharedPointer<FiffSimulator> pFiffSimulatorClone(new FiffSimulator());
    return pFiffSimulatorClone;
}


//*************************************************************************************************************

void FiffSimulator::init()
{
    // Start FiffSimulatorProducer
    m_pFiffSimulatorProducer->start();

    //init channels when fiff info is available
    connect(this, &FiffSimulator::fiffInfoAvailable, this, &FiffSimulator::initConnector);

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient();
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void FiffSimulator::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSA_FiffSimulator = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "FiffSimulator", "Fiff Simulator Output");

        m_pRTMSA_FiffSimulator->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_FiffSimulator->data()->setMultiArraySize(10);

        m_pRTMSA_FiffSimulator->data()->setVisibility(true);

        m_outputConnectors.append(m_pRTMSA_FiffSimulator);
    }

}


//*************************************************************************************************************

void FiffSimulator::changeConnector(qint32 p_iNewConnectorId)
{
    if(p_iNewConnectorId != m_iActiveConnectorId)
    {
        // read meas info
        (*m_pRtCmdClient)["selcon"].pValues()[0].setValue(p_iNewConnectorId);
        (*m_pRtCmdClient)["selcon"].send();

        m_iActiveConnectorId = p_iNewConnectorId;

        // clear all and request everything new
        clear();

        //
        // request available commands
        //
        m_pRtCmdClient->requestCommands();

        //
        // Read Info
        //
        if(!m_pFiffInfo)
            requestInfo();

        //
        // Read Buffer Size
        //
        m_iBufferSize = m_pRtCmdClient->requestBufsize();

        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void FiffSimulator::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

void FiffSimulator::connectCmdClient()
{
    if(m_pRtCmdClient.isNull())
        m_pRtCmdClient = QSharedPointer<RtCmdClient>(new RtCmdClient);
    else if(m_bCmdClientIsConnected)
        this->disconnectCmdClient();

    m_pRtCmdClient->connectToHost(m_sFiffSimulatorIP);
    m_pRtCmdClient->waitForConnected(1000);

    if(m_pRtCmdClient->state() == QTcpSocket::ConnectedState)
    {
        rtServerMutex.lock();

        if(!m_bCmdClientIsConnected)
        {
            //
            // request available commands
            //
            m_pRtCmdClient->requestCommands();

            //
            // set cmd client is connected
            //
            m_bCmdClientIsConnected = true;

            //
            // Read Info
            //
            if(!m_pFiffInfo)
                requestInfo();

            //
            // Read Connectors
            //
            if(m_qMapConnectors.size() == 0)
                m_iActiveConnectorId = m_pRtCmdClient->requestConnectors(m_qMapConnectors);


            QMap<qint32, QString>::const_iterator it;
            for(it = m_qMapConnectors.begin(); it != m_qMapConnectors.end(); ++it)
                if(it.value().compare("Fiff File Simulator") == 0 && m_iActiveConnectorId != it.key())
                    changeConnector(it.key());

            //
            // Read Buffer Size
            //
            m_iBufferSize = m_pRtCmdClient->requestBufsize();

            emit cmdConnectionChanged(m_bCmdClientIsConnected);
        }
        rtServerMutex.unlock();
    }
}


//*************************************************************************************************************

void FiffSimulator::disconnectCmdClient()
{
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        m_pRtCmdClient->waitForDisconnected();
        rtServerMutex.lock();
        m_bCmdClientIsConnected = false;
        rtServerMutex.unlock();
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void FiffSimulator::requestInfo()
{
    if(m_pFiffSimulatorProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)
    {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pFiffSimulatorProducer->producerMutex.lock();
        m_pFiffSimulatorProducer->m_bFlagInfoRequest = true;
        m_pFiffSimulatorProducer->producerMutex.unlock();
    }
    else
        qWarning() << "FiffSimulatorProducer is not connected!";
}


//*************************************************************************************************************

bool FiffSimulator::start()
{
    if(m_bCmdClientIsConnected && m_pFiffInfo)
    {
        // Initialize real time measurements
        init();

        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        // Buffer
        m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8,m_pFiffInfo->nchan,m_iBufferSize));

        // Start threads
        QThread::start();

        // Start Measurement at rt_Server
        // start measurement
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        m_pFiffSimulatorProducer->producerMutex.lock();
        m_pFiffSimulatorProducer->m_bFlagMeasuring = true;
        m_pFiffSimulatorProducer->producerMutex.unlock();
        m_pFiffSimulatorProducer->start();

        return true;
    }
    else
        return false;
}


//*************************************************************************************************************

bool FiffSimulator::stop()
{
    if(m_bCmdClientIsConnected) //ToDo replace this with is running
    {
        // Stop Measurement at rt_Server
        (*m_pRtCmdClient)["stop-all"].send();

        m_pFiffSimulatorProducer->producerMutex.lock();
        m_pFiffSimulatorProducer->m_bFlagMeasuring = false;
        m_pFiffSimulatorProducer->producerMutex.unlock();
    }

    // Stop threads
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pRawMatrixBuffer_In->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType FiffSimulator::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString FiffSimulator::getName() const
{
    return "Fiff Simulator";
}


//*************************************************************************************************************

QWidget* FiffSimulator::setupWidget()
{
    FiffSimulatorSetupWidget* widget = new FiffSimulatorSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    return widget;
}


//*************************************************************************************************************

void FiffSimulator::run()
{

    MatrixXf matValue;
    while(true)
    {
        //pop matrix
        matValue = m_pRawMatrixBuffer_In->pop();

        //emit values
        for(qint32 i = 0; i < matValue.cols(); ++i)
            m_pRTMSA_FiffSimulator->data()->setValue(matValue.col(i).cast<double>());
    }
}
