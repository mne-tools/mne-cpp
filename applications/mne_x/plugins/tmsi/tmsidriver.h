//=============================================================================================================
/**
* @file     tmsidriver.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
* @brief    Contains the declaration of the tmsidriver class.
*
*/

#ifndef TMSIDRIVER_H
#define TMSIDRIVER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <tchar.h>
#include <string.h>
#include <windows.h>
#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace TMSIPlugin
{


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
// USED NAMESPACES
//=============================================================================================================

using namespace std;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSIProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define MAX_BUFFER_SIZE 0xFFFFFFFF

//DLL Function loader define
#define __load_dll_func__(var, type, name) \
    var = (type)::GetProcAddress(m_oLibHandle, name); \
    if(!var) \
        cout<< "Plugin TMSI - ERROR - Error loading method " << name << "\n"; \


//=============================================================================================================
/**
* TMSIDriver
*
* @brief The TMSIDriver class provides real time data acquisition of EEG data with a TMSi Refa device.
*/
class TMSIDriver
{
public:
    //=========================================================================================================
    /**
    * Constructs a TMSIDriver.
    *
    * @param [in] pTMSIProducer a pointer to the corresponding TMSI Producer class.
    */
    TMSIDriver(TMSIProducer* pTMSIProducer);

    //=========================================================================================================
    /**
    * Destroys the TMSIDriver.
    */
    ~TMSIDriver();

    //=========================================================================================================
    /**
    * Get sample from the device in form of a mtrix.
    * @param [in] MatrixXf the block sample values in form of a matrix.
    * @param [out] bool returns true if sample was successfully written to the input variable, false otherwise.
    */
    bool getSampleMatrixValue(MatrixXf& sampleMatrix);

    //=========================================================================================================
    /**
    * Initialise device.
    * @param [in] iNumberOfChannels number of channels specified by the user.
    * @param [in] iSamplingFrequency sampling frequency specified by the user.
    * @param [in] iSamplesPerBlock samples per block specified by the user.
    * @param [out] bool returns true if device was successfully initialised, false otherwise.
    */
    bool initDevice(int iNumberOfChannels, int iSamplingFrequency, int iSamplesPerBlock);

    //=========================================================================================================
    /**
    * Uninitialise device.
    * @param [out] bool returns true if device was successfully uninitialised, false otherwise.
    */
    bool uninitDevice();

protected:
    //=========================================================================================================
    /**
    * Checks if a device is connected to the computer.
    * @param [out] bool returns true if device is connected, false otherwise.
    */
    bool deviceConnected();

private:
    QSharedPointer<TMSIProducer>    m_pTMSIProducer;    /**< A pointer to the corresponding TMSIProducer class.*/

    bool                m_bInitDeviceSuccess;

    uint                m_uiNumberOfChannels;
    uint                m_uiSamplingFrequency;           /**< The sampling frequency in millihertz;.*/
    uint                m_uiSamplesPerBlock;

    //Device handle Master
    HANDLE              m_HandleMaster;

    //Lib handle
    HINSTANCE           m_oLibHandle;

    //device info
    WCHAR               m_wcDeviceName[40];             /**< m_vDevicePathMap contains the connected devicePath.*/
    ULONG               m_ulSerialNumber;
    PSP_DEVICE_PATH     m_PSPDPMasterDevicePath;

    //signal info
    ULONG               m_iNumberOfAvailableChannels;

    //Buffer
    ULONG *             m_ulSignalBuffer;               /**< Buffer in which the device can write the samples.*/
    LONG                m_lSignalBufferSize ;           /**< Size of m_ulSignalBuffer = (samples per block) * (number of channels) * 4 (4 because every signal value takes 4 bytes - see TMSi SDK doc).*/
    float *             m_fSampleBuffer;                /**< Buffer for one sample read from the device signal buffer.*/

    //Signal formats fo every channel
    vector <LONG>       m_vExponentChannel;
    vector <FLOAT>      m_vUnitGain;
    vector <FLOAT>      m_vUnitOffSet;

    // Variables used for loading the RTINST.DLL methods
    POPEN               m_oFpOpen;
    PCLOSE              m_oFpClose;
    PGETDEVICESTATE     m_oFpGetDeviceState;
    PSTART              m_oFpStart;
    PRESETDEVICE        m_oFpReset;
    PSTOP               m_oFpStop;
    PGETSLAVEHANDLE     m_oFpGetSlaveHandle;
    PADDSLAVE           m_oFpAddSlave;
    PGETSIGNALFORMAT    m_oFpGetSignalFormat;
    PSETSIGNALBUFFER    m_oFpSetSignalBuffer;
    PGETSAMPLES         m_oFpGetSamples;
    PGETBUFFERINFO      m_oFpGetBufferInfo;
    PDEVICEFEATURE      m_oFpDeviceFeature;
    PGETINSTANCEID      m_oFpGetInstanceId;
    POPENREGKEY         m_oFpOpenRegKey;
    PFREE               m_oFpFree;
};

} // NAMESPACE

#endif // TMSIDRIVER_H
