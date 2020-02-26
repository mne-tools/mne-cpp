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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer.h"
#include "ftbuffproducer.h"
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace FTBUFFERPLUGIN;
using namespace SCMEASLIB;
using namespace Eigen;
using namespace FIFFLIB;

//*************************************************************************************************************

FtBuffer::FtBuffer()
: m_bIsRunning(false)
, m_pFtBuffProducer(QSharedPointer<FtBuffProducer>::create(this))
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_pRTMSA_BufferOutput(PluginOutputData<RealTimeMultiSampleArray>::create(this, "FtBuffer", "Output data"))
, m_bBuffOutputSet(false)
, m_bCustomFiff(false)
{
    qInfo() << "[FtBuffer::FtBufer] Object created.";

    m_pActionShowYourWidget = new QAction(QIcon(":/images/options.png"), tr("FieldTrip Buffer Widget"),this);
    m_pActionShowYourWidget->setShortcut(tr("F12"));
    m_pActionShowYourWidget->setStatusTip(tr("FieldTrip Buffer Widget"));

    connect(m_pActionShowYourWidget, &QAction::triggered, this, &FtBuffer::showYourWidget);

    addPluginAction(m_pActionShowYourWidget);
}

//*************************************************************************************************************

FtBuffer::~FtBuffer()
{
    if(this->isRunning()) {
        stop();
    }
}

//*************************************************************************************************************

QSharedPointer<IPlugin> FtBuffer::clone() const
{
    QSharedPointer<FtBuffer> pFtBufferClone(new FtBuffer);
    return pFtBufferClone;
}

//*************************************************************************************************************

void FtBuffer::init()
{
    qInfo() << "[FtBuffer::init] Initializing FtBuffer plugin...";
    m_outputConnectors.append(m_pRTMSA_BufferOutput);
}

//*************************************************************************************************************

void FtBuffer::unload()
{
}

//*************************************************************************************************************

bool FtBuffer::start()
{

    qInfo() << "[FtBuffer::start] Starting FtBuffer...";
    m_bIsRunning = true;

    qRegisterMetaType<QPair<int,float>>("QPair<int,float>");

    // FtProducer in it's own thread and connect communications signals/slots
    m_pFtBuffProducer->m_pFtConnector->moveToThread(&m_pProducerThread);
    m_pFtBuffProducer->moveToThread(&m_pProducerThread);
    connect(m_pFtBuffProducer.data(), &FtBuffProducer::newDataAvailable, this, &FtBuffer::onNewDataAvailable, Qt::DirectConnection);
    connect(m_pFtBuffProducer.data(), &FtBuffProducer::extendedHeaderChunks, this, &FtBuffer::parseHeader, Qt::DirectConnection);
    connect(this, &FtBuffer::workCommand, m_pFtBuffProducer.data(),&FtBuffProducer::doWork);
    connect(m_pFtBuffProducer.data(), &FtBuffProducer::bufferParameters, this, &FtBuffer::setParams, Qt::DirectConnection);
    m_pProducerThread.start();

    qInfo() << "[FtBuffer::start] Producer thread created, sending work command...";
    emit workCommand();

    return true;
}

//*************************************************************************************************************

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

    return true;
}

//*************************************************************************************************************

IPlugin::PluginType FtBuffer::getType() const
{
    return _ISensor;
}

//*************************************************************************************************************

QString FtBuffer::getName() const
{
    return "FtBuffer";
}

//*************************************************************************************************************

QWidget* FtBuffer::setupWidget()
{
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this);
    return setupWidget;
}

//*************************************************************************************************************

void FtBuffer::run()
{
}

//*************************************************************************************************************

void FtBuffer::showYourWidget()
{
    m_pYourWidget = FtBufferYourWidget::SPtr(new FtBufferYourWidget());
    m_pYourWidget->show();
}

//*************************************************************************************************************

