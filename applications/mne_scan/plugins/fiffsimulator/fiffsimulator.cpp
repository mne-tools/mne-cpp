//=============================================================================================================
/**
 * @file     fiffsimulator.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
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
 * @brief    Definition of the FiffSimulator class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"
#include "fiffsimulatorproducer.h"

#include "FormFiles/fiffsimulatorsetupwidget.h"

#include <utils/ioutils.h>
#include <fiff/fiff_info.h>
#include <fiff/c/fiff_digitizer_data.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QMutexLocker>
#include <QList>

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace UTILSLIB;
using namespace SCMEASLIB;
using namespace COMMUNICATIONLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_pFiffSimulatorProducer(new FiffSimulatorProducer(this))
, m_bCmdClientIsConnected(false)
, m_sFiffSimulatorIP("127.0.0.1")
, m_sFiffSimulatorClientAlias("mne_scan")
, m_iActiveConnectorId(0)
, m_iBufferSize(-1)
, m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_float>(new CircularBuffer_Matrix_float(40)))
, m_pRtCmdClient(QSharedPointer<RtCmdClient>::create())
, m_iDefaultPortCmdClient(4217)
{
    //init channels when fiff info is available
    connect(this, &FiffSimulator::fiffInfoAvailable,
            this, &FiffSimulator::initConnector);
}

//=============================================================================================================

FiffSimulator::~FiffSimulator()
{
    if(m_pFiffSimulatorProducer->isRunning() || this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

void FiffSimulator::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> FiffSimulator::clone() const
{
    QSharedPointer<FiffSimulator> pFiffSimulatorClone(new FiffSimulator());
    return std::move(pFiffSimulatorClone);
}

//=============================================================================================================

void FiffSimulator::init()
{
    m_pRTMSA_FiffSimulator = PluginOutputData<RealTimeMultiSampleArray>::create(this, "FiffSimulator", "Fiff Simulator Output");
    m_pRTMSA_FiffSimulator->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_FiffSimulator);

    //Try to connect the cmd client on start up using localhost connection
 //   this->connectCmdClient();
}

//=============================================================================================================

void FiffSimulator::unload()
{
    qDebug() << "FiffSimulator::unload()";
}

//=============================================================================================================

bool FiffSimulator::start()
{
    if(m_bCmdClientIsConnected && m_pFiffInfo) {
        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        m_pFiffSimulatorProducer->start();

        // Wait one sec so the producer can update the m_iDataClientId accordingly
        msleep(1000);

        // Start Measurement at mne_rt_server
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        QThread::start();

        return true;
    }

    return false;
}

//=============================================================================================================

bool FiffSimulator::stop()
{
    // Stop this (consumer) thread first
    requestInterruption();
    wait(500);

    // Stop producer thread second
    m_pFiffSimulatorProducer->stop();

    // Tell the mne_rt_server to stop sending data
    if(m_bCmdClientIsConnected) {
        (*m_pRtCmdClient)["stop-all"].send();
    }

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_FiffSimulator->measurementData()->clear();
    m_pCircularBuffer->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType FiffSimulator::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString FiffSimulator::getName() const
{
    return "Fiff Simulator";
}

//=============================================================================================================

QWidget* FiffSimulator::setupWidget()
{
    //widget is later distroyed by CentralWidget - so it has to be created everytime new
    QWidget * pWidget(new FiffSimulatorSetupWidget(this));
    return pWidget;
}

//=============================================================================================================

void FiffSimulator::run()
{
    MatrixXf matValue;

    while(!isInterruptionRequested()) {
        //pop matrix
        if(m_pCircularBuffer->pop(matValue)) {
            //emit values
            if(!isInterruptionRequested()) {
                m_pRTMSA_FiffSimulator->measurementData()->setValue(matValue.cast<double>());
            }
        }
    }
}

//=============================================================================================================

void FiffSimulator::initConnector()
{
    QMutexLocker locker (&m_qMutex);
    if(m_pFiffInfo) {
        m_pRTMSA_FiffSimulator->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_FiffSimulator->measurementData()->setDigitizerData(m_pFiffDigData);
        m_pRTMSA_FiffSimulator->measurementData()->setMultiArraySize(1);
        m_pRTMSA_FiffSimulator->measurementData()->setVisibility(true);
        m_pRTMSA_FiffSimulator->measurementData()->setXMLLayoutFile(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/FiffSimulator/VectorViewSimLayout.xml");
    }
}

//=============================================================================================================

void FiffSimulator::changeConnector(qint32 p_iNewConnectorId)
{
    if(p_iNewConnectorId != m_iActiveConnectorId) {
        // read meas info
        (*m_pRtCmdClient)["selcon"].pValues()[0].setValue(p_iNewConnectorId);
        (*m_pRtCmdClient)["selcon"].send();

        m_iActiveConnectorId = p_iNewConnectorId;

        // clear all and request everything new
        clear();

        // request available commands
        m_pRtCmdClient->requestCommands();

        // Read Info
        if(!m_pFiffInfo)
            requestInfo();

        // Read Buffer Size
        m_iBufferSize = m_pRtCmdClient->requestBufsize();

        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}

//=============================================================================================================

void FiffSimulator::connectCmdClient()
{
    if(m_bCmdClientIsConnected) {
        this->disconnectCmdClient();
    }

    if(!m_pFiffSimulatorProducer->isRunning()) {
        m_pFiffSimulatorProducer->start();
    }

    m_pRtCmdClient->connectToHost(m_sFiffSimulatorIP, m_iDefaultPortCmdClient);
    m_pRtCmdClient->waitForConnected(1000);

    if(m_pRtCmdClient->state() == QTcpSocket::ConnectedState)
    {
        m_qMutex.lock();

        if(!m_bCmdClientIsConnected)
        {
            // request available commands
            m_pRtCmdClient->requestCommands();

            // set cmd client is connected
            m_bCmdClientIsConnected = true;

            // Read Info
            if(!m_pFiffInfo)
                requestInfo();

            // Read Connectors
            if(m_qMapConnectors.size() == 0)
                m_iActiveConnectorId = m_pRtCmdClient->requestConnectors(m_qMapConnectors);

            QMap<qint32, QString>::const_iterator it;
            for(it = m_qMapConnectors.begin(); it != m_qMapConnectors.end(); ++it)
                if(it.value().compare("Fiff File Simulator") == 0 && m_iActiveConnectorId != it.key())
                    changeConnector(it.key());

            // Read Buffer Size
            m_iBufferSize = m_pRtCmdClient->requestBufsize();

            emit cmdConnectionChanged(m_bCmdClientIsConnected);
        }
        m_qMutex.unlock();
    }
}

//=============================================================================================================

void FiffSimulator::disconnectCmdClient()
{
    QMutexLocker locker(&m_qMutex);
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        if(m_pRtCmdClient->ConnectedState != QTcpSocket::UnconnectedState) {
            m_pRtCmdClient->waitForDisconnected();
        }
        m_bCmdClientIsConnected = false;
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}

//=============================================================================================================

void FiffSimulator::requestInfo()
{
    while(!(m_pFiffSimulatorProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)) {
        qWarning() << "FiffSimulatorProducer is not running! Retry...";
    }

    if(m_pFiffSimulatorProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected) {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pFiffSimulatorProducer->m_producerMutex.lock();
        m_pFiffSimulatorProducer->m_bFlagInfoRequest = true;
        m_pFiffSimulatorProducer->m_producerMutex.unlock();
    } else {
        qWarning() << "FiffSimulatorProducer is not connected!";
    }
}

//=============================================================================================================

QString FiffSimulator::getBuildDateTime()
{
    return QString(BUILDINFO::dateTime());
}
