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

using namespace EEGOSPORTSPLUGIN;
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
, m_uiSamplingFrequency(512)
, m_uiSamplesPerBlock(100)
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
                            int iSamplesPerBlock,
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
    m_uiSamplesPerBlock = iSamplesPerBlock;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_bWriteDriverDebugToFile = bWriteDriverDebugToFile;
    m_sOutputFilePath = sOutpuFilePath;
    m_bMeasureImpedances = bMeasureImpedance;

    //Open debug file to write to
    if(m_bWriteDriverDebugToFile) {
        m_outputFileStream.open("./mne_scan_plugins/resources/eegosports/EEGoSports_Driver_Debug.txt", std::ios::trunc); //ios::trunc deletes old file data
    }

    try {
        // Get device handler
        factory factoryObj ("eego-SDK.dll"); // Make sure that eego-SDK.dll resides in the working directory
        m_pAmplifier = factoryObj.getAmplifier(); // Get an amplifier

        //std::cout<<"EEGoSportsDriver::initDevice - Serial number of connected eegosports device: "<<m_pAmplifier->getSerialNumber()<<std::endl;

        //Start the stream
        if(bMeasureImpedance) {
            m_pDataStream = m_pAmplifier->OpenImpedanceStream(m_uiSamplingFrequency);
        } else {            
            //reference_range the range, in volt, for the referential channels. Valid values are: 1, 0.75, 0.15
            //bipolar_range the range, in volt, for the bipolar channels. Valid values are: 4, 1.5, 0.7, 0.35
            double reference_range = 0.75;
            double bipolar_range = 4;

            m_pDataStream = m_pAmplifier->OpenEegStream(m_uiSamplingFrequency, reference_range, bipolar_range);
        }

        Sleep(100);
    } catch (std::runtime_error& e) {
        std::cout <<"EEGoSportsDriver::initDevice - error " << e.what() << std::endl;
        return false;
    }

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

bool EEGoSportsDriver::getSampleMatrixValue(Eigen::MatrixXd &sampleMatrix)
{
    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        std::cout << "Plugin EEGoSports - ERROR - getSampleMatrixValue() - Cannot start to get samples from device because device was not initialised correctly" << std::endl;
        return false;
    }

    sampleMatrix = MatrixXd(90, m_uiSamplesPerBlock);
    sampleMatrix.setZero(); // Clear matrix - set all elements to zero

    uint iSamplesWrittenToMatrix = 0;
    int sampleMax = 0;
    int sampleIterator = 0;

    //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
    while(iSamplesWrittenToMatrix < m_uiSamplesPerBlock)
    {
        //Get sample block from device
        buffer buf = m_pDataStream->getData();

        int iReceivedSamples = buf.getSampleCount();
        int iChannelCount = buf.getChannelCount();

        //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
        if(iReceivedSamples > 0)
        {
            int actualSamplesWritten = 0; //Holds the number of samples which are actually written to the matrix in this while procedure

            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. These are then written to the next matrix (block)
            for(int i=0; i<iReceivedSamples; i++) {
                VectorXd vec(iChannelCount);

                for(uint j=0; j<iChannelCount; j++) {
                    vec(j) = buf.getSample(j,i);
                    //std::cout<<vec(j)<<std::endl;
                }

                m_vecSampleBlockBuffer.push_back(vec);
            }

            //If the number of the samples which were already written to the matrix plus the last received number of samples is larger then the defined block size
            //-> only fill until the matrix is completeley filled with samples. The other (unused) samples are still stored in the vector buffer m_vSampleBlockBuffer and will be used in the next matrix which is to be sent to the circular buffer
            if(iSamplesWrittenToMatrix + iReceivedSamples > m_uiSamplesPerBlock)
                sampleMax = m_uiSamplesPerBlock - iSamplesWrittenToMatrix + sampleIterator;
            else
                sampleMax = iReceivedSamples + sampleIterator;

            //Read the needed number of samples from the vector buffer to store them in the matrix
            for(; sampleIterator < sampleMax; sampleIterator++)
            {
                sampleMatrix.col(sampleIterator) = m_vecSampleBlockBuffer.first();
                m_vecSampleBlockBuffer.pop_front();

                actualSamplesWritten ++;
            }

            iSamplesWrittenToMatrix = iSamplesWrittenToMatrix + actualSamplesWritten;
        }

//        std::cout << "buf.getSampleCount(): " << buf.getSampleCount() << std::endl;
//        std::cout << "buf.getChannelCount(): " << buf.getChannelCount() << std::endl;
//        std::cout << "buf.size(): " << buf.size() << std::endl;
//        std::cout << "iReceivedSamples: " << iReceivedSamples << std::endl;
//        std::cout << "sampleMax: " << sampleMax << std::endl;
//        std::cout << "sampleIterator: " << sampleIterator << std::endl;
//        std::cout << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << std::endl;
//        std::cout << "m_uiSamplesPerBlock: " << m_uiSamplesPerBlock << std::endl;
//        std::cout << "m_uiSamplesPerBlock: " << m_uiSamplesPerBlock << std::endl << std::endl;

        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
        {
            m_outputFileStream << "buf.getSampleCount(): " << buf.getSampleCount() << std::endl;
            m_outputFileStream << "buf.getChannelCount(): " << buf.getChannelCount() << std::endl;
            m_outputFileStream << "buf.size(): " << buf.size() << std::endl;
            m_outputFileStream << "iReceivedSamples: " << iReceivedSamples << std::endl;
            m_outputFileStream << "sampleMax: " << sampleMax << std::endl;
            m_outputFileStream << "sampleIterator: " << sampleIterator << std::endl;
            m_outputFileStream << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << std::endl << std::endl;
            m_outputFileStream << "m_vecSampleBlockBuffer.size(): " << m_vecSampleBlockBuffer.size() << std::endl;
        }
    }

    Sleep(100);

    return true;
}
