//=============================================================================================================
/**
* @file     lsladapter.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2019
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the definition of the LSLAdapter class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter.h"
#include "lsladapterproducer.h"
#include "FormFiles/lsladaptersetup.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>
#include <QtConcurrent>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLADAPTERPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LSLAdapter::LSLAdapter()
    : ISensor()
    , m_updateStreamsFutureWatcher()
    , m_vAvailableStreams()
    , m_pProducerThread()
    , m_pProducer(new LSLAdapterProducer())
{
    // move producer to thread and connect a few feedback functions
    m_pProducer->moveToThread(&m_pProducerThread);
    connect(&m_pProducerThread,
            &QThread::started,
            m_pProducer,
            &LSLAdapterProducer::readStream);
    connect(m_pProducer,
            &LSLAdapterProducer::finished,
            &m_pProducerThread,
            &QThread::quit);
    connect(&m_pProducerThread,
            &QThread::finished,
            this,
            &LSLAdapter::onProducerThreadFinished);

    // connect finished signal for background lsl stream scanning
    connect(&m_updateStreamsFutureWatcher,
            &QFutureWatcher<QVector<lsl::stream_info>>::finished,
            this,
            &LSLAdapter::onLSLStreamScanReady);
}


//*************************************************************************************************************

LSLAdapter::~LSLAdapter()
{
    m_pProducer->stop();
    m_pProducerThread.wait();
    delete m_pProducer;
}


//*************************************************************************************************************

QSharedPointer<IPlugin> LSLAdapter::clone() const
{
    return qSharedPointerCast<IPlugin>(QSharedPointer<LSLAdapter>(new LSLAdapter()));
}


//*************************************************************************************************************

void LSLAdapter::init()
{
    // load filtering settings etc.
}


//*************************************************************************************************************

void LSLAdapter::unload()
{
    // save filtering settings etc.
}


//*************************************************************************************************************

bool LSLAdapter::start()
{
    // starting is UI-controlled
    return true;
}


//*************************************************************************************************************

bool LSLAdapter::stop()
{
    // stop the producer and wait for it
    m_pProducer->stop();
    m_pProducerThread.wait();
    return true;
}


//*************************************************************************************************************

QWidget* LSLAdapter::setupWidget()
{
    // apparently the widget may get deleted during runtime,
    // so we have to return a new one everytime this method is called
    LSLAdapterSetup* temp = new LSLAdapterSetup();
    connect(temp, &LSLAdapterSetup::startStream, this, &LSLAdapter::onStartStream);
    connect(temp, &LSLAdapterSetup::stopStream, this, &LSLAdapter::onStopStream);
    connect(temp, &LSLAdapterSetup::refreshAvailableStreams, this, &LSLAdapter::onRefreshAvailableStreams);
    connect(this, &LSLAdapter::updatedAvailableLSLStreams, temp, &LSLAdapterSetup::onLSLScanResults);

    // check if we have some information about previously available lsl streams:
    if(m_vAvailableStreams.isEmpty() == false) {
        // let the widget display potentially outdated info, until the background thread for stream scanning will return
        temp->onLSLScanResults(m_vAvailableStreams);
    }

    // lsl stream scanning is time consuming, run in background:
    QFuture<QVector<lsl::stream_info>> future = QtConcurrent::run(scanAvailableLSLStreams);
    m_updateStreamsFutureWatcher.setFuture(future);

    return temp;
}


//*************************************************************************************************************

void LSLAdapter::run()
{
    // nothing so far
}


//*************************************************************************************************************

void LSLAdapter::onRefreshAvailableStreams()
{
    // lsl stream scanning is time consuming, run in background:
    if (m_updateStreamsFutureWatcher.isRunning() == false) {
        QFuture<QVector<lsl::stream_info>> future = QtConcurrent::run(scanAvailableLSLStreams);
        m_updateStreamsFutureWatcher.setFuture(future);
    }
}


//*************************************************************************************************************

void LSLAdapter::onStartStream(const lsl::stream_info& stream)
{
    // start producer, catch a few basic faulty conditions first
    if (m_pProducerThread.isRunning())
    {
        qDebug() << "[LSLAdapter::onStartStream] Producer thread still / already running";
        return;
    }
    // pass stream info to producer
    m_pProducer->reset();
    m_pProducer->setStreamInfo(stream);
    // start background thread
    m_pProducerThread.start();
}


//*************************************************************************************************************

void LSLAdapter::onStopStream()
{
    if (m_pProducerThread.isRunning() == false) {
        qDebug() << "LSLAdapter::onStopStream] Stream should already be stopped...";
    }
    m_pProducer->stop();
}


//*************************************************************************************************************

void LSLAdapter::onProducerThreadFinished()
{
    m_pProducer->reset();
    m_pProducerThread.wait();
}


//*************************************************************************************************************

void LSLAdapter::onLSLStreamScanReady()
{
    // save available streams
    m_vAvailableStreams = m_updateStreamsFutureWatcher.result();

    // tell UI
    emit updatedAvailableLSLStreams(m_vAvailableStreams);
}


//*************************************************************************************************************

QVector<lsl::stream_info> LSLAdapter::scanAvailableLSLStreams()
{
    // no filtering implemented so far, simply get all streams
    QVector<lsl::stream_info> temp = QVector<lsl::stream_info>::fromStdVector(lsl::resolve_streams());
    return temp;
}