void FtBuffer::setUpFiffInfo()
{
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();


    if (m_bCustomFiff) {
        //
        //Set number of channels, sampling frequency and high/-lowpass
        //
        qInfo() << "Custom Fiff";
        m_pFiffInfo->nchan = m_iNumChannels;
        m_pFiffInfo->sfreq = m_iSampFreq;
        m_pFiffInfo->highpass = 0.001f;
        m_pFiffInfo->lowpass = m_iSampFreq/2;
    } else {
        //
        //Set number of channels, sampling frequency and high/-lowpass
        //
        qInfo() << "Default Fiff";
        m_pFiffInfo->nchan = m_iNumChannels;
        m_pFiffInfo->sfreq = m_iSampFreq;
        m_pFiffInfo->highpass = 0.001f;
        m_pFiffInfo->lowpass = m_iSampFreq/2;
    }

    //
    //Set up the channel info
    //
    QStringList QSLChNames;
    m_pFiffInfo->chs.clear();

    for(int i = 0; i < m_pFiffInfo->nchan; ++i)
    {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

//        //EEG Channels
//        if(i <= m_pFiffInfo->nchan-2)
//        {
            //Set channel name
            sChType = QString("CH. ");
            if(i<10) {
                sChType.append("00");
            }

            if(i>=10 && i<100) {
                sChType.append("0");
            }

            fChInfo.ch_name = sChType.append(sChType.number(i));

            //Set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            //Set logno
            fChInfo.logNo = i;

            //Set coord frame
            fChInfo.coord_frame = FIFFV_COORD_HEAD;

            //Set unit
            fChInfo.unit = FIFF_UNIT_V;

            //Set EEG electrode location - Convert from mm to m
            fChInfo.eeg_loc(0,0) = 0;
            fChInfo.eeg_loc(1,0) = 0;
            fChInfo.eeg_loc(2,0) = 0;

            //Set EEG electrode direction - Convert from mm to m
            fChInfo.eeg_loc(0,1) = 0;
            fChInfo.eeg_loc(1,1) = 0;
            fChInfo.eeg_loc(2,1) = 0;

            //Also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
            fChInfo.chpos.r0(0) = 0;
            fChInfo.chpos.r0(1) = 0;
            fChInfo.chpos.r0(2) = 0;

            fChInfo.chpos.ex(0) = 1;
            fChInfo.chpos.ex(1) = 0;
            fChInfo.chpos.ex(2) = 0;

            fChInfo.chpos.ey(0) = 0;
            fChInfo.chpos.ey(1) = 1;
            fChInfo.chpos.ey(2) = 0;

            fChInfo.chpos.ez(0) = 0;
            fChInfo.chpos.ez(1) = 0;
            fChInfo.chpos.ez(2) = 1;
//        }

//        //Digital input channel
//        if(i == m_pFiffInfo->nchan-1)
//        {
//            //Set channel type
//            fChInfo.kind = FIFFV_STIM_CH;

//            sChType = QString("STIM");
//            fChInfo.ch_name = sChType;
//        }

        QSLChNames << sChType;

        m_pFiffInfo->chs.append(fChInfo);
    }

    //Set channel names in fiff_info_base
    m_pFiffInfo->ch_names = QSLChNames;

    //
    //Set head projection
    //
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}

//*************************************************************************************************************

bool FtBuffer::isRunning()
{
    return m_bIsRunning;
}

//*************************************************************************************************************

void FtBuffer::onNewDataAvailable(const Eigen::MatrixXd &matData)
{
    qInfo() << "[FtBuffer::onNewDataAvailable] Appending matrix to plugin output...";
    m_mutex.lock();
    if(m_bIsRunning) {
//        if (!m_bBuffOutputSet) {
//            qDebug() << "Setting up buffer output";
//            setupRTMSA();
//            m_bBuffOutputSet = true;
//        }
        m_pRTMSA_BufferOutput->data()->setValue(matData);
        //std::cout << matData.col(0);

    }
    m_mutex.unlock();
    qInfo() << "[FtBuffer::onNewDataAvailable] Done.";

}

