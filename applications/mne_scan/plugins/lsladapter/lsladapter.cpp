//=============================================================================================================
/**
 * @file     lsladapter.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Simon Heinke. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapter.h"
#include "lsladapterproducer.h"
#include "FormFiles/lsladaptersetup.h"

#include <fiff/fiff.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QListWidgetItem>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLADAPTERPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace FIFFLIB;

//=============================================================================================================
// METATYPES
//=============================================================================================================

Q_DECLARE_METATYPE(lsl::stream_info);
Q_DECLARE_METATYPE(QVector<lsl::stream_info>);

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LSLAdapter::LSLAdapter()
: AbstractSensor()
, m_fSamplingFrequency(-1.0f)
, m_iNumberChannels(-1)
, m_iOutputBlockSize(100)
, m_pFiffInfo(QSharedPointer<FiffInfo>::create())
, m_pRTMSA(PluginOutputData<RealTimeMultiSampleArray>::create(this, "LSL Adapter", "LSL stream data"))
, m_updateStreamsFutureWatcher()
, m_vAvailableStreams()
, m_currentStream()
, m_bHasValidStream(false)
, m_producerThread()
, m_pProducer(new LSLAdapterProducer(m_pRTMSA, m_iOutputBlockSize))
{
    // would be better to do this on a single occasion
    qRegisterMetaType<lsl::stream_info>("lsl::stream_info");
    qRegisterMetaType<QVector<lsl::stream_info>>("QVector<lsl::stream_info>");
}

//=============================================================================================================

LSLAdapter::~LSLAdapter()
{
    m_pProducer->stop();
    m_producerThread.wait();
    m_pProducer->deleteLater();
    m_producerThread.deleteLater();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> LSLAdapter::clone() const
{
    return qSharedPointerCast<AbstractPlugin>(QSharedPointer<LSLAdapter>(new LSLAdapter()));
}

//=============================================================================================================

void LSLAdapter::init()
{
    // move producer to thread and connect a few synchronization points
    m_pProducer->moveToThread(&m_producerThread);
    connect(&m_producerThread, &QThread::started,
            m_pProducer, &LSLAdapterProducer::readStream);
    connect(m_pProducer, &LSLAdapterProducer::finished,
            &m_producerThread, &QThread::quit, Qt::DirectConnection);  // apparently a direct connection is needed in order to avoid a crash upon 'stop'

    // make RTMSA accessible
    m_pRTMSA->measurementData()->setName(this->getName());
    m_outputConnectors.append(m_pRTMSA);

    // connect finished signal for background lsl stream scanning
    connect(&m_updateStreamsFutureWatcher,
            &QFutureWatcher<QVector<lsl::stream_info>>::finished,
            this,
            &LSLAdapter::onLSLStreamScanReady);

    // try to start background scan for available LSL streams
    onRefreshAvailableStreams();

    // load filtering settings etc.
}

//=============================================================================================================

void LSLAdapter::unload()
{
    // stop producer
    m_pProducer->stop();
    m_producerThread.wait();
    m_pProducer->deleteLater();
    m_producerThread.deleteLater();

    // save filtering settings etc.
}

//=============================================================================================================

bool LSLAdapter::start()
{
//    // check if the thread is already or still running.
//    // this can happen if the start button is pressed immediately after the stop button was pressed.
//    // in this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning()) {
//        this->wait();
//    }

    if(m_bHasValidStream) {
        prepareFiffInfo(m_currentStream);

        // set the channel size of the RTMSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
        m_pRTMSA->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA->measurementData()->setMultiArraySize(1);
        m_pRTMSA->measurementData()->setVisibility(true);

        // start producer
        m_pProducer->setOutputBlockSize(m_iOutputBlockSize);
        m_pProducer->setStreamInfo(m_currentStream);
        m_producerThread.start();

        return true;
    } else {
        qDebug() << "[LSLAdapter::start] No valid stream !";
        return false;
    }
}

//=============================================================================================================

bool LSLAdapter::stop()
{
    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA->measurementData()->clear();

    // stop the producer and wait for it
    m_pProducer->stop();
    m_producerThread.wait();

    return true;
}

//=============================================================================================================

QWidget* LSLAdapter::setupWidget()
{
    // apparently the widget may get deleted during runtime,
    // so we have to return a new one everytime this method is called
    LSLAdapterSetup* temp = new LSLAdapterSetup(m_iOutputBlockSize);
    connect(temp, &LSLAdapterSetup::refreshAvailableStreams, this, &LSLAdapter::onRefreshAvailableStreams);
    connect(temp, &LSLAdapterSetup::streamSelectionChanged, this, &LSLAdapter::onStreamSelectionChanged);
    connect(temp, &LSLAdapterSetup::blockSizeChanged, this, &LSLAdapter::onBlockSizeChanged);
    connect(this, &LSLAdapter::updatedAvailableLSLStreams, temp, &LSLAdapterSetup::onLSLScanResults);

    // check if we have some information about previously available lsl streams:
    if(m_vAvailableStreams.isEmpty() == false && m_bHasValidStream) {
        // let the widget display potentially outdated info, until the background thread for stream scanning will return
        temp->onLSLScanResults(m_vAvailableStreams, m_currentStream);
    }

    // try to start background scan for available LSL streams
    onRefreshAvailableStreams();

    return temp;
}

//=============================================================================================================

void LSLAdapter::run()
{
    // producer has access to the RTMSA and publishes the blocks on its own, so there is nothing left to do here.
}

//=============================================================================================================

void LSLAdapter::onRefreshAvailableStreams()
{
    // lsl stream scanning is time-consuming, run in background:
    if (m_updateStreamsFutureWatcher.isFinished()) {
        QFuture<QVector<lsl::stream_info>> future = QtConcurrent::run(scanAvailableLSLStreams);
        m_updateStreamsFutureWatcher.setFuture(future);
    }
}

//=============================================================================================================

void LSLAdapter::onLSLStreamScanReady()
{
    // save result streams
    m_vAvailableStreams = m_updateStreamsFutureWatcher.result();

    // check whether any streams are available
    if(m_vAvailableStreams.size() == 0) {
        m_bHasValidStream = false;
        // overwrite current stream with default constructor, this will also result in correct UI display
        m_currentStream = lsl::stream_info();
    }
    else {
        // check whether we had a valid stream, and if its still amongst the available ones
        if(m_bHasValidStream) {
            if(contains(m_vAvailableStreams, m_currentStream) == false) {
                qDebug() << "[LSLAdapter] Old stream no longer available, switching to first available stream";
                m_currentStream = m_vAvailableStreams[0];
                m_bHasValidStream = true;
            }
            // else-case: no need to change anything
        } else {
            // simply take first stream
            m_currentStream = m_vAvailableStreams[0];
            m_bHasValidStream = true;
        }
    }

    // tell UI
    emit updatedAvailableLSLStreams(m_vAvailableStreams, m_currentStream);
}

//=============================================================================================================

QVector<lsl::stream_info> LSLAdapter::scanAvailableLSLStreams()
{
    // no filtering implemented so far, simply get all streams
    QVector<lsl::stream_info> vAvailableStreams = QVector<lsl::stream_info>::fromStdVector(lsl::resolve_streams());
    // do validity checks for all stream infos
    for(int i = 0; i < vAvailableStreams.size(); ++i) {
        if(isValid(vAvailableStreams[i]) == false) {
            vAvailableStreams.remove(i);
            i--;
            qDebug() << "[LSLAdapter::scanAvailableLSLStreams] Found and removed an invalid stream !";
        }
    }
    return vAvailableStreams;
}

//=============================================================================================================

void LSLAdapter::onStreamSelectionChanged(const lsl::stream_info& newStream)
{
    // no validity checks are done, since the UI only knows the streams we told it about
    m_bHasValidStream = true;
    m_currentStream = newStream;
}

//=============================================================================================================

void LSLAdapter::prepareFiffInfo(const lsl::stream_info &stream)
{
    // parse fiff info from lsl stream info
    QString type = QString::fromStdString(stream.type()).toUpper();
    if(type == "EEG") {
        // copy basic info into intended member fields
        m_iNumberChannels = stream.channel_count();
        m_fSamplingFrequency = static_cast<float>(stream.nominal_srate());

        // clear old fiff info data
        m_pFiffInfo->clear();
        // set number of channels, sampling frequency and high/-lowpass
        m_pFiffInfo->nchan = m_iNumberChannels;
        m_pFiffInfo->sfreq = m_fSamplingFrequency;

        // set up the channel info
        QStringList QSLChNames;
        m_pFiffInfo->chs.clear();

        for(int i = 0; i < m_pFiffInfo->nchan; ++i) {
            // create information for each channel
            QString sChType;
            FiffChInfo fChInfo;

            // set channel name
            sChType = QString("EEG ");
            if(i < 10) {
                sChType.append("00");
            }

            if(i >= 10 && i < 100) {
                sChType.append("0");
            }

            fChInfo.ch_name = sChType.append(sChType.number(i));

            // set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            // set logno
            fChInfo.logNo = i;

            // set coord frame
            fChInfo.coord_frame = FIFFV_COORD_HEAD;

            // set unit
            fChInfo.unit = FIFF_UNIT_V;
            fChInfo.unit_mul = 0;

            // set EEG electrode location - Convert from mm to m
            fChInfo.eeg_loc(0,0) = 0;
            fChInfo.eeg_loc(1,0) = 0;
            fChInfo.eeg_loc(2,0) = 0;

            // set EEG electrode direction - Convert from mm to m
            fChInfo.eeg_loc(0,1) = 0;
            fChInfo.eeg_loc(1,1) = 0;
            fChInfo.eeg_loc(2,1) = 0;

            // also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
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

            QSLChNames << sChType;

            m_pFiffInfo->chs.append(fChInfo);
        }

        // set channel names in fiff_info_base
        m_pFiffInfo->ch_names = QSLChNames;

        // set head projection
        m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
        m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
        m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
        m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
    }
    else {
        qDebug() << "[LSLAdapterProducer::setStreamInfo] Type " << type << " not implemented !";
        return;
    }
}

//=============================================================================================================

void LSLAdapter::onBlockSizeChanged(const int newBlockSize)
{
    m_iOutputBlockSize = newBlockSize;
}

//=============================================================================================================

QString LSLAdapter::getBuildDateTime()
{
    return QString(BUILDTIMESTAMP().date()) + " " + QString(BUILDTIMESTAMP().time());
}
