//=============================================================================================================
/**
* @file     tmsidriver.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the TMSIDriver class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsidriver.h"
#include "tmsiproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIDriver::TMSIDriver(TMSIProducer* pTMSIProducer)
: m_pTMSIProducer(pTMSIProducer)
, m_bDllLoaded(true)
, m_bInitDeviceSuccess(false)
, m_uiNumberOfChannels(138)
, m_uiSamplingFrequency(1024)
, m_uiSamplesPerBlock(1)
, m_bConvertToVolt(false)
, m_bUseChExponent(false)
, m_bUseUnitGain(false)
, m_bUseUnitOffset(false)
, m_bWriteToFile(false)
, m_sOutputFilePath("/mne_x_plugins/resources/tmsi")
{
    //Initialise NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;
    m_PSPDPMasterDevicePath = NULL;
    m_lSignalBuffer = NULL;

    //Open library
    m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\RTINST.DLL");

    //If it can't be open return
    if( m_oLibHandle == NULL)
    {
        cout << "Plugin TMSI - ERROR - Couldn't load DLL in 'C:\\Windows\\System32\\RTINST.DLL' - Is the driver for the TMSi USB Fiber Connector installed?" << endl;
        m_bDllLoaded = false;
        return;
    }

    //Load DLL methods for initializing the driver
    __load_dll_func__(m_oFpOpen, POPEN, "Open");
    __load_dll_func__(m_oFpClose, PCLOSE, "Close");
    __load_dll_func__(m_oFpGetDeviceState, PGETDEVICESTATE, "GetDeviceState");
    __load_dll_func__(m_oFpStart, PSTART, "Start");
    __load_dll_func__(m_oFpReset, PRESETDEVICE, "ResetDevice");
    __load_dll_func__(m_oFpStop, PSTOP, "Stop");
    __load_dll_func__(m_oFpGetSlaveHandle, PGETSLAVEHANDLE, "GetSlaveHandle");
    __load_dll_func__(m_oFpAddSlave, PADDSLAVE, "AddSlave");
    __load_dll_func__(m_oFpGetSignalFormat, PGETSIGNALFORMAT, "GetSignalFormat");
    __load_dll_func__(m_oFpSetSignalBuffer, PSETSIGNALBUFFER, "SetSignalBuffer");
    __load_dll_func__(m_oFpGetSamples, PGETSAMPLES, "GetSamples");
    __load_dll_func__(m_oFpGetBufferInfo, PGETBUFFERINFO, "GetBufferInfo");
    __load_dll_func__(m_oFpDeviceFeature, PDEVICEFEATURE, "DeviceFeature");
    __load_dll_func__(m_oFpGetInstanceId, PGETINSTANCEID, "GetInstanceId" );
    __load_dll_func__(m_oFpOpenRegKey, POPENREGKEY, "OpenRegKey" );
    __load_dll_func__(m_oFpFree, PFREE, "Free" );

    cout << "Plugin TMSI - INFO - TMSIDriver() - Successfully loaded all DLL functions" << endl;
}


//*************************************************************************************************************

TMSIDriver::~TMSIDriver()
{
    //cout << "TMSIDriver::~TMSIDriver()" << endl;
}


//*************************************************************************************************************

bool TMSIDriver::initDevice(int iNumberOfChannels,
                            int iSamplingFrequency,
                            int iSamplesPerBlock,
                            bool bConvertToVolt,
                            bool bUseChExponent,
                            bool bUseUnitGain,
                            bool bUseUnitOffset,
                            bool bWriteToFile,
                            QString sOutpuFilePath)
{
    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
        return false;

    //Set global variables
    m_uiNumberOfChannels = iNumberOfChannels;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_uiSamplesPerBlock = iSamplesPerBlock;
    m_bConvertToVolt = bConvertToVolt;
    m_bUseChExponent = bUseChExponent;
    m_bUseUnitGain = bUseUnitGain;
    m_bUseUnitOffset = bUseUnitOffset;
    m_bWriteToFile = bWriteToFile;
    m_sOutputFilePath = sOutpuFilePath;

    //Open file to write to
    if(m_bWriteToFile)
        m_outputFileStream.open(m_sOutputFilePath.append("/TMSi_Sample_Data.txt").toStdString(), ios::trunc); //ios::trunc deletes old file data

    //Check if device handler already exists and a connection was established before
    if(m_HandleMaster != NULL)
    {
        m_oFpClose(m_HandleMaster);
        m_HandleMaster = NULL;
    }

    //Get the device path connected
    ULONG maxDevices = 0;

    if(m_oFpGetInstanceId == NULL)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Could not get instance id of the device" << endl;
        return false;
    }
    PSP_DEVICE_PATH m_PSPDPMasterDevicePath = m_oFpGetInstanceId(0 , TRUE, &maxDevices);

    //Check if a Refa device is connected
    if(maxDevices<1)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - No connected device was found" << endl;
        return false;
    }

    //Open master device
    m_HandleMaster = m_oFpOpen(m_PSPDPMasterDevicePath);

    //Stop the device from sampling. Reason for this: just in case the device was not stopped correctly after the last sampling process
    m_oFpStop(m_HandleMaster);

    if(m_HandleMaster == INVALID_HANDLE_VALUE)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to open connected device" << endl;
        return false;
    }

    //Initialise and set up (sample rate/frequency and buffer size) the intern driver signal buffer which is used by the driver to store the value
    ULONG iSamplingFrequencyMilliHertz = m_uiSamplingFrequency*1000;    //Times 1000 because the driver works in millihertz
    ULONG iBufferSize = MAX_BUFFER_SIZE;                                //see TMSi doc file for more info. This size is not defined in bytes but in the number of elements which are to be sampled. A sample in this case is one conversion result for all input channels..

    if(!m_oFpSetSignalBuffer(m_HandleMaster, &iSamplingFrequencyMilliHertz, &iBufferSize))
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to allocate signal buffer" << endl;
        return false;
    }

    //Start the sampling process
    bool start = m_oFpStart(m_HandleMaster);
    if(!start)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to start the sampling procedure" << endl;
        return false;
    }

    //Get information about the signal format created by the device - UnitExponent, UnitGain, UnitOffSet
    PSIGNAL_FORMAT pSignalFormat = m_oFpGetSignalFormat(m_HandleMaster, NULL);

    if(pSignalFormat != NULL)
    {
        wcscpy_s(m_wcDeviceName, pSignalFormat->PortName);
        m_ulSerialNumber = pSignalFormat->SerialNumber;
        m_uiNumberOfAvailableChannels = pSignalFormat[0].Elements;

        if(m_bWriteToFile)
            m_outputFileStream << "Found "<< m_wcDeviceName << " device (" << m_ulSerialNumber << ") with " << m_uiNumberOfAvailableChannels << " available channels" << endl << endl;


        for(uint i = 0 ; i < m_uiNumberOfAvailableChannels; i++ )
        {
            m_vExponentChannel.push_back(pSignalFormat[i].UnitExponent);
            m_vUnitGain.push_back(pSignalFormat[i].UnitGain);
            m_vUnitOffSet.push_back(pSignalFormat[i].UnitOffSet);

            if(m_bWriteToFile)
                m_outputFileStream << "Channel number: " << i << " has type " << pSignalFormat[i].Type << " , format " << pSignalFormat[i].Format << " exponent " << pSignalFormat[i].UnitExponent << " gain " << pSignalFormat[i].UnitGain << " offset " << pSignalFormat[i].UnitOffSet << endl;
        }

        if(m_bWriteToFile)
            m_outputFileStream << endl;
    }

    //Create the buffers
    //The sampling frequency is not needed here because it is only used to specify the internal buffer size used by the driver with setSignalBuffer()
    m_lSignalBufferSize = m_uiSamplesPerBlock*m_uiNumberOfAvailableChannels*4;
    m_lSignalBuffer = new LONG[m_lSignalBufferSize];

    cout << "Plugin TMSI - INFO - initDevice() - The device has been connected and initialised successfully" << endl;
    m_bInitDeviceSuccess = true;
    return true;
}


//*************************************************************************************************************

bool TMSIDriver::uninitDevice()
{
    //Clear the buffer which is used to store the received samples
    m_vSampleBlockBuffer.clear();

    //Check if the device was initialised
    if(!m_bInitDeviceSuccess)
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Device was not initialised - therefore can not be uninitialised" << endl;
        return false;
    }

    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Driver DLL was not loaded" << endl;
        return false;
    }

    //Close the output stream/file
    if(m_bWriteToFile)
        m_outputFileStream.close();

    if(!m_oFpStop(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Failed to stop the device" << endl;
        return false;
    }

    if(!m_oFpReset(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Failed to reset the device" << endl;
        return false;
    }

    if(!m_oFpClose(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Failed to close the device" << endl;
        return false;
    }

    //Reset to NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;
    m_PSPDPMasterDevicePath = NULL;
    m_lSignalBuffer = NULL;

    cout << "Plugin TMSI - INFO - uninitDevice() - Successfully uninitialised the device" << endl;
    return true;
}


//*************************************************************************************************************

 bool TMSIDriver::getSampleMatrixValue(MatrixXf& sampleMatrix)
{
    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
        return false;

    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        cout << "Plugin TMSI - ERROR - getSampleMatrixValue() - Cannot start to get samples from device because device was not initialised correctly" << endl;
        return false;
    }

    sampleMatrix.setZero(); // Clear matrix - set all elements to zero
    uint iSamplesWrittenToMatrix = 0;
    int channelMax = 0;
    int sampleMax = 0;
    int sampleIterator = 0;

    //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
    while(iSamplesWrittenToMatrix < m_uiSamplesPerBlock)
    {
        //Get sample block from device
        ULONG ulSizeSamples = m_oFpGetSamples(m_HandleMaster, (PULONG)m_lSignalBuffer, m_lSignalBufferSize);
        ULONG ulNumSamplesReceived = ulSizeSamples/(m_uiNumberOfAvailableChannels*4);

        //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
        if(ulNumSamplesReceived > 0)
        {
            int actualSamplesWritten = 0; //Holds the number of samples which are actually written to the matrix in this while procedure

            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. The  are then written to the next matrix (block)
            for(uint i=0; i<ulNumSamplesReceived*m_uiNumberOfAvailableChannels; i++)
                m_vSampleBlockBuffer.push_back((double)m_lSignalBuffer[i]);

            //If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
            if(m_uiNumberOfAvailableChannels < m_uiNumberOfChannels)
                channelMax = m_uiNumberOfAvailableChannels;
            else
                channelMax = m_uiNumberOfChannels;

            //If the number of the samples which were already written to the matrix plus the last received number of samples is larger then the defined block size
            //-> only fill until the matrix is completeley filled with samples. The other (unused) samples are still stored in the vector buffer m_vSampleBlockBuffer and will be used in the next matrix which is to be sent to the circualr buffer by the producer
            if(iSamplesWrittenToMatrix + ulNumSamplesReceived > m_uiSamplesPerBlock)
                sampleMax = m_uiSamplesPerBlock - iSamplesWrittenToMatrix + sampleIterator;
            else
                sampleMax = ulNumSamplesReceived + sampleIterator;

            //Read the needed number of samples from the vector buffer to store them in the matrix
            for(; sampleIterator < sampleMax; sampleIterator++)
            {
                for(int channelIterator = 0; channelIterator < channelMax; channelIterator++)
                {
                    sampleMatrix(channelIterator, sampleIterator) = (m_vSampleBlockBuffer.first())*(m_bUseUnitGain ? m_vUnitGain[channelIterator] : 1) + (m_bUseUnitOffset ? m_vUnitOffSet[channelIterator] : 0) * (m_bUseChExponent ? pow(10., (m_bConvertToVolt ? (double)m_vExponentChannel[channelIterator]+6 : (double)m_vExponentChannel[channelIterator])) : 1);
                    m_vSampleBlockBuffer.pop_front();
                }

                actualSamplesWritten ++;
            }

            iSamplesWrittenToMatrix = iSamplesWrittenToMatrix + actualSamplesWritten;
        }

        m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;
        m_outputFileStream << "sampleMax: " << sampleMax << endl;
        m_outputFileStream << "sampleIterator: " << sampleIterator << endl;
        m_outputFileStream << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << endl << endl;
    }

    if(m_outputFileStream.is_open() && m_bWriteToFile)
    {
        ULONG Overflow;
        ULONG PercentFull;

        m_oFpGetBufferInfo(m_HandleMaster, &Overflow, &PercentFull);

        m_outputFileStream << "Plugin TMSI - INFO - Internal driver buffer is " << PercentFull << "% full" << endl;
        m_outputFileStream << sampleMatrix.block(0, 0, channelMax, m_uiSamplesPerBlock) << endl << endl;
    }

    return true;


//    //---- Fill up with zeros -> the function does not wait until the block is completley filled with values ------------------------------------------

//    ULONG ulSizeSamples = m_oFpGetSamples(m_HandleMaster, (PULONG)m_lSignalBuffer, m_lSignalBufferSize);
//    ULONG ulNumSamplesReceived = ulSizeSamples/(m_uiNumberOfAvailableChannels*4);

//    //Only read from buffer if at least one sample was received otherwise return false
//    if(ulNumSamplesReceived<1)
//    {
//        sampleMatrix.setZero();
//        //cout << "Plugin TMSI - ERROR - getSampleMatrixValue() - No samples received from device" << endl;
//        return false;
//    }
//    else
//    {
//        ULONG Overflow;
//        ULONG PercentFull;

//        m_oFpGetBufferInfo(m_HandleMaster, &Overflow, &PercentFull);

//        //Read the sample block out of the signal buffer (m_ulSignalBuffer) and write them to the sample buffer (m_pSample)
//        sampleMatrix.setZero(); // Clear matrix - set all elements to zero
//        int channelMax;
//        int sampleMax;

//        if(ulNumSamplesReceived<m_uiSamplesPerBlock) //If the number of received samples is smaller than the number defined by the user -> set the sampleMax to the smaller number
//            sampleMax = ulNumSamplesReceived;
//        else
//            sampleMax = m_uiSamplesPerBlock;

//        if(m_uiNumberOfAvailableChannels<m_uiNumberOfChannels)//If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
//            channelMax = m_uiNumberOfAvailableChannels;
//        else
//            channelMax = m_uiNumberOfChannels;

//        for(int channel = 0; channel<channelMax; channel++)
//        {
//            for(int sample = 0; sample<sampleMax; sample++)
//            {
//                sampleMatrix(channel, sample) = (float)(((float)m_lSignalBuffer[(m_uiNumberOfChannels*sample)+channel]*(m_bUseUnitGain ? m_vUnitGain[channel] : 1) + (m_bUseUnitOffset ? m_vUnitOffSet[channel] : 0)) * (m_bUseChExponent ? pow(10., (m_bConvertToVolt ? (double)m_vExponentChannel[channel]+6 : (double)m_vExponentChannel[channel])) : 1));
//            }
//        }

//        //cout << sampleMatrix.block(0, 0, channelMax, sampleMax) << endl << endl;

//        //write to file
//        if(m_outputFileStream.is_open() && m_bWriteToFile)
//        {
//            m_outputFileStream << "Plugin TMSI - INFO - Internal driver buffer is " << PercentFull << "% full" << endl;
//            m_outputFileStream << "Plugin TMSI - INFO - " << ulSizeSamples << " bytes of " << ulNumSamplesReceived << " samples received from device" << endl;
//            m_outputFileStream << sampleMatrix.block(0, 0, m_uiNumberOfChannels, m_uiSamplesPerBlock) << endl << endl;
//        }
//    }

//    return true;
}


//*************************************************************************************************************


