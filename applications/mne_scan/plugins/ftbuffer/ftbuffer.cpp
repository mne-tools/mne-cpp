/**
* @file     ftbuffer.cpp
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace FTBUFFERPLUGIN;
using namespace SCMEASLIB;
//using namespace IOBUFFER;

//*************************************************************************************************************

FtBuffer::FtBuffer()
: m_bIsRunning(false)
, m_pFtBuffProducer(new FtBuffProducer(this))
, m_pListReceivedSamples(QSharedPointer<QList<Eigen::MatrixXd> >::create())
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_pRTMSA_BufferOutput(PluginOutputData<RealTimeMultiSampleArray>::create(this, "FtBuffer", "Output data"))
{
    qDebug() << "Constructing FtBuffer Object";

    m_pFtBuffClient = new FtBuffClient();
    m_pActionShowYourWidget = new QAction(QIcon(":/images/options.png"), tr("FieldTrip Buffer Widget"),this);
    m_pActionShowYourWidget->setShortcut(tr("F12"));
    m_pActionShowYourWidget->setStatusTip(tr("FieldTrip Buffer Widget"));
    connect(m_pActionShowYourWidget, &QAction::triggered, this, &FtBuffer::showYourWidget);
    addPluginAction(m_pActionShowYourWidget);
}

//*************************************************************************************************************

FtBuffer::~FtBuffer() {
    if(this->isRunning()){
        stop();
    }
}


QSharedPointer<IPlugin> FtBuffer::clone() const {
    QSharedPointer<FtBuffer> pFtBufferClone(new FtBuffer);
    return pFtBufferClone;
}

//*************************************************************************************************************

void FtBuffer::init() {
    qDebug() << "Running init()";

    m_outputConnectors.append(m_pRTMSA_BufferOutput);

}

void FtBuffer::unload() {
    delete m_pFtBuffClient;
    delete m_pTempAddress;
}

bool FtBuffer::start() {

    qDebug() << "Running start()";

    m_bIsRunning = true;

    //Setup fiff info before setting up the RMTSA because we need it to init the RTMSA
    qDebug() << "Running setUpFiffInfo()";
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    qDebug() << "Running initFromFiffInfo";
    m_pRTMSA_BufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
    qDebug() << "Running setMultiArraySize";
    m_pRTMSA_BufferOutput->data()->setMultiArraySize(1);


    QThread::start();

    m_pFtBuffProducer->moveToThread(&m_pProducerThread);
    m_pProducerThread.start();

    //connect(m_pFtBuffProducer.data(), &FtBuffProducer::newDataAvailable, this, &FtBuffer::onNewDataAvailable, Qt::DirectConnection);

    //m_FtBuffClient
    //while (true) { qDebug() << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAA"; }

    return true;
}

//*************************************************************************************************************

bool FtBuffer::stop() {

    m_bIsRunning = false;

    m_pRTMSA_BufferOutput->data()->clear();

    m_pProducerThread.quit();
    m_pProducerThread.wait();

    return true;
}

//*************************************************************************************************************

IPlugin::PluginType FtBuffer::getType() const {
    return _ISensor;
}

//*************************************************************************************************************

QString FtBuffer::getName() const {
    return "FtBuffer";
}

//*************************************************************************************************************

QWidget* FtBuffer::setupWidget() {
    FtBufferSetupWidget* setupWidget = new FtBufferSetupWidget(this);
    return setupWidget;
}

//*************************************************************************************************************

void FtBuffer::run() {
    qDebug() << "run()";

    while(m_bIsRunning) {

        m_pFtBuffProducer->run();

        /*
        m_mutex.lock();
        if(!m_pListReceivedSamples->isEmpty()) {
            MatrixXd matData = m_pListReceivedSamples->takeFirst();
            m_pRTMSA_BufferOutput->data()->setValue(matData);
        };
        m_mutex.unlock();
        */
    }
}

//*************************************************************************************************************

void FtBuffer::showYourWidget() {
    m_pYourWidget = FtBufferYourWidget::SPtr(new FtBufferYourWidget());
    m_pYourWidget->show();
}

//*************************************************************************************************************

bool FtBuffer::connectToBuffer(QString addr){
    //this->m_FtBuffClient.setAddress(addr);
    //updateBufferAddress(addr);

    m_pTempAddress = new char[(addr.toLocal8Bit().size()) + 1];
    strcpy(m_pTempAddress, addr.toLocal8Bit().constData());

    delete m_pFtBuffClient;

    m_pFtBuffClient = new FtBuffClient(m_pTempAddress);
    return this->m_pFtBuffClient->startConnection();
}

//*************************************************************************************************************
bool FtBuffer::disconnectFromBuffer(){
    return this->m_pFtBuffClient->stopConnection();
}

//*************************************************************************************************************

Eigen::MatrixXd FtBuffer::getData() {
    qDebug() << "FtBuffer::getData()";


    int i = 0;
    while(m_bIsRunning) {

        qDebug() << "Loop" << i;
        m_pFtBuffClient->getData();
        i++;
    }

}

//*************************************************************************************************************

//void FtBuffer::updateBufferAddress(QString address) {}

//*************************************************************************************************************


void FtBuffer::setUpFiffInfo()
{
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    //CURRENTLY HARDWIRED TO FTBUFFER EXAMPLE DATA PARAMS FROM SINE2FT
    m_pFiffInfo->nchan = 15;
    m_pFiffInfo->sfreq = 256;
    m_pFiffInfo->highpass = 0.001f;
    m_pFiffInfo->lowpass = 256/2;

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
            sChType = QString("EEG ");
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

bool FtBuffer::isRunning() {
    return m_bIsRunning;
}

void FtBuffer::onNewDataAvailable(const Eigen::MatrixXd &matData) {
    m_mutex.lock();
    if(m_bIsRunning) {
        //qDebug()<<"Natus::onNewDataAvailable - appending data";
        m_pListReceivedSamples->append(matData);
    }
    m_mutex.unlock();

}
