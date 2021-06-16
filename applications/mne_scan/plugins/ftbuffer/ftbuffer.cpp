//=============================================================================================================
/**
 * @file     ftbuffer.cpp
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
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
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBuffer::FtBuffer()
: m_bIsConfigured(false)
, m_pFtBuffProducer(QSharedPointer<FtBuffProducer>::create(this))
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_double>(new CircularBuffer_Matrix_double(10)))
, m_sBufferAddress("127.0.0.1")
, m_iBufferPort(1972)
{
}

//=============================================================================================================

FtBuffer::~FtBuffer()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> FtBuffer::clone() const
{
    QSharedPointer<FtBuffer> pFtBufferClone(new FtBuffer);
    return pFtBufferClone;
}

//=============================================================================================================

void FtBuffer::init()
{
    qInfo() << "[FtBuffer::init] Initializing FtBuffer plugin...";
    m_pRTMSA_BufferOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "FtBuffer", "FtBuffer Output");
    m_pRTMSA_BufferOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_BufferOutput);
}

//=============================================================================================================

void FtBuffer::unload()
{
}

//=============================================================================================================

bool FtBuffer::start()
{
    if (!m_bIsConfigured) {
        m_pFtBuffProducer->connectToBuffer(m_sBufferAddress,
                                           m_iBufferPort);
        if (!m_bIsConfigured) {
            return false;
        }
    }

    qInfo() << "[FtBuffer::start] Starting FtBuffer...";

    //Move relevant objects to new thread
    m_pFtBuffProducer->m_pFtConnector->m_pSocket->moveToThread(&m_pProducerThread);
    m_pFtBuffProducer->m_pFtConnector->moveToThread(&m_pProducerThread);
    m_pFtBuffProducer->moveToThread(&m_pProducerThread);

    //Connect signals to communicate with new thread
    connect(m_pFtBuffProducer.data(), &FtBuffProducer::newDataAvailable,
            this, &FtBuffer::onNewDataAvailable, Qt::DirectConnection);
    connect(this, &FtBuffer::workCommand,
            m_pFtBuffProducer.data(), &FtBuffProducer::doWork);

    m_pProducerThread.start();

    qInfo() << "[FtBuffer::start] Producer thread created, sending work command...";
    emit workCommand();

    QThread::start();

    return true;
}

//=============================================================================================================

bool FtBuffer::stop()
{
    qInfo() << "[FtBuffer::stop] Stopping.";

    m_bIsConfigured = false;

    //stops separate producer/client thread first
    m_pProducerThread.requestInterruption();
    while(m_pProducerThread.isRunning()) {
        msleep(10);
    }

    //Reset ftproducer and sample received list
    m_pFtBuffProducer.clear();
    m_pFtBuffProducer = QSharedPointer<FtBuffProducer>::create(this);

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_BufferOutput->measurementData()->clear();
    m_pCircularBuffer->clear();

    qInfo() << "[FtBuffer::stop] Stopped.";
    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType FtBuffer::getType() const
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
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this, QString("MNESCAN/%1").arg(this->getName()));
    return setupWidget;
}

//=============================================================================================================

void FtBuffer::run()
{
    MatrixXd matData;

    while(!isInterruptionRequested()) {
        //qDebug() << "[FtBuffer::run] m_pFiffInfo->dig.size()" << m_pFiffInfo->dig.size();
        //pop matrix
        if(m_pCircularBuffer->pop(matData)) {
            //emit values
            if(!isInterruptionRequested()) {
                m_pRTMSA_BufferOutput->measurementData()->setValue(matData);
            }
        }
    }
}

//=============================================================================================================

void FtBuffer::onNewDataAvailable(const Eigen::MatrixXd &matData)
{
    while(!m_pCircularBuffer->push(matData)) {
        //Do nothing until the circular buffer is ready to accept new data again
    }
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
        m_pRTMSA_BufferOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_BufferOutput->measurementData()->setMultiArraySize(1);
        m_pRTMSA_BufferOutput->measurementData()->setVisibility(true);

        qInfo() << "[FtBuffer::setupRTMSA] Successfully acquired fif info from file.";
        return m_bIsConfigured = true;
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

    std::cout << "attempting to init info\n";

    m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo (FiffInfo));

    std::cout << "init info\n";

    //Set the RMTSA parameters
    m_pRTMSA_BufferOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_BufferOutput->measurementData()->setMultiArraySize(1);
    m_pRTMSA_BufferOutput->measurementData()->setVisibility(true);

    qInfo() << "[FtBuffer::setupRTMSA] Successfully acquired fif info from buffer.";
    return m_bIsConfigured = true;
}

//=============================================================================================================

bool FtBuffer::setupRTMSA(MetaData metadata)
{
    if (metadata.info.sfreq < 0) {
        return false;
    } else {
        m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo (metadata.info));

        m_pRTMSA_BufferOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_BufferOutput->measurementData()->setMultiArraySize(1);
        m_pRTMSA_BufferOutput->measurementData()->setVisibility(true);
        if(metadata.bFiffDigitizerData){
            m_pRTMSA_BufferOutput->measurementData()->setDigitizerData(QSharedPointer<FIFFLIB::FiffDigitizerData>(new FIFFLIB::FiffDigitizerData (metadata.dig)));
        }
        m_bIsConfigured = true;
    }

    return m_bIsConfigured;
}

//=============================================================================================================

void FtBuffer::setBufferAddress(const QString &sAddress)
{
    m_sBufferAddress = sAddress;
}

//=============================================================================================================

void FtBuffer::setBufferPort(int iPort)
{
    m_iBufferPort = iPort;
}
