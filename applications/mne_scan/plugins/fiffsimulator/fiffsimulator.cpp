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
#include <fiff/fiff_info.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include <scDisp/hpiwidget.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QMutexLocker>
#include <QList>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace IOBUFFER;
using namespace SCMEASLIB;
using namespace RTCLIENTLIB;
using namespace SCDISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_sFiffSimulatorClientAlias("mne_scan")
, m_pRtCmdClient(NULL)
, m_bCmdClientIsConnected(false)
, m_sFiffSimulatorIP("127.0.0.1")//("172.21.16.88")
, m_pFiffSimulatorProducer(new FiffSimulatorProducer(this))
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(0)
, m_bIsRunning(false)
, m_iActiveConnectorId(0)
{
    //Create HPI action in toolbar
    m_pActionComputeHPI = new QAction(QIcon(":/images/latestFiffInfoHPI.png"), tr("Compute HPI"),this);
    m_pActionComputeHPI->setStatusTip(tr("Compute HPI"));
    connect(m_pActionComputeHPI, &QAction::triggered,
            this, &FiffSimulator::showHPIDialog);
    addPluginAction(m_pActionComputeHPI);

    //Set some HPI connects
    connect(this, &FiffSimulator::started,
            this, &FiffSimulator::sendStatusToHPI);
    connect(this, &FiffSimulator::finished,
            this, &FiffSimulator::sendStatusToHPI);
}


//*************************************************************************************************************

FiffSimulator::~FiffSimulator()
{
    if(m_pFiffSimulatorProducer->isRunning() || this->isRunning())
        stop();
}


//*************************************************************************************************************

void FiffSimulator::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
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
    m_pRTMSA_FiffSimulator = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "FiffSimulator", "Fiff Simulator Output");
    m_pRTMSA_FiffSimulator->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_FiffSimulator);

    // Start FiffSimulatorProducer
    m_pFiffSimulatorProducer->start();

    //init channels when fiff info is available
    connect(this, &FiffSimulator::fiffInfoAvailable, this, &FiffSimulator::initConnector);

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient();
}


//*************************************************************************************************************

void FiffSimulator::unload()
{
    qDebug() << "void FiffSimulator::unload()";
}


//*************************************************************************************************************

bool FiffSimulator::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    if(m_bCmdClientIsConnected && m_pFiffInfo)
    {
        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        // Buffer
        m_qMutex.lock();
        m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8,m_pFiffInfo->nchan,m_iBufferSize));
        m_bIsRunning = true;
        m_qMutex.unlock();

        // Start threads
        QThread::start();

        m_pFiffSimulatorProducer->start();

        while(!m_pFiffSimulatorProducer->m_bFlagMeasuring)
            msleep(1);

        // Start Measurement at rt_Server
        // start measurement
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        return true;
    }
    else
        return false;
}


//*************************************************************************************************************

bool FiffSimulator::stop()
{
    if(m_pFiffSimulatorProducer->isRunning()) {
        m_pFiffSimulatorProducer->stop();
    }

    //Wait until this thread is stopped
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    if(this->isRunning())
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pRawMatrixBuffer_In->releaseFromPop();

        m_pRawMatrixBuffer_In->clear();

        m_pRTMSA_FiffSimulator->data()->clear();
    }

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
    while(true)
    {
        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
                break;
        }
        //pop matrix
        m_matValue = m_pRawMatrixBuffer_In->pop();

        //emit values
        m_pRTMSA_FiffSimulator->data()->setValue(m_matValue.cast<double>());
    }
}


//*************************************************************************************************************

void FiffSimulator::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSA_FiffSimulator->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_FiffSimulator->data()->setMultiArraySize(1);
        m_pRTMSA_FiffSimulator->data()->setVisibility(true);
        m_pRTMSA_FiffSimulator->data()->setXMLLayoutFile("./mne_scan_plugins/resources/FiffSimulator/VectorViewSimLayout.xml");
    }
}


//*************************************************************************************************************

void FiffSimulator::changeConnector(qint32 p_iNewConnectorId)
{
    if(p_iNewConnectorId != m_iActiveConnectorId)
    {
        //
        // read meas info
        //
        (*m_pRtCmdClient)["selcon"].pValues()[0].setValue(p_iNewConnectorId);
        (*m_pRtCmdClient)["selcon"].send();

        m_iActiveConnectorId = p_iNewConnectorId;

        //
        // clear all and request everything new
        //
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
        m_qMutex.lock();

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
        m_qMutex.unlock();
    }
}


//*************************************************************************************************************

void FiffSimulator::disconnectCmdClient()
{
    QMutexLocker locker(&m_qMutex);
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        if(m_pRtCmdClient->ConnectedState != QTcpSocket::UnconnectedState)
            m_pRtCmdClient->waitForDisconnected();
        m_bCmdClientIsConnected = false;
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void FiffSimulator::requestInfo()
{
    while(!(m_pFiffSimulatorProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)) {
        qWarning() << "FiffSimulatorProducer is not running! Retry...";
    }

    if(m_pFiffSimulatorProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected) {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pFiffSimulatorProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pFiffSimulatorProducer->producerMutex.lock();
        m_pFiffSimulatorProducer->m_bFlagInfoRequest = true;
        m_pFiffSimulatorProducer->producerMutex.unlock();
    } else {
        qWarning() << "FiffSimulatorProducer is not connected!";
    }
}


//*************************************************************************************************************

void FiffSimulator::showHPIDialog()
{
    if(!m_pFiffInfo) {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    } else {
        if (!m_pHPIWidget) {
            m_pHPIWidget = QSharedPointer<HPIWidget>(new HPIWidget(m_pFiffInfo));

            connect(m_pHPIWidget.data(), &HPIWidget::needData,
                    this, &FiffSimulator::sendHPIData);
        }

        if (!m_pHPIWidget->isVisible()) {
            m_pHPIWidget->show();
            m_pHPIWidget->raise();
        }
    }
}


//*************************************************************************************************************

void FiffSimulator::sendHPIData()
{
    if(m_pFiffInfo && m_pHPIWidget) {
        Eigen::MatrixXd matProj;
        m_pFiffInfo->make_projector(matProj);

        //set columns of matrix to zero depending on bad channels indexes
        for(qint32 j = 0; j < m_pFiffInfo->bads.size(); ++j) {
            matProj.col(m_pFiffInfo->ch_names.indexOf(m_pFiffInfo->bads.at(j))).setZero();
        }

        // Setup Comps
        FiffCtfComp newComp;
        m_pFiffInfo->make_compensator(0, 101, newComp);//Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
        Eigen::MatrixXd matComp = newComp.data->data;

        m_pHPIWidget->setData(matProj * matComp * m_matValue.cast<double>());

        m_pHPIWidget->setData(m_matValue.cast<double>());
    }
}


//*************************************************************************************************************

void FiffSimulator::sendStatusToHPI()
{
    if (!m_pHPIWidget) {
        m_pHPIWidget = QSharedPointer<HPIWidget>(new HPIWidget(m_pFiffInfo));

        connect(m_pHPIWidget.data(), &HPIWidget::needData,
                this, &FiffSimulator::sendHPIData);
    }

    m_pHPIWidget->setIsRunning(this->isRunning());
}

