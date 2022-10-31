//=============================================================================================================
/**
 * @file     eegosportsdriver.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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
 * @brief    Contains the implementation of the EEGoSportsDriver class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsdriver.h"
#include "eegosportsproducer.h"

#include "eemagine/sdk/wrapper.cc" // Wrapper code to be compiled.
#include "eemagine/sdk/factory.h" // SDK header

#ifndef _WIN32
#include <unistd.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace eemagine::sdk;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsDriver::EEGoSportsDriver(EEGoSportsProducer* pEEGoSportsProducer)
: m_pEEGoSportsProducer(pEEGoSportsProducer)
, m_bDllLoaded(true)
, m_bInitDeviceSuccess(false)
, m_bStartRecordingSuccess(false)
, m_uiNumberOfChannels(90)
, m_uiNumberOfEEGChannels(64)
, m_uiSamplingFrequency(512)
, m_uiSamplesPerBlock(100)
, m_bWriteDriverDebugToFile(false)
, m_bMeasureImpedances(false)
{
    m_bDllLoaded = true;
}

//=============================================================================================================

EEGoSportsDriver::~EEGoSportsDriver()
{
}

//=============================================================================================================

bool EEGoSportsDriver::initDevice(bool bWriteDriverDebugToFile,
                                  bool bMeasureImpedance)
{
    m_bMeasureImpedances = bMeasureImpedance;

    //Check if the driver DLL was loaded
    if(!m_bDllLoaded) {
        return false;
    }

    //Set global variables
    m_bWriteDriverDebugToFile = bWriteDriverDebugToFile;

    //Open debug file to write to
    if(m_bWriteDriverDebugToFile) {
        m_outputFileStream.open("./EEGoSports_Driver_Debug.txt", std::ios::trunc); //ios::trunc deletes old file data
    }

    try {
        // Get device handler
#ifdef _WIN32
        factory factoryObj ("eego-SDK.dll"); // Make sure that eego-SDK.dll resides in the working directory
#else
        factory factoryObj ("libeego-SDK.so"); // Make sure that eego-SDK.dll resides in the working directory
#endif

        m_pAmplifier = factoryObj.getAmplifier(); // Get an amplifier

        //std::cout<<"[EEGoSportsDriver::initDevice] Serial number of connected eegosports device: "<<m_pAmplifier->getSerialNumber()<<std::endl;

        QThread::msleep(100);

    } catch (std::runtime_error& e) {
        qWarning() <<"[EEGoSportsDriver::initDevice] error " << e.what();
        return false;
    }

    std::vector<channel> channellist = m_pAmplifier->getChannelList();

    int iEEGChannelCount = 0;
    int iBipolarChannelCount = 0;
    for(std::vector<channel>::iterator it = channellist.begin(); it != channellist.end(); ++it){
        if(it->getType() == 1)
            ++iEEGChannelCount;
        else if(it->getType() == 2)
            ++iBipolarChannelCount;
    }
    if(!bMeasureImpedance)
        m_uiNumberOfChannels = channellist.size() + 3; //trigger, sample count, and ref not considered here
    else
        m_uiNumberOfChannels = iEEGChannelCount + 2; //ref and gnd not considered here

    m_uiNumberOfEEGChannels = iEEGChannelCount;
    m_uiNumberOfBipolarChannels = iBipolarChannelCount;

    qDebug() << "[EEGoSportsDriver::initDevice] iChannelcount " << m_uiNumberOfChannels;
    qDebug() << "[EEGoSportsDriver::initDevice] iEEGChannelcount " << iEEGChannelCount;
    qDebug() << "[EEGoSportsDriver::initDevice] iBipolarChannelCount " << iBipolarChannelCount;

    qInfo() << "[EEGoSportsDriver::initDevice] Successfully initialised the device.";

    // Set flag for successfull initialisation true
    m_bInitDeviceSuccess = true;

    return true;
}

//=============================================================================================================

bool EEGoSportsDriver::startRecording(int iSamplesPerBlock,
                                      int iSamplingFrequency,
                                      bool bMeasureImpedance)
{
    //Set global variables
    m_uiSamplesPerBlock = iSamplesPerBlock;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_bMeasureImpedances = bMeasureImpedance;

    try {
        //Start the stream
        if(bMeasureImpedance) {
            m_pDataStream = m_pAmplifier->OpenImpedanceStream();

        } else {
            //reference_range the range, in volt, for the referential channels. Valid values are: 1, 0.75, 0.15
            //bipolar_range the range, in volt, for the bipolar channels. Valid values are: 4, 1.5, 0.7, 0.35
            double reference_range = 0.75;
            double bipolar_range = 4;

            m_pDataStream = m_pAmplifier->OpenEegStream(m_uiSamplingFrequency, reference_range, bipolar_range);
        }

        QThread::msleep(100);

    } catch (std::runtime_error& e) {
        qWarning() <<"[EEGoSportsDriver::startRecording]" << e.what();
        return false;
    }

    qInfo() << "[EEGoSportsDriver::startRecording] Successfully started recording";

    // Set flag for successfull initialisation true
    m_bStartRecordingSuccess = true;

    return true;
}

//=============================================================================================================

bool EEGoSportsDriver::uninitDevice()
{
    //Check if the device was initialised
    if(!m_bStartRecordingSuccess) {
        qWarning() << "[EEGoSportsDriver::uninitDevice] Recording was not started - therefore can not be stopped";
        return false;
    }

    //Check if the device was initialised
    if(!m_bInitDeviceSuccess) {
        qWarning() << "[EEGoSportsDriver::uninitDevice] Device was not initialised - therefore can not be uninitialised";
        return false;
    }

    //Check if the driver DLL was loaded
    if(!m_bDllLoaded) {
        qWarning() << "[EEGoSportsDriver::uninitDevice] Driver DLL was not loaded";
        return false;
    }

    //Close the output stream/file
    if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile) {
        m_outputFileStream.close();
        m_outputFileStream.clear();
    }

    delete m_pDataStream;
    delete m_pAmplifier;

    qInfo() << "[EEGoSportsDriver::uninitDevice] Successfully uninitialised the device";

    m_bStartRecordingSuccess = false;
    m_bInitDeviceSuccess = false;

    return true;
}

//=============================================================================================================

bool EEGoSportsDriver::getSampleMatrixValue(Eigen::MatrixXd &sampleMatrix)
{
    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess) {
        qWarning() << "[EEGoSportsDriver::getSampleMatrixValue] Cannot start to get samples from device because device was not initialised correctly";
        return false;
    }

    if(!m_bStartRecordingSuccess) {
        qWarning() << "[EEGoSportsDriver::getSampleMatrixValue] Cannot start to get samples from device because recording was not started";
        return false;
    }

    sampleMatrix = MatrixXd::Zero(m_uiNumberOfChannels, m_uiSamplesPerBlock);

    uint iSampleIterator, iReceivedSamples, iChannelCount, i, j;

    iSampleIterator = 0;

    buffer buf;
    VectorXd vec;

    //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
    while(iSampleIterator < m_uiSamplesPerBlock) {
        //Get sample block from device
        buf = m_pDataStream->getData();

        iReceivedSamples = buf.getSampleCount();
        iChannelCount = buf.getChannelCount();

        //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. These are then written to the next matrix (block)
        for(i = 0; i < iReceivedSamples; ++i) {
            vec.resize(iChannelCount);

            for(j = 0; j < iChannelCount; ++j) {
                vec(j) = buf.getSample(j,i);
                //std::cout<<vec(j)<<std::endl;
            }

            m_lSampleBlockBuffer.push_back(vec);
        }

        //Fill matrix with data from buffer
        while(!m_lSampleBlockBuffer.isEmpty()) {
            if(iSampleIterator >= m_uiSamplesPerBlock) {
                break;
            }

            sampleMatrix.col(iSampleIterator).head(iChannelCount) = m_lSampleBlockBuffer.takeFirst();

            iSampleIterator++;
        }

        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile) {
            m_outputFileStream << "buf.getSampleCount(): " << buf.getSampleCount() << std::endl;
            m_outputFileStream << "buf.getChannelCount(): " << buf.getChannelCount() << std::endl;
            m_outputFileStream << "buf.size(): " << buf.size() << std::endl;
            m_outputFileStream << "iSampleIterator: " << iSampleIterator << std::endl;
            m_outputFileStream << "m_lSampleBlockBuffer.size(): " << m_lSampleBlockBuffer.size() << std::endl << std::endl;
            std::vector<channel> channellist = m_pDataStream->getChannelList();
            if(iSampleIterator == 1)
                for(std::vector<channel>::iterator it = channellist.begin(); it != channellist.end(); ++it){
                    m_outputFileStream << "Channeltype " << it->getType() << std::endl;
                    m_outputFileStream << "Channelindex " << it->getIndex() << std::endl;
                }
        }
    }

    return true;
}

//=============================================================================================================

uint EEGoSportsDriver::getNumberOfChannels()
{
    return m_uiNumberOfChannels;
}

//=============================================================================================================

uint EEGoSportsDriver::getNumberOfEEGChannels()
{
    return m_uiNumberOfEEGChannels;
}

//=============================================================================================================

uint EEGoSportsDriver::getNumberOfBipolarChannels()
{
    return m_uiNumberOfBipolarChannels;
}

//=============================================================================================================

QList<uint> EEGoSportsDriver::getChannellist()
{
    std::vector<channel> channellist = m_pAmplifier->getChannelList();

    QList<uint> uichannellist;
    for(std::vector<channel>::iterator it = channellist.begin(); it != channellist.end(); ++it)
        uichannellist.append(it->getType());

    return uichannellist;
}
