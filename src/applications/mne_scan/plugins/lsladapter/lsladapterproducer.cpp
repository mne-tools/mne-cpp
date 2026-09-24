//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     lsladapterproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 * @brief    Contains the definition of the LSLAdapterProducer class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladapterproducer.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QThread>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLADAPTERPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LSLAdapterProducer::LSLAdapterProducer(QSharedPointer<PluginOutputData<RealTimeMultiSampleArray> > pRTMSA,
                                       int iOutputBlockSize,
                                       QObject *parent)
: QObject(parent)
, m_StreamInfo()
, m_StreamInlet(Q_NULLPTR)
, m_bHasStreamInfo(false)
, m_bIsRunning(false)
, m_iOutputBlockSize(iOutputBlockSize)
, m_vBufferedSamples()
, m_pRTMSA(pRTMSA)
{
}

//=============================================================================================================

LSLAdapterProducer::~LSLAdapterProducer()
{
    delete m_StreamInlet;
}

//=============================================================================================================

void LSLAdapterProducer::readStream()
{
    // check if we have a stream info
    if (m_bHasStreamInfo == false) {
        qDebug() << "[LSLAdapterProducer::readStream] No stream info was supplied !";
        return;
    }

    // start to stream: build a stream inlet
    try {
        m_StreamInlet = new LSLLIB::stream_inlet(m_StreamInfo);
        m_StreamInlet->open_stream();
    }
    catch (std::exception& e) {
        qDebug() << "[LSLAdapterProducer::readStream] Something went wrong when trying to open LSL stream inlet: " << e.what();
    }

    m_bIsRunning = true;
    while(m_bIsRunning) {
        try {
            if(m_StreamInlet->samples_available() == false) {
                // save CPU time, then check again
                QThread::msleep(5);
                continue;
            }
            std::vector<std::vector<float>> chunk = m_StreamInlet->pull_chunk<float>();

            // append chunk to buffer
            m_vBufferedSamples.insert(std::end(m_vBufferedSamples), std::begin(chunk), std::end(chunk));

            // check if we can output another block
            if(static_cast<int>(m_vBufferedSamples.size()) >= m_iOutputBlockSize) {
                Eigen::MatrixXd matOutput(m_StreamInfo.channel_count(), m_iOutputBlockSize);

                // copy samples
                for(int iSampleIdx = 0; iSampleIdx < m_iOutputBlockSize; ++iSampleIdx) {
                    for(int iChannelIdx = 0; iChannelIdx < m_StreamInfo.channel_count(); ++iChannelIdx) {
                        matOutput(iChannelIdx, iSampleIdx) = static_cast<double>(m_vBufferedSamples[iSampleIdx][iChannelIdx]);
                    }
                }

                // remove copied samples
                for(int i = 0; i < m_iOutputBlockSize; ++i) {
                    m_vBufferedSamples.erase(m_vBufferedSamples.begin());
                }

                // publish new block
                m_pRTMSA->measurementData()->setValue(matOutput);
            }
        }
        catch (std::exception& e) {
            qDebug() << "[LSLAdapterProducer::readStream] Something went wrong while streaming data: " << e.what();
            m_bIsRunning = false;
        }
    }

    // cleanup: close stream
    try {
        m_StreamInlet->close_stream();
    }
    catch (std::exception& e) {
        qDebug() << "[LSLAdapterProducer::readStream] Something went wrong when trying to close LSL stream: " << e.what();
    }

    emit finished();
}

//=============================================================================================================

void LSLAdapterProducer::setStreamInfo(const LSLLIB::stream_info& stream)
{
    m_StreamInfo = stream;   
    m_bHasStreamInfo = true;
}

//=============================================================================================================

void LSLAdapterProducer::stop()
{
    m_bIsRunning = false;
}

//=============================================================================================================

void LSLAdapterProducer::reset()
{
    // reset flags
    m_bIsRunning = false;
    m_bHasStreamInfo = false;
    // clear buffer
    m_vBufferedSamples.clear();
    // reset lsl members
    m_StreamInfo = LSLLIB::stream_info();
    delete m_StreamInlet;
    m_StreamInlet = Q_NULLPTR;
}

//=============================================================================================================

void LSLAdapterProducer::setOutputBlockSize(const int iNewBlockSize)
{
    m_iOutputBlockSize = iNewBlockSize;
}
