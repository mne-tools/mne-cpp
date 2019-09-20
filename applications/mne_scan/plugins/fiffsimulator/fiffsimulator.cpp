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
* @brief    Definition of the FiffSimulator class.
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
#include <scMeas/realtimemultisamplearray.h>
#include <disp3D/viewers/hpiview.h>


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
using namespace COMMUNICATIONLIB;
using namespace DISP3DLIB;


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
, m_bDoContinousHPI(false)
{
    //Init HPI
    m_pActionComputeHPI = new QAction(QIcon(":/images/latestFiffInfoHPI.png"), tr("Compute HPI"),this);
    m_pActionComputeHPI->setStatusTip(tr("Compute HPI"));
    connect(m_pActionComputeHPI, &QAction::triggered,
            this, &FiffSimulator::showHPIDialog);
    addPluginAction(m_pActionComputeHPI);
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
    m_pRTMSA_FiffSimulator = PluginOutputData<RealTimeMultiSampleArray>::create(this, "FiffSimulator", "Fiff Simulator Output");
    m_pRTMSA_FiffSimulator->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_FiffSimulator);

    // Start FiffSimulatorProducer
    m_pFiffSimulatorProducer->start();

    //init channels when fiff info is available
    connect(this, &FiffSimulator::fiffInfoAvailable,
            this, &FiffSimulator::initConnector);

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
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

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

    if(m_pHPIWidget) {
        m_pHPIWidget->hide();
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
    MatrixXf matValue;

    while(true) {
        {
            QMutexLocker locker(&m_qMutex);
            if(!m_bIsRunning)
                break;
        }
        //pop matrix
        matValue = m_pRawMatrixBuffer_In->pop();

        //Update HPI data (for single and continous HPI fitting)
        updateHPI(matValue);

        //Do continous HPI fitting and write result to data block
        if(m_bDoContinousHPI) {
            doContinousHPI(matValue);
        }

        //emit values
        m_pRTMSA_FiffSimulator->data()->setValue(matValue.cast<double>());
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
        m_pRTMSA_FiffSimulator->data()->setXMLLayoutFile(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/FiffSimulator/VectorViewSimLayout.xml");
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
            m_pHPIWidget = QSharedPointer<HpiView>(new HpiView(m_pFiffInfo));
            connect(m_pHPIWidget.data(), &HpiView::continousHPIToggled,
                    this, &FiffSimulator::onContinousHPIToggled);
        }

        if (!m_pHPIWidget->isVisible()) {
            m_pHPIWidget->show();
            m_pHPIWidget->raise();
        }
    }
}


//*************************************************************************************************************

void FiffSimulator::updateHPI(const MatrixXf& matData)
{
    //Update HPI data
    if(m_pFiffInfo && m_pHPIWidget) {
        m_pHPIWidget->setData(matData.cast<double>());
    }
}


//*************************************************************************************************************

void FiffSimulator::doContinousHPI(MatrixXf& matData)
{
    //This only works with babyMEG HPI channels 400 ... 407
    if(m_pFiffInfo && m_pHPIWidget && matData.rows() >= 407) {
        if(m_pHPIWidget->wasLastFitOk()) {
            // Load device to head transformation matrix from Fiff info
            QMatrix3x3 rot;

            for(int ir = 0; ir < 3; ir++) {
                for(int ic = 0; ic < 3; ic++) {
                    rot(ir,ic) = m_pFiffInfo->dev_head_t.trans(ir,ic);
                }
            }

            QQuaternion quatHPI = QQuaternion::fromRotationMatrix(rot);

            // Write rotation quaternion to HPI Ch #1~3
            matData.row(401) = MatrixXf::Constant(1,matData.cols(), quatHPI.x());
            matData.row(402) = MatrixXf::Constant(1,matData.cols(), quatHPI.y());
            matData.row(403) = MatrixXf::Constant(1,matData.cols(), quatHPI.z());

            // Write translation vector to HPI Ch #4~6
            matData.row(404) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(0,3));
            matData.row(405) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(1,3));
            matData.row(406) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(2,3));

            // Write GOF to HPI Ch #7
            // Write goodness of fit (GOF)to HPI Ch #7
            float dpfitError = 0.0;
            float GOF = 1 - dpfitError;
            matData.row(407) = MatrixXf::Constant(1,matData.cols(), GOF);
        }
    }
}


//*************************************************************************************************************

void FiffSimulator::onContinousHPIToggled(bool bDoContinousHPI)
{
    m_bDoContinousHPI = bDoContinousHPI;
}

