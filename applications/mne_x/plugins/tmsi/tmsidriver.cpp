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
, m_bInitDeviceSuccess(false)
, m_iNumberOfChannels(64)
, m_iSamplingFrequency(2048)
, m_iSamplesPerBlock(32)
{
    //Initialise NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;

    //Open library
    m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\RTINST.DLL");

    //If it can't be open return
    if( m_oLibHandle == NULL)
    {
        cout << "Plugin TMSI - ERROR - Couldn't load DLL in 'C:\\Windows\\System32\\RTINST.DLL' - Is the driver for the TMSi USB Fiber Connector installed?" << endl;
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

    cout << "Plugin TMSI - INFO - Successfully loaded all DLL functions" << endl;
}


//*************************************************************************************************************

TMSIDriver::~TMSIDriver()
{
    this->uninitDevice();
}


//*************************************************************************************************************

bool TMSIDriver::initDevice(int iNumberOfChannels, int iSamplingFrequency, int iSamplesPerBlock)
{
    m_iNumberOfChannels = iNumberOfChannels;
    m_iSamplingFrequency = iSamplingFrequency;
    m_iSamplesPerBlock = iSamplesPerBlock;

    //Check if device handler already exists and a connection was therefore established before
    if(m_HandleMaster != NULL)
    {
        m_oFpClose(m_HandleMaster);
        m_HandleMaster = NULL;
    }

    //Get the device path connected
    ULONG maxDevices = 0;

    if(m_oFpGetInstanceId == NULL)
    {
        cout << "Plugin TMSI - ERROR - Could not get instance id of the device" << endl;
        return false;
    }
    PSP_DEVICE_PATH m_PSPDPMasterDevicePath = m_oFpGetInstanceId(0 , TRUE, &maxDevices);

    //Check if a Refa device is connected
    if(maxDevices<1)
    {
        cout << "Plugin TMSI - ERROR - There was no connected device found" << endl;
        return false;
    }

    //Open master device
    m_HandleMaster = m_oFpOpen(m_PSPDPMasterDevicePath);
    if(m_HandleMaster == INVALID_HANDLE_VALUE)
    {
        cout << "Plugin TMSI - ERROR - Failed to open connected device" << endl;
        return false;
    }

    //Initialise and set up (sample rate/frequency and buffer size) the intern driver signal buffer which is used by the driver to store the value
    ULONG iSamplingFrequencyMilliHertz = m_iSamplingFrequency*1000; //Times 1000 because the driver works in millihertz
    ULONG iBufferSize = m_iSamplingFrequency*m_iSamplesPerBlock;    //This size is not defined in bytes but in the number of elements which are to be sampled. A sample in this case is one conversion result for all input channels..

    if(!m_oFpSetSignalBuffer(m_HandleMaster, &iSamplingFrequencyMilliHertz, &iBufferSize))
    {
        cout << "Plugin TMSI - ERROR - Failed to allocate signal buffer" << endl;
        return false;
    }

    //Start the sampling process
    bool start = m_oFpStart(m_HandleMaster);
    if(!start)
    {
        cout << "Plugin TMSI - ERROR - Failed to start the sampling procedure" << endl;
        return false;
    }

    //Get information about the signal format created by the device - UnitExponent, UnitGain, UnitOffSet
    PSIGNAL_FORMAT pSignalFormat = m_oFpGetSignalFormat(m_HandleMaster, NULL);

    if(pSignalFormat != NULL)
    {
        cout << "Plugin TMSI - INFO - Master device name: " << (char*)pSignalFormat[0].PortName << endl;
        cout << "Plugin TMSI - INFO - Number of available channels: " << (uint)pSignalFormat[0].Elements << endl;
        m_iNumberOfAvailableChannels = pSignalFormat[0].Elements;

        for(int i = 0 ; i < m_iNumberOfAvailableChannels; i++ )
        {
            m_vExponentChannel.push_back(pSignalFormat[i].UnitExponent+6/*changed measure unit in V*/);
            m_vUnitGain.push_back(pSignalFormat[i].UnitGain);
            m_vUnitOffSet.push_back(pSignalFormat[i].UnitOffSet);
        }
    }

    //Create the buffers
    //TODO: Check if sampling frequency must be considered when specifying the buffer size
    m_afSampleBuffer = new float[m_iNumberOfChannels*m_iSamplesPerBlock*4];

    m_lSignalBufferSize = m_iSamplesPerBlock*m_iNumberOfChannels*4;
    m_aulSignalBuffer = new ULONG[m_lSignalBufferSize];

    m_bInitDeviceSuccess = true;
    return true;
}


//*************************************************************************************************************

bool TMSIDriver::uninitDevice()
{
    if(!m_oFpStop(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - Failed to stop the device" << endl;
        return false;
    }

    if(!m_oFpClose(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - Failed to close the device" << endl;
        return false;
    }

    //Reset to NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;
//    m_PSPDPMasterDevicePath = NULL;
//    m_aulSignalBuffer = NULL;
//    m_afSampleBuffer = NULL;
//    m_aulSignalBuffer = NULL;

    return true;
}

//*************************************************************************************************************

MatrixXf TMSIDriver::getSampleMatrixValue()
{
    MatrixXf sampleValue;

    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        cout << "Plugin TMSI - ERROR - Cannot start to get samples from device because device was not initialised correctly" << endl;
        return sampleValue;
    }

    //Get sample block from device
    ULONG lSizeSamples = m_oFpGetSamples(m_HandleMaster, (PULONG)m_aulSignalBuffer, m_lSignalBufferSize);

    if(lSizeSamples<1)
    {
        cout << "Plugin TMSI - ERROR - No samples received from device" << endl;
        return sampleValue;
    }
    else
        cout << "Plugin TMSI - INFO - " << lSizeSamples << " bytes of samples received from device" << endl;

    //Read the sample block out of the signal buffer (m_ulSignalBuffer) and write them to the sample buffer (m_pSample)
    //int iNumberSamplesReceived = lSizeSamples/(m_ui32NbTotalChannels*4);

    for(int channel = 0; channel<m_iNumberOfChannels ; channel++)
    {
        for(int channel = 0; channel<m_iNumberOfChannels ; channel++)
        {

        }
    }

    //Convert the m_pSample buffer into the MatrixXf format

    return sampleValue;
}





//*************************************************************************************************************


