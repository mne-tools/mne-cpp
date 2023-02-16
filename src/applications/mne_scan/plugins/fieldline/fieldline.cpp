//=============================================================================================================
/**
 * @file     fieldline.cpp
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
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
 * @brief    Contains the definition of the Fieldline class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fieldline.h"
// #include "fieldlineproducer.h"
// #include "FormFiles/fieldlinesetup.h"

// #include <fiff/fiff.h>
// #include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

// #include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIELDLINEPLUGIN;
// using namespace SCSHAREDLIB;
// using namespace SCMEASLIB;
// using namespace FIFFLIB;
// using namespace Eigen;
// using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Fieldline::Fieldline()
: m_iSamplingFreq(2048)
, m_iNumberChannels(46)
, m_iSamplesPerBlock(256)
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/fieldline/")
, m_pRMTSA_Fieldline(PluginOutputData<RealTimeMultiSampleArray>::create(this, "Fieldline", "EEG output data"))
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
{
    m_pRMTSA_Fieldline->measurementData()->setName(this->getName());//Provide name to auto store widget settings
}

//=============================================================================================================

Fieldline::~Fieldline() {
    // If the program is closed while the sampling is in process
    if(this->isRunning()) {
        this->stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Fieldline::clone() const
{
    QSharedPointer<AbstractPlugin> pFieldline(new Fieldline());
    return pFieldlineClone;
}

//=============================================================================================================

void Fieldline::init()
{
    m_outputConnectors.append(m_pRMTSA_Natus);
}

//=============================================================================================================

void Fieldline::unload()
{
}

//=============================================================================================================

void Fieldline::setUpFiffInfo()
{
    //Clear old fiff info data
    m_pFiffInfo->clear();

    //Set number of channels, sampling frequency and high/-lowpass
    m_pFiffInfo->nchan = m_iNumberChannels;
    m_pFiffInfo->sfreq = m_iSamplingFreq;
    m_pFiffInfo->highpass = 0.001f;
    m_pFiffInfo->lowpass = m_iSamplingFreq/2;

    //Set up the channel info
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

    //Set head projection
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}

//=============================================================================================================

bool Fieldline::start()
{
    // Init circular buffer to transmit data from the producer to this thread
    if(!m_pCircularBuffer) {
        m_pCircularBuffer = QSharedPointer<CircularBuffer_Matrix_double>(new CircularBuffer_Matrix_double(10));
    }

    //Setup fiff info before setting up the RMTSA because we need it to init the RTMSA
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_Natus->measurementData()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_Natus->measurementData()->setMultiArraySize(1);

    QThread::start();

    // Start the producer
    m_pNatusProducer = QSharedPointer<NatusProducer>::create(m_iSamplesPerBlock, m_iNumberChannels);
    m_pNatusProducer->moveToThread(&m_pProducerThread);
    connect(m_pNatusProducer.data(), &NatusProducer::newDataAvailable,
            this, &Fieldline::onNewDataAvailable, Qt::DirectConnection);
    m_pProducerThread.start();

    return true;
}

//=============================================================================================================

bool Fieldline::stop()
{
    requestInterruption();
    wait(500);

    // Clear all data in the buffer connected to displays and other plugins
    m_pRMTSA_Natus->measurementData()->clear();
    m_pCircularBuffer->clear();

    m_pProducerThread.quit();
    m_pProducerThread.wait();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Fieldline::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString Fieldline::getName() const
{
    return "Natus EEG";
}

//=============================================================================================================

QWidget* Fieldline::setupWidget()
{
    NatusSetup* widget = new NatusSetup(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}

//=============================================================================================================

void Fieldline::onNewDataAvailable(const Eigen::MatrixXd &matData)
{
    while(!m_pCircularBuffer->push(matData)) {
        //Do nothing until the circular buffer is ready to accept new data again
    }
}

//=============================================================================================================

void Fieldline::run()
{
    MatrixXd matData;

    while(!isInterruptionRequested()) {
        if(m_pCircularBuffer->pop(matData)) {
            //emit values
            if(!isInterruptionRequested()) {
                m_pRMTSA_Natus->measurementData()->setValue(matData);
            }
        }
    }
}

//=============================================================================================================

QString Fieldline::getBuildInfo()
{
    return QString(NATUSPLUGIN::buildDateTime()) + QString(" - ")  + QString(NATUSPLUGIN::buildHash());
}

