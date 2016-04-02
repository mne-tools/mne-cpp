//=============================================================================================================
/**
* @file     eegosportsdriver.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsdriver.h"
#include "eegosportsproducer.h"

#include <eemagine/sdk/wrapper.cc> // Wrapper code to be compiled.


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGoSportsPlugin;
using namespace eemagine::sdk;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsDriver::EEGoSportsDriver(EEGoSportsProducer* pEEGoSportsProducer)
: m_pEEGoSportsProducer(pEEGoSportsProducer)
, m_bDllLoaded(true)
, m_bInitDeviceSuccess(false)
, m_uiNumberOfChannels(64)
, m_uiSamplingFrequency(1024)
, m_bWriteDriverDebugToFile(false)
, m_sOutputFilePath("/mne_x_plugins/resources/eegosports")
, m_bMeasureImpedances(false)
{
    m_bDllLoaded = true;
}


//*************************************************************************************************************

EEGoSportsDriver::~EEGoSportsDriver()
{
    //cout << "EEGoSportsDriver::~EEGoSportsDriver()" << endl;
}


//*************************************************************************************************************

bool EEGoSportsDriver::initDevice(int iNumberOfChannels,
                            int iSamplingFrequency,
                            bool bWriteDriverDebugToFile,
                            QString sOutpuFilePath,
                            bool bMeasureImpedance)
{
    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
        return false;

    //Set global variables
    m_uiNumberOfChannels = iNumberOfChannels;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_bWriteDriverDebugToFile = bWriteDriverDebugToFile;
    m_sOutputFilePath = sOutpuFilePath;
    m_bMeasureImpedances = bMeasureImpedance;

    //Open debug file to write to
    if(m_bWriteDriverDebugToFile)
        m_outputFileStream.open("mne_x_plugins/resources/eegosports/EEGoSports_Driver_Debug.txt", std::ios::trunc); //ios::trunc deletes old file data

    // Get device handler
    factory factoryObj ("eego-SDK.dll"); // Make sure that eego-SDK.dll resides in the working directory
    m_pAmplifier = factoryObj.getAmplifier(); // Get an amplifier
    std::cout<<"EEGoSportsDriver::initDevice - Serial number of connected eegosports device: "<<m_pAmplifier->getSerialNumber()<<std::endl;

    //Start the stream
    if(bMeasureImpedance) {
        m_pDataStream = m_pAmplifier->OpenImpedanceStream(m_uiSamplingFrequency);
    } else {
        m_pDataStream = m_pAmplifier->OpenEegStream(m_uiSamplingFrequency);

    }

    Sleep(100);

    std::cout << "EEGoSportsDriver::initDevice - Successfully initialised the device." << std::endl;

    // Set flag for successfull initialisation true
    m_bInitDeviceSuccess = true;

    return true;
}


//*************************************************************************************************************

bool EEGoSportsDriver::uninitDevice()
{
    //Check if the device was initialised
    if(!m_bInitDeviceSuccess)
    {
        std::cout << "Plugin EEGoSports - ERROR - uninitDevice() - Device was not initialised - therefore can not be uninitialised" << std::endl;
        return false;
    }

    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
    {
        std::cout << "Plugin EEGoSports - ERROR - uninitDevice() - Driver DLL was not loaded" << std::endl;
        return false;
    }

    //Close the output stream/file
    if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
    {
        m_outputFileStream.close();
        m_outputFileStream.clear();
    }

    delete m_pDataStream;
    delete m_pAmplifier;

    std::cout << "Plugin EEGoSports - INFO - uninitDevice() - Successfully uninitialised the device" << std::endl;
    return true;
}


//*************************************************************************************************************

bool EEGoSportsDriver::getSampleMatrixValue(MatrixXf& sampleMatrix)
{
    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        std::cout << "Plugin EEGoSports - ERROR - getSampleMatrixValue() - Cannot start to get samples from device because device was not initialised correctly" << std::endl;
        return false;
    }

    buffer buf = m_pDataStream->getData();
    std::cout << "EEGoSportsDriver::getSampleMatrixValue - Samples read: " << buf.getSampleCount() << std::endl;
    std::cout << "EEGoSportsDriver::getSampleMatrixValue - Channel count: " << buf.getChannelCount() << std::endl;
    std::cout << "EEGoSportsDriver::getSampleMatrixValue - size: " << buf.size() << std::endl;

    int iReceivedSamples = buf.getSampleCount();
    int iChannelCount = buf.getChannelCount();

    sampleMatrix = MatrixXf(iChannelCount, iReceivedSamples);

    if(iReceivedSamples > 0) {
        //Write data to matrix


        for(int sample = 0; sample < iReceivedSamples; sample++)
            for(int channel = 0; channel < iChannelCount; channel++)
                sampleMatrix(channel, sample) = buf.getSample(channel, sample);
    }

    Sleep(100);

    return true;

//    // Init stuff
//    int channelMax = 0;
//    IBuffer* pBuffer; // The data storage

//    //Fetch data from device/driver like this:
//    m_pAmplifier->GetData(&pBuffer);

//    //Get sample and channel infos from device
//    m_uiNumberOfAvailableChannels = pBuffer->GetChannelCount();
//    int ulNumSamplesReceived = pBuffer->GetSampleCount();

//    //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
//    if(ulNumSamplesReceived>0)
//    {
//        //If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
//        if(m_uiNumberOfAvailableChannels < m_uiNumberOfChannels)
//            channelMax = m_uiNumberOfAvailableChannels;
//        else
//            channelMax = m_uiNumberOfChannels;

//        sampleMatrix = MatrixXf (channelMax, ulNumSamplesReceived);

//        // Write data to matrix
//        for(int sample = 0; sample < ulNumSamplesReceived; sample++)
//            for(int channel = 0; channel < channelMax; channel++)
//                sampleMatrix(channel, sample) = m_bUseChExponent ? pBuffer->GetBuffer(channel, sample) * 1e-6 : pBuffer->GetBuffer(channel, sample);

//        // Memory cleanup
//        pBuffer->Release();
//        pBuffer = NULL;

//        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
//            m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;

//        return true;
//    }

//    if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
//        m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;

//    // Memory cleanup
//    pBuffer->Release();
//    pBuffer = NULL;

    return false;
}
