//=============================================================================================================
/**
 * @file     tmsidriver.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the implementation of the TMSIDriver class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsidriver.h"
#include "tmsiproducer.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;

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
, m_bUseChExponent(false)
, m_bUseUnitGain(false)
, m_bUseUnitOffset(false)
, m_bWriteDriverDebugToFile(false)
, m_bUseCommonAverage(false)
, m_bMeasureImpedances(false)
{
    //Initialise NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;
    m_PSPDPMasterDevicePath = NULL;
    m_lSignalBuffer = NULL;

    //Open library
//    TCHAR Path[MAX_PATH];
//    GetSystemDirectory(Path, sizeof(Path) / sizeof(TCHAR) );
//    lstrcat(Path, _T("\\TMSiSDK.dll"));
//    m_oLibHandle = LoadLibrary(Path);

    //Check which driver dll to take: TMSiSDK.dll oder TMSiSDK32bit.dll
//    if(TMSISDK)
#ifdef TAKE_TMSISDK_DLL //32 bit system & 64 bit (with 64 bit compiler)
        m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\TMSiSDK.dll");
#elif TAKE_TMSISDK_32_DLL //64 bit (with 32 bit compiler)
//    if(TMSISDK32)
        m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\SysWOW64\\TMSiSDK32bit.dll");
#endif

    //If dll can't be open return
    if( m_oLibHandle == NULL)
    {
        cout << "Plugin TMSI - ERROR - Couldn't load DLL - Check if the driver for the TMSi USB Fiber Connector installed in the system dir" << endl;
        m_bDllLoaded = false;
        return;
    }

    //Load DLL methods for initializing the driver
    __load_dll_func__(m_oFpOpen, POPEN, "Open");
    __load_dll_func__(m_oFpClose, PCLOSE, "Close");
    __load_dll_func__(m_oFpStart, PSTART, "Start");
    __load_dll_func__(m_oFpStop, PSTOP, "Stop");
    __load_dll_func__(m_oFpGetSignalFormat, PGETSIGNALFORMAT, "GetSignalFormat");
    __load_dll_func__(m_oFpSetSignalBuffer, PSETSIGNALBUFFER, "SetSignalBuffer");
    __load_dll_func__(m_oFpGetSamples, PGETSAMPLES, "GetSamples");
    __load_dll_func__(m_oFpGetBufferInfo, PGETBUFFERINFO, "GetBufferInfo");
    __load_dll_func__(m_oFpFree, PFREE, "Free");
    __load_dll_func__(m_oFpLibraryInit, PLIBRARYINIT, "LibraryInit");
    __load_dll_func__(m_oFpLibraryExit, PLIBRARYEXIT, "LibraryExit");
    __load_dll_func__(m_oFpGetDeviceList, PGETDEVICELIST, "GetDeviceList");
    __load_dll_func__(m_oFpGetFrontEndInfo, PGETFRONTENDINFO, "GetFrontEndInfo");
    __load_dll_func__(m_oFpSetRefCalculation, PSETREFCALCULATION, "SetRefCalculation");
    __load_dll_func__(m_oFpSetMeasuringMode, PSETMEASURINGMODE, "SetMeasuringMode");
    __load_dll_func__(m_oFpGetErrorCode, PGETERRORCODE, "GetErrorCode");

    cout << "Plugin TMSI - INFO - TMSIDriver() - Successfully loaded all DLL functions" << endl;
}

//=============================================================================================================

TMSIDriver::~TMSIDriver()
{
    //cout << "TMSIDriver::~TMSIDriver()" << endl;
}

//=============================================================================================================

bool TMSIDriver::initDevice(int iNumberOfChannels,
                            int iSamplingFrequency,
                            int iSamplesPerBlock,
                            bool bUseChExponent,
                            bool bUseUnitGain,
                            bool bUseUnitOffset,
                            bool bWriteDriverDebugToFile,
                            bool bUseCommonAverage,
                            bool bMeasureImpedance)
{
    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
        return false;

    //Set global variables
    m_uiNumberOfChannels = iNumberOfChannels;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_uiSamplesPerBlock = iSamplesPerBlock;
    m_bUseChExponent = bUseChExponent;
    m_bUseUnitGain = bUseUnitGain;
    m_bUseUnitOffset = bUseUnitOffset;
    m_bWriteDriverDebugToFile = bWriteDriverDebugToFile;
    m_bUseCommonAverage = bUseCommonAverage;
    m_bMeasureImpedances = bMeasureImpedance;

    //Open file to write to
    if(m_bWriteDriverDebugToFile)
        m_outputFileStream.open("../resources/mne_scan/plugins/tmsi/TMSi_Driver_Debug.txt", ios::trunc); //ios::trunc deletes old file data

    //Check if device handler already exists and a connection was established before
//    if(m_HandleMaster != NULL)
//    {
//        m_oFpClose(m_HandleMaster);
//        m_HandleMaster = NULL;
//    }
    int ErrorCode = 0;
    m_HandleMaster = m_oFpLibraryInit(TMSiConnectionUSB, &ErrorCode);

    if( ErrorCode != 0 )
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Can not initialize library" << endl;
        return false;
    }

    //Get the device list of connected devices
    char **DeviceList = NULL;
    int NrOfDevices=0;

    DeviceList = m_oFpGetDeviceList(m_HandleMaster, &NrOfDevices);

    if( NrOfDevices == 0 )
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Frontend list NOT available - Maybe no devices are connected" << endl;
        m_oFpLibraryExit(m_HandleMaster);
        return false;
    }

    //Open device
    BOOLEAN Status;
    char *DeviceLocator = DeviceList[0] ;
    Status = m_oFpOpen(m_HandleMaster, DeviceLocator);

    //Stop the device from sampling. Just in case the device was not stopped correctly after the last sampling process
    m_oFpStop(m_HandleMaster);

    if(!Status)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to open connected device" << endl;
        m_oFpLibraryExit(m_HandleMaster);
        return false;
    }

    // Turn on the impendance mode
    ULONG impedanceMode = 3;
    ULONG normalMode = 0;

    if(m_bMeasureImpedances)
    {
        if(m_oFpSetMeasuringMode(m_HandleMaster, impedanceMode, 1))
            cout << "Plugin TMSI - INFO - Now measuring impedances" << endl;
        else
        {
            int ErrorCode = m_oFpGetErrorCode(m_HandleMaster);
            cout << "Unable to set Measuremode impedance, errorcode = " << ErrorCode << endl;
        }
    }
    else
        m_oFpSetMeasuringMode(m_HandleMaster, normalMode, 0);

    //Get information about the connected device
    FRONTENDINFO FrontEndInfo;
    Status = m_oFpGetFrontEndInfo(m_HandleMaster, &FrontEndInfo);
    unsigned short serial, hwVersion, swVersion, baseSf, maxRS232, nrOfChannels;

    if(!Status)
        cout << "Plugin TMSI - ERROR - initDevice() - FrontendInfo NOT available" << endl;
    else
    {
        serial = FrontEndInfo.Serial;
        hwVersion = FrontEndInfo.HwVersion;
        swVersion = FrontEndInfo.SwVersion;
        baseSf = FrontEndInfo.BaseSf;
        maxRS232 = FrontEndInfo.maxRS232;
        nrOfChannels = FrontEndInfo.NrOfChannels;
    }

    // Set Ref Calculation
    if(m_bUseCommonAverage)
    {
        BOOLEAN setRefCalculation = m_oFpSetRefCalculation(m_HandleMaster, 1);
        if(setRefCalculation)
            cout << "Plugin TMSI - INFO - initDevice() - Common average now active" << endl;
        else
            cout << "Plugin TMSI - INFO - initDevice() - Common average is inactive (Could not be initiated)" << endl;
    }

    //Get information about the signal format created by the device - UnitExponent, UnitGain, UnitOffSet
    PSIGNAL_FORMAT pSignalFormat = m_oFpGetSignalFormat(m_HandleMaster, NULL);

    if(pSignalFormat != NULL)
    {
        wcscpy_s(m_wcDeviceName, pSignalFormat->PortName);
        m_ulSerialNumber = pSignalFormat->SerialNumber;
        m_uiNumberOfAvailableChannels = pSignalFormat[0].Elements;

        if(m_bWriteDriverDebugToFile)
            m_outputFileStream << "Found "<< m_wcDeviceName << " device (" << m_ulSerialNumber << ") with " << m_uiNumberOfAvailableChannels << " available channels" << endl << endl;

        for(uint i = 0 ; i < m_uiNumberOfAvailableChannels; i++ )
        {
            m_vExponentChannel.push_back(pSignalFormat[i].UnitExponent);
            m_vUnitGain.push_back(pSignalFormat[i].UnitGain);
            m_vUnitOffSet.push_back(pSignalFormat[i].UnitOffSet);

            if(m_bWriteDriverDebugToFile)
                m_outputFileStream << "Channel number: " << i << " has type " << pSignalFormat[i].Type << " , format " << pSignalFormat[i].Format << " exponent " << pSignalFormat[i].UnitExponent << " gain " << pSignalFormat[i].UnitGain << " offset " << pSignalFormat[i].UnitOffSet << endl;
        }

        if(m_bWriteDriverDebugToFile)
            m_outputFileStream << endl;
    }

    //Initialise and set up (sample rate/frequency and buffer size) the internal driver signal buffer which is used by the driver to store the value
    ULONG iSamplingFrequencyMilliHertz = m_uiSamplingFrequency*1000;    //Times 1000 because the driver works in millihertz
    ULONG iBufferSize = MAX_BUFFER_SIZE;                                //see TMSi doc file for more info. This size is not defined in bytes but in the number of elements which are to be sampled. A sample in this case is one conversion result for all input channels..

    if(!m_oFpSetSignalBuffer(m_HandleMaster, &iSamplingFrequencyMilliHertz, &iBufferSize))
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to allocate signal buffer" << endl;
        m_oFpLibraryExit(m_HandleMaster);
        return false;
    }

    //Start the sampling process
    bool start = m_oFpStart(m_HandleMaster);
    if(!start)
    {
        cout << "Plugin TMSI - ERROR - initDevice() - Failed to start the sampling procedure" << endl;
        m_oFpLibraryExit(m_HandleMaster);
        return false;
    }

    //Create the buffers
    //The sampling frequency is not needed here because it is only used to specify the internal buffer size used by the driver with setSignalBuffer()
    m_lSignalBufferSize = m_uiSamplesPerBlock*m_uiNumberOfAvailableChannels*4;
    m_lSignalBuffer = new LONG[m_lSignalBufferSize];

    cout << "Plugin TMSI - INFO - initDevice() - The device has been connected and initialised successfully" << endl;
    m_bInitDeviceSuccess = true;
    return true;
}

//=============================================================================================================

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
    if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
    {
        m_outputFileStream.close();
        m_outputFileStream.clear();
    }

    if(!m_oFpStop(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Failed to stop the device" << endl;
        return false;
    }

    if(!m_oFpClose(m_HandleMaster))
    {
        cout << "Plugin TMSI - ERROR - uninitDevice() - Failed to close the device" << endl;
        return false;
    }

    m_oFpLibraryExit(m_HandleMaster);

    //Reset to NULL pointers
    m_oLibHandle = NULL ;
    m_HandleMaster = NULL;
    m_PSPDPMasterDevicePath = NULL;
    m_lSignalBuffer = NULL;

    cout << "Plugin TMSI - INFO - uninitDevice() - Successfully uninitialised the device" << endl;
    return true;
}

//=============================================================================================================

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
        LONG ulSizeSamples = m_oFpGetSamples(m_HandleMaster, (PULONG)m_lSignalBuffer, m_lSignalBufferSize);
        LONG ulNumSamplesReceived = ulSizeSamples/(m_uiNumberOfAvailableChannels*4);

        //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
        if(ulNumSamplesReceived > 0)
        {
            int actualSamplesWritten = 0; //Holds the number of samples which are actually written to the matrix in this while procedure

            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. These are then written to the next matrix (block)
            for(int i=0; i<ulNumSamplesReceived; i++)
            {
                for(uint j=i*m_uiNumberOfAvailableChannels; j<(i*m_uiNumberOfAvailableChannels)+m_uiNumberOfChannels; j++)
                    m_vSampleBlockBuffer.push_back((double)m_lSignalBuffer[j]);
            }

            //If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
            if(m_uiNumberOfAvailableChannels < m_uiNumberOfChannels)
                channelMax = m_uiNumberOfAvailableChannels;
            else
                channelMax = m_uiNumberOfChannels;

            //If the number of the samples which were already written to the matrix plus the last received number of samples is larger then the defined block size
            //-> only fill until the matrix is completeley filled with samples. The other (unused) samples are still stored in the vector buffer m_vSampleBlockBuffer and will be used in the next matrix which is to be sent to the circular buffer
            if(iSamplesWrittenToMatrix + ulNumSamplesReceived > m_uiSamplesPerBlock)
                sampleMax = m_uiSamplesPerBlock - iSamplesWrittenToMatrix + sampleIterator;
            else
                sampleMax = ulNumSamplesReceived + sampleIterator;

            //Read the needed number of samples from the vector buffer to store them in the matrix
            for(; sampleIterator < sampleMax; sampleIterator++)
            {
                for(int channelIterator = 0; channelIterator < channelMax; channelIterator++)
                {
                    sampleMatrix(channelIterator, sampleIterator) = ((m_vSampleBlockBuffer.first() * (m_bUseUnitGain ? m_vUnitGain[channelIterator] : 1)) + (m_bUseUnitOffset ? m_vUnitOffSet[channelIterator] : 0)) * (m_bUseChExponent ? pow(10., (double)m_vExponentChannel[channelIterator]) : 1);
                    m_vSampleBlockBuffer.pop_front();
                }

                actualSamplesWritten ++;
            }

            iSamplesWrittenToMatrix = iSamplesWrittenToMatrix + actualSamplesWritten;
        }

        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
        {
            m_outputFileStream << "samples in buffer: " << m_vSampleBlockBuffer.size()/m_uiNumberOfChannels << endl;
            m_outputFileStream << "ulSizeSamples: " << ulSizeSamples << endl;
            m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;
            m_outputFileStream << "sampleMax: " << sampleMax << endl;
            m_outputFileStream << "sampleIterator: " << sampleIterator << endl;
            m_outputFileStream << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << endl << endl;
        }
    }

    if(/*m_outputFileStream.is_open() &&*/ m_bWriteDriverDebugToFile)
    {
        //Get device buffer info
        ULONG ulOverflow;
        ULONG ulPercentFull;
        m_oFpGetBufferInfo(m_HandleMaster, &ulOverflow, &ulPercentFull);

        m_outputFileStream <<  "Unit offset: " << endl;
        for(int w = 0; w<<m_vUnitOffSet.size(); w++)
            cout << float(m_vUnitOffSet[w]) << "  ";
        m_outputFileStream << endl << endl;

        m_outputFileStream <<  "Unit gain: " << endl;
        for(int w = 0; w<<m_vUnitGain.size(); w++)
            m_outputFileStream << float(m_vUnitGain[w]) << "  ";
        m_outputFileStream << endl << endl;

        m_outputFileStream << "----------<See output file for sample matrix>----------" <<endl<<endl;
        m_outputFileStream << "----------<Internal driver buffer is "<<ulPercentFull<<" full>----------"<<endl;
        m_outputFileStream << "----------<Internal driver overflow is "<<ulOverflow<< ">----------"<<endl;
    }

    return true;
}