//*************************************************************************************************************

void FtBuffer::setParams(QPair<int,float> val)
{
    m_iNumChannels = val.first;
    m_iSampFreq = val.second;
}

//*************************************************************************************************************

bool FtBuffer::setupRTMSA()
{

    qInfo() << "[FtBuffer::setupRTMSA] Attempting to set up parameters from .fif file...";

    QBuffer qbuffInputSampleFif;
    qbuffInputSampleFif.open(QIODevice::ReadWrite);

    QFile infile("neuromag2ft.fif");

    if(!infile.open(QIODevice::ReadOnly)) {
        qWarning() << "[FtBuffer::setupRTMSA] Could not open file.  Plugin output won't be based on local fif parameters.";
        qInfo() << "[FtBuffer::setupRTMSA] Please verify neuromag2ft.fif is present in bin folder.";
    } else {
        qbuffInputSampleFif.write(infile.readAll());

        m_pNeuromagHeadChunkData = QSharedPointer<FIFFLIB::FiffRawData>(new FiffRawData(qbuffInputSampleFif));
        m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FiffInfo (m_pNeuromagHeadChunkData->info));

        m_bCustomFiff = true;

        //Set the RMTSA parameters
        m_pRTMSA_BufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_BufferOutput->data()->setMultiArraySize(1);
        m_pRTMSA_BufferOutput->data()->setVisibility(true);

        qInfo() << "[FtBuffer::setupRTMSA] Successfully acquired fif info from file.";
        return true;
    }
    return false;
}

//*************************************************************************************************************

bool FtBuffer::setupRTMSA(FIFFLIB::FiffInfo FiffInfo)
{
    //Check for FiffInfo that has not changed its default values and return early
    if (FiffInfo.sfreq < 0) {
        return false;
    }

    m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo (FiffInfo));

    m_bCustomFiff = true;

    //Set the RMTSA parameters
    m_pRTMSA_BufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_BufferOutput->data()->setMultiArraySize(1);
    m_pRTMSA_BufferOutput->data()->setVisibility(true);

    qInfo() << "[FtBuffer::setupRTMSA] Successfully aquired fif info from buffer.";
    return true;
}

//*************************************************************************************************************

void FtBuffer::parseHeader(QBuffer* chunkData)
{

    qInfo() << "Chunk data received";


//    char ch;
//    chunkData->open(QBuffer::ReadWrite);
    qInfo() << "Data buffer of size" << chunkData->size();
//    qDebug() << "test read:" << chunkData->getChar(&ch) << ch
//                             << chunkData->getChar(&ch) << ch
//                             << chunkData->getChar(&ch) << ch;


//    QFile infile("neuromag2ft.fif");

//    QBuffer mynewbuffer;

//    mynewbuffer.open(QIODevice::ReadWrite);

//    if(!infile.open(QIODevice::ReadOnly)) {
//        qDebug() << "Could not open file";
//    } else {
//        mynewbuffer.write(infile.readAll());
//    }

//    qDebug() << "My new buffer of size" << mynewbuffer.size();

////    mynewbuffer.seek(0);

//    m_pNeuromagHeadChunkData = QSharedPointer<FIFFLIB::FiffRawData>(new FiffRawData(mynewbuffer));

//    //qDebug() << "Buffer fed to FiffRawData, now buffer of size" << chunkData->size();

//    m_bCustomFiff = true;


//    QFile outfile("mytestoutput.txt");

//    if(!outfile.open(QIODevice::ReadWrite)) {
//        qDebug() << "Could not open file";
//    } else {
//        outfile.write(mynewbuffer.read(mynewbuffer.size()));
//    }

//    outfile.close();

//    chunkData->close();
}

//*************************************************************************************************************
