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
// Structure Typedefs - structure define as used in the RTINST.DLL
//=============================================================================================================

typedef struct _SP_DEVICE_PATH
{
    DWORD  dwCbSize;
    TCHAR  devicePath[1];
} SP_DEVICE_PATH, *PSP_DEVICE_PATH;

typedef struct _FeatureData
{
    ULONG FeatureId;
    ULONG Info;
} FEATURE_DATA, *PFEATURE_DATA;

typedef struct _SYSTEM_TIME
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEM_TIME;

typedef struct _SIGNAL_FORMAT
{
    ULONG Size;      // Size of this structure
    ULONG Elements;  // Number of elements in list

    ULONG Type;      // One of the signal types above
    ULONG SubType;   // One of the signal sub-types above
    ULONG Format;    // Float / Integer / Asci / Ect..
    ULONG Bytes;     // Number of bytes per sample including subsignals

    FLOAT UnitGain;
    FLOAT UnitOffSet;
    ULONG UnitId;
    LONG UnitExponent;

    WCHAR Name[40];

    ULONG Port;
    WCHAR PortName[40];
    ULONG SerialNumber;
} SIGNAL_FORMAT, *PSIGNAL_FORMAT;

typedef struct _FeatureMemory{
    FEATURE_DATA Feature;
    ULONG Data[1];
}FEATURE_MEMORY, *PFEATURE_MEMORY;

typedef struct _FeatureMode{
    FEATURE_DATA Feature;
    ULONG Mode;
}FEATURE_MODE,*PFEATURE_MODE;


//*************************************************************************************************************
//=============================================================================================================
// Method Typedefs - method defines as used in the RTINST.DLL
//=============================================================================================================

typedef HANDLE          ( __stdcall * POPEN)            (PSP_DEVICE_PATH DevicePath);
typedef BOOL            ( __stdcall * PCLOSE)           (HANDLE hHandle);
typedef ULONG           ( __stdcall * PGETDEVICESTATE)  (IN HANDLE Handle);
typedef BOOLEAN         ( __stdcall * PSTART)           (IN HANDLE Handle);
typedef BOOLEAN         ( __stdcall * PRESETDEVICE)     (IN HANDLE Handle);
typedef BOOLEAN         ( __stdcall * PSTOP)            (IN HANDLE Handle);
typedef HANDLE          ( __stdcall * PGETSLAVEHANDLE)  (IN HANDLE Handle);
typedef BOOLEAN         ( __stdcall * PADDSLAVE)        (IN HANDLE Handle, IN HANDLE SlaveHandle);
typedef PSIGNAL_FORMAT  ( __stdcall * PGETSIGNALFORMAT) (IN HANDLE Handle, IN OUT PSIGNAL_FORMAT pSignalFormat);
typedef BOOLEAN         ( __stdcall * PSETSIGNALBUFFER) (IN HANDLE Handle, IN OUT PULONG SampleRate, IN OUT PULONG BufferSize);
typedef ULONG           ( __stdcall * PGETSAMPLES)      (IN HANDLE Handle, OUT PULONG SampleBuffer, IN ULONG Size);
typedef BOOLEAN         ( __stdcall * PGETBUFFERINFO)   (IN HANDLE Handle, OUT PULONG Overflow, OUT PULONG PercentFull);
typedef BOOLEAN         ( __stdcall * PDEVICEFEATURE)   (IN HANDLE Handle, IN LPVOID DataIn, IN DWORD InSize, OUT LPVOID DataOut, IN DWORD OutSize);
typedef PSP_DEVICE_PATH ( __stdcall * PGETINSTANCEID)   (IN LONG DeviceIndex, IN BOOLEAN Present, OUT ULONG  *MaxDevices );
typedef HKEY            ( __stdcall * POPENREGKEY)      (IN PSP_DEVICE_PATH Path );
typedef BOOL            ( __stdcall * PFREE)            (IN VOID *Memory);


//*************************************************************************************************************
//=============================================================================================================
// Variables used for loading the RTINST.DLL methods
//=============================================================================================================

POPEN m_oFpOpen;
PCLOSE m_oFpClose;
PGETDEVICESTATE m_oFpGetDeviceState;
PSTART m_oFpStart;
PRESETDEVICE m_oFpReset;
PSTOP m_oFpStop;
PGETSLAVEHANDLE m_oFpGetSlaveHandle;
PADDSLAVE m_oFpAddSlave;
PGETSIGNALFORMAT m_oFpGetSignalFormat;
PSETSIGNALBUFFER m_oFpSetSignalBuffer;
PGETSAMPLES m_oFpGetSamples;
PGETBUFFERINFO m_oFpGetBufferInfo;
PDEVICEFEATURE m_oFpDeviceFeature;
PGETINSTANCEID m_oFpGetInstanceId;
POPENREGKEY m_oFpOpenRegKey;
PFREE m_oFpFree;


//*************************************************************************************************************
//=============================================================================================================
// Handler, buffer declarations
//=============================================================================================================

//Device handle Master
HANDLE m_HandleMaster;

//Lib handle
HINSTANCE m_oLibHandle;

//device info
TCHAR m_cDevicePathAndSerialNumber[1024]; //m_vDevicePathMap contains the connected devicePath

//signal info
uint m_iNumberOfAvailableChannels;

