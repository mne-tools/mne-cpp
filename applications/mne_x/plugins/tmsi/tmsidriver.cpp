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
// Handler, buffer defines
//=============================================================================================================

//Device Handle Master
HANDLE m_HandleMaster;

//Lib handle
HINSTANCE m_oLibHandle;

//Buffer for storing the samples
ULONG *m_ulSignalBuffer;
LONG *m_lSignalBuffer;
bool m_bSignalBufferUnsigned;
ULONG m_ulSampleRate ;
ULONG m_ulBufferSize ;

//device
char m_cDevicePath[1024]; //m_vDevicePathMap contains all connected devicePath and their name
string m_pDevicePathMaster; //the name of the Master devicePath chosen
vector <string> m_vDevicePathSlave; //a list with the name of the Slave devicePath chosen
ULONG m_lNrOfDevicesConnected; // Number of devices on this PC
ULONG m_lNrOfDevicesOpen; //total of Master/slaves device open

//store value for calculating the data
vector <LONG> m_vExponentChannel;
vector <FLOAT> m_vUnitGain;
vector <FLOAT> m_vUnitOffSet;

//number of channels
ULONG m_ui32NbTotalChannels;
uint m_ui32BufferSize;


//*************************************************************************************************************
//=============================================================================================================
// DLL Function loader define
//=============================================================================================================

#define __load_dll_func__(var, type, name) \
    var = (type)::GetProcAddress(m_oLibHandle, name); \
    if(!var) \
        cout<< "Error loading method " << name << "\n"; \


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSIDriver::TMSIDriver(TMSIProducer* pTMSIProducer)
: m_pTMSIProducer(pTMSIProducer)
, m_iNumberOfChannels(32)
, m_iSamplingFrequency(512)
, m_iSamplesPerblock(32)
{
    //Open library
    m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\RTINST.DLL");

    //if it can't be open return FALSE;
    if( m_oLibHandle == NULL)
    {
        cout << "Couldn't load DLL in 'C:\\Windows\\System32\\RTINST.DLL' - Is the USB Fiber Connector driver installed?" << endl;
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

    cout << "Successfully loaded all DLL functions" << endl;
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

void TMSIDriver::InitDevice()
{
    //Get the device path connected
    ULONG maxDevices = 0;

    PSP_DEVICE_PATH device = m_oFpGetInstanceId(0 , TRUE, &maxDevices);

    // get the name corresponding to this device
    HKEY hKey = m_oFpOpenRegKey(device);

    if(hKey != INVALID_HANDLE_VALUE)
    {
        ULONG sizeSerial;
        ULONG sizeDesc;

        TCHAR deviceName[] = _T("Unknown Device");
        ULONG serialNumber = 0;

        //get the serial number of the device
        sizeSerial = sizeof(serialNumber);
        ::RegQueryValueEx(hKey, "DeviceSerialNumber", NULL, NULL, (PBYTE)&serialNumber, &sizeSerial);

        //get the name of the device
        sizeDesc = sizeof( deviceName );
        ::RegQueryValueEx(hKey , "DeviceDescription", NULL, NULL, (PBYTE)&deviceName[0], &sizeDesc);
        ::sprintf(m_cDevicePath, "%s %d", deviceName, serialNumber);
        ::RegCloseKey(hKey);

        cout<<"Found device: "<< *m_cDevicePath <<endl;
    }

}


//*************************************************************************************************************


