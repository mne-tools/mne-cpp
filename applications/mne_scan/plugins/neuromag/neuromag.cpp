//=============================================================================================================
/**
* @file     neuromag.cpp
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
* @brief    Definition of the Neuromag class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag.h"
#include "neuromagproducer.h"

#include "FormFiles/neuromagsetupwidget.h"

#include <utils/ioutils.h>
#include <fiff/fiff_dir_node.h>


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

using namespace MneRtClientPlugin;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neuromag::Neuromag()
: m_sNeuromagClientAlias("mne_scan")
, m_pRtCmdClient(NULL)
, m_bCmdClientIsConnected(false)
, m_sNeuromagIP("172.21.16.88")//("127.0.0.1")
, m_pNeuromagProducer(new NeuromagProducer(this))
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(0)
, m_bIsRunning(false)
, m_sFiffHeader(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/neuromag/header.fif")
, m_iActiveConnectorId(0)
{

}


//*************************************************************************************************************

Neuromag::~Neuromag()
{
    if(m_pNeuromagProducer->isRunning() || this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Neuromag::clone() const
{
    QSharedPointer<Neuromag> pNeuromagClone(new Neuromag());
    return pNeuromagClone;
}


//*************************************************************************************************************

void Neuromag::init()
{
    // Start NeuromagProducer
    m_pNeuromagProducer->start();

    //init channels when fiff info is available
    connect(this, &Neuromag::fiffInfoAvailable, this, &Neuromag::initConnector);

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient();
}


//*************************************************************************************************************

void Neuromag::unload()
{

}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void Neuromag::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSA_Neuromag = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "Realtime", "MNE Rt Client");

        m_pRTMSA_Neuromag->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_Neuromag->data()->setMultiArraySize(1);

        m_pRTMSA_Neuromag->data()->setVisibility(true);

        m_pRTMSA_Neuromag->data()->setXMLLayoutFile("./mne_scan_plugins/resources/Neuromag/VectorViewLayout.xml");

        m_outputConnectors.append(m_pRTMSA_Neuromag);
    }

}


//*************************************************************************************************************

void Neuromag::changeConnector(qint32 p_iNewConnectorId)
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

void Neuromag::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

void Neuromag::connectCmdClient()
{
    if(m_pRtCmdClient.isNull())
        m_pRtCmdClient = QSharedPointer<RtCmdClient>(new RtCmdClient);
    else if(m_bCmdClientIsConnected)
        this->disconnectCmdClient();

    m_pRtCmdClient->connectToHost(m_sNeuromagIP);
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

            // This will read projectors from an external file and replace the one received from mne_rt_server
            //if(m_pFiffInfo)
            //    readProjectors();

            //
            // Read Connectors
            //
            if(m_qMapConnectors.size() == 0)
                m_iActiveConnectorId = m_pRtCmdClient->requestConnectors(m_qMapConnectors);

            QMap<qint32, QString>::const_iterator it;
            for(it = m_qMapConnectors.begin(); it != m_qMapConnectors.end(); ++it)
                if(it.value().compare("Neuromag Connector") == 0 && m_iActiveConnectorId != it.key())
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

void Neuromag::disconnectCmdClient()
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

void Neuromag::requestInfo()
{
    while(!(m_pNeuromagProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected))
        qWarning() << "NeuromagProducer is not running! Retry...";

    if(m_pNeuromagProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)
    {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pNeuromagProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pNeuromagProducer->producerMutex.lock();
        m_pNeuromagProducer->m_bFlagInfoRequest = true;
        m_pNeuromagProducer->producerMutex.unlock();
    }
    else
        qWarning() << "NeuromagProducer is not connected!";
}


//*************************************************************************************************************

bool Neuromag::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(m_bCmdClientIsConnected && m_pFiffInfo)
    {
        // Initialize real time measurements
        init();

        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        // Buffer
        m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8,m_pFiffInfo->nchan,m_iBufferSize));

        m_bIsRunning = true;

        // Start threads
        QThread::start();

        m_pNeuromagProducer->start();

        while(!m_pNeuromagProducer->m_bFlagMeasuring)
            msleep(1);

        // Start Measurement at rt_Server
        // start measurement
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pNeuromagProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        return true;
    }
    else {
        if(!m_pFiffInfo) {
            qWarning()<<"Neuromag::start - FiffInfo is empty (NULL).";
        }
        if(!m_bCmdClientIsConnected) {
            qWarning()<<"Neuromag::start - m_bCmdClientIsConnected is false.";
        }
        return false;
    }
}


//*************************************************************************************************************

bool Neuromag::stop()
{
    if(m_pNeuromagProducer->isRunning())
        m_pNeuromagProducer->stop();

    //Wait until this thread (TMSI) is stopped
    m_bIsRunning = false;

    if(this->isRunning())
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pRawMatrixBuffer_In->releaseFromPop();

        m_pRawMatrixBuffer_In->clear();

        m_pRTMSA_Neuromag->data()->clear();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Neuromag::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString Neuromag::getName() const
{
    return "Neuromag";
}


//*************************************************************************************************************

QWidget* Neuromag::setupWidget()
{
    NeuromagSetupWidget* widget = new NeuromagSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    return widget;
}


//*************************************************************************************************************

bool Neuromag::readProjectors()
{
    QFile t_headerFiffFile(m_sFiffHeader);

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&t_headerFiffFile));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open())
        return false;

    QList<FiffProj> q_ListProj = t_pStream->read_proj(t_pStream->dirtree());

    if (q_ListProj.size() == 0)
    {
        printf("Could not find projectors\n");
        return false;
    }

    m_pFiffInfo->projs = q_ListProj;

    //
    //   Activate the projection items
    //
    for (qint32 k = 0; k < m_pFiffInfo->projs.size(); ++k)
        m_pFiffInfo->projs[k].active = true;

    //garbage collecting
    t_pStream->close();

    return true;
}


//*************************************************************************************************************

void Neuromag::run()
{

    MatrixXf matValue;
    while(m_bIsRunning)
    {
        //pop matrix
        matValue = m_pRawMatrixBuffer_In->pop();

        //emit values
        m_pRTMSA_Neuromag->data()->setValue(matValue.cast<double>());
    }
}
