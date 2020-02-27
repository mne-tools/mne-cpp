//=============================================================================================================
/**
 * @file     ftbuffer.cpp
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @version  dev
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel B Motta. All rights reserved.
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
 * @brief    Definition of the FtBuffer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include "ftbuffer.h"
#include "ftbuffproducer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBuffer::FtBuffer()
: m_bIsRunning(false)
, m_pFtBuffProducer(QSharedPointer<FtBuffProducer>::create(this))
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_pRTMSA_BufferOutput(PluginOutputData<RealTimeMultiSampleArray>::create(this, "FtBuffer", "Output data"))
{
    qInfo() << "[FtBuffer::FtBufer] Object created.";

    //Sets up GUI and GUI connections
    m_pActionShowYourWidget = new QAction(QIcon(":/images/options.png"), tr("FieldTrip Buffer Widget"),this);
    m_pActionShowYourWidget->setShortcut(tr("F12"));
    m_pActionShowYourWidget->setStatusTip(tr("FieldTrip Buffer Widget"));

    connect(m_pActionShowYourWidget, &QAction::triggered, this, &FtBuffer::showYourWidget);

    addPluginAction(m_pActionShowYourWidget);
}

//=============================================================================================================

FtBuffer::~FtBuffer()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> FtBuffer::clone() const
{
    QSharedPointer<FtBuffer> pFtBufferClone(new FtBuffer);
    return pFtBufferClone;
}

//=============================================================================================================

void FtBuffer::init()
{
    qInfo() << "[FtBuffer::init] Initializing FtBuffer plugin...";
    m_outputConnectors.append(m_pRTMSA_BufferOutput);
}

//=============================================================================================================

void FtBuffer::unload()
{
}

//=============================================================================================================

bool FtBuffer::start()
{
    qInfo() << "[FtBuffer::start] Starting FtBuffer...";
    m_bIsRunning = true;

    //Move relevant objects to new thread
    m_pFtBuffProducer->m_pFtConnector->m_pSocket->moveToThread(&m_pProducerThread);
    m_pFtBuffProducer->m_pFtConnector->moveToThread(&m_pProducerThread);
    m_pFtBuffProducer->moveToThread(&m_pProducerThread);

    //Connect signals to communicate with new thread
    connect(m_pFtBuffProducer.data(), &FtBuffProducer::newDataAvailable, this, &FtBuffer::onNewDataAvailable, Qt::DirectConnection);
    connect(this, &FtBuffer::workCommand, m_pFtBuffProducer.data(),&FtBuffProducer::doWork);

    m_pProducerThread.start();

    qInfo() << "[FtBuffer::start] Producer thread created, sending work command...";
    emit workCommand();

    return true;
}

//=============================================================================================================

bool FtBuffer::stop()
{
    qInfo() << "[FtBuffer::stop] Stopping...";

    m_mutex.lock();
    m_bIsRunning = false;
    m_mutex.unlock();

    m_pRTMSA_BufferOutput->data()->clear();

    //stops separate producer/client thread
    m_pProducerThread.quit();
    m_pProducerThread.wait();

    //Reset ftproducer and sample received list
    m_pFtBuffProducer.clear();
    m_pFtBuffProducer = QSharedPointer<FtBuffProducer>::create(this);

    qInfo() << "[FtBuffer::stop] Stoped.";
    return true;
}

//=============================================================================================================

IPlugin::PluginType FtBuffer::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString FtBuffer::getName() const
{
    return "FtBuffer";
}

//=============================================================================================================

QWidget* FtBuffer::setupWidget()
{
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void FtBuffer::run()
{
}

//=============================================================================================================

void FtBuffer::showYourWidget()
{
    m_pYourWidget = FtBufferYourWidget::SPtr(new FtBufferYourWidget());
    m_pYourWidget->show();
}

//=============================================================================================================

bool FtBuffer::isRunning()
{
    return m_bIsRunning;
}

//=============================================================================================================

void FtBuffer::onNewDataAvailable(const Eigen::MatrixXd &matData)
{
    qInfo() << "[FtBuffer::onNewDataAvailable] Appending matrix to plugin output...";

    m_mutex.lock();

    if(m_bIsRunning) {
        m_pRTMSA_BufferOutput->data()->setValue(matData);
    }

    m_mutex.unlock();

    qInfo() << "[FtBuffer::onNewDataAvailable] Done.";
}

//=============================================================================================================

bool FtBuffer::setupRTMSA()
{
    qInfo() << "[FtBuffer::setupRTMSA] Attempting to set up parameters from .fif file...";

    QBuffer qbuffInputSampleFif;
    qbuffInputSampleFif.open(QIODevice::ReadWrite);

    QFile infile("neuromag2ft.fif");

    if(!infile.open(QIODevice::ReadOnly)) {
        qInfo() << "[FtBuffer::setupRTMSA] Could not open file.  Plugin output won't be based on local fif parameters.";
    } else {
        qbuffInputSampleFif.write(infile.readAll());

        m_pNeuromagHeadChunkData = QSharedPointer<FIFFLIB::FiffRawData>(new FiffRawData(qbuffInputSampleFif));
        m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo (m_pNeuromagHeadChunkData->info));

        //Set the RMTSA parameters
        m_pRTMSA_BufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_BufferOutput->data()->setMultiArraySize(1);
        m_pRTMSA_BufferOutput->data()->setVisibility(true);

        qInfo() << "[FtBuffer::setupRTMSA] Successfully acquired fif info from file.";
        return true;
    }
    return false;
}

//=============================================================================================================

bool FtBuffer::setupRTMSA(FIFFLIB::FiffInfo FiffInfo)
{
    //Check for FiffInfo that has not changed its default values and return early
    if (FiffInfo.sfreq < 0) {
        return false;
    }

    m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo (FiffInfo));

    //Set the RMTSA parameters
    m_pRTMSA_BufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_BufferOutput->data()->setMultiArraySize(1);
    m_pRTMSA_BufferOutput->data()->setVisibility(true);

    qInfo() << "[FtBuffer::setupRTMSA] Successfully aquired fif info from buffer.";
    return true;
}