//store value for calculating the data
vector <LONG>  m_vExponentChannel;
vector <FLOAT> m_vUnitGain;
vector <FLOAT> m_vUnitOffSet;


//*************************************************************************************************************
//=============================================================================================================
// DLL Function loader define
//=============================================================================================================

#define __load_dll_func__(var, type, name) \
    var = (type)::GetProcAddress(m_oLibHandle, name); \
    if(!var) \
        cout<< "Plugin TMSI - Error loading method " << name << "\n"; \


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIDriver::TMSIDriver(TMSIProducer* pTMSIProducer)
: m_pTMSIProducer(pTMSIProducer)
, m_iNumberOfChannels(32)
, m_iSamplingFrequency(512)
, m_iSamplesPerBlock(32)
{
    //Open library
    m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\RTINST.DLL");

    //if it can't be open return FALSE;
    if( m_oLibHandle == NULL)
    {
        cout << "Plugin TMSI - Couldn't load DLL in 'C:\\Windows\\System32\\RTINST.DLL' - Is the driver for the TMSi USB Fiber Connector installed?" << endl;
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

    cout << "Plugin TMSI - Successfully loaded all DLL functions" << endl;
}


//*************************************************************************************************************

TMSIDriver::~TMSIDriver()
{
}


//*************************************************************************************************************

MatrixXf TMSIDriver::getSampleMatrixValue()
{
    MatrixXf sampleValue;

    return sampleValue;
}


//*************************************************************************************************************

bool TMSIDriver::InitDevice()
{
    //Get the device path connected
    ULONG maxDevices = 0;

    PSP_DEVICE_PATH device = m_oFpGetInstanceId(0 , TRUE, &maxDevices);

    // get the name corresponding to this device
    HKEY hKey = m_oFpOpenRegKey(device);

    if(hKey != INVALID_HANDLE_VALUE)
    {
        ULONG serialNumber = 0;
        char deviceNameTemp[80] = "Unknown Device";
        string deviceName = deviceNameTemp;

        //get the serial number of the device
        DWORD sizeSerial = sizeof(serialNumber);
        ::RegQueryValueEx(hKey, L"DeviceSerialNumber", NULL, NULL, (PBYTE)&serialNumber, &sizeSerial);

        //get the name of the device
        DWORD sizeDesc = sizeof(deviceName);
        ::RegQueryValueEx(hKey, L"DeviceDescription", NULL, NULL, (PBYTE)&deviceName, &sizeDesc);

        //TODO: Get rid of \0 escape sequences in the char. Why is RegQueryValueEx() adding \0's?
        //        TCHAR deviceNameTemp[80];

        //        for(int i=0; i<79; i++)
        //        {
        //            if(&deviceName[i]=="\0")
        //                cout<<i<<endl;
        //        }

        ::_tprintf(m_cDevicePathAndSerialNumber, "%s %d", deviceName, serialNumber);

        cout << "Plugin TMSI - Found device " << m_cDevicePathAndSerialNumber << endl;

        ::RegCloseKey(hKey);
    }
    else
    {
        cout << "Plugin TMSI - Invalid registry handle" << endl;
        return false;
    }

    //Check if a Refa device is connected
    if(maxDevices==0)
    {
        cout << "Plugin TMSI - There was no connected device found" << endl;
        return false;
    }

    //Open master device
    m_HandleMaster = m_oFpOpen(device);
    if(!m_HandleMaster)
    {
        cout << "Plugin TMSI - Failed to open connected device" << endl;
        return false;
    }

    //Initialise and set up the signal buffer
    ULONG iSampleRate = m_iSamplingFrequency*1000; //Times 1000 because the driver works in millihertz
    ULONG iBufferSize = MAX_BUFFER_SIZE;
    if(!m_oFpSetSignalBuffer(m_HandleMaster, &iSampleRate, &iBufferSize))
    {
        cout << "Plugin TMSI - Failed to allocate signal buffer" << endl;
        return false;
    }

    //Start the sampling process
    bool start = m_oFpStart(m_HandleMaster);
    if(!start)
    {
        cout << "Plugin TMSI - Failed to start the sampling procedure" << endl;
        return false;
    }

    //Get information about the signal format created by the device - UnitExponent, UnitGain, UnitOffSet
    PSIGNAL_FORMAT pSignalFormat = m_oFpGetSignalFormat(m_HandleMaster, NULL);

    if(pSignalFormat != NULL)
    {
        cout << "Plugin TMSI - Master device name: " << (char*)pSignalFormat[0].PortName << endl;
        cout << "Plugin TMSI - Number of available channels: " << (uint)pSignalFormat[0].Elements << endl;
        m_iNumberOfAvailableChannels = pSignalFormat[0].Elements;

        for(uint i = 0 ; i < m_iNumberOfAvailableChannels; i++ )
        {
            m_vExponentChannel.push_back(pSignalFormat[i].UnitExponent+6/*changed measure unit in V*/);
            m_vUnitGain.push_back(pSignalFormat[i].UnitGain);
            m_vUnitOffSet.push_back(pSignalFormat[i].UnitOffSet);
        }
    }

    //Create the signal buffer which we want to write to


    cout << "Plugin TMSI - The device has been connected and initialised successfully" << endl;
    return true;
}


//*************************************************************************************************************


