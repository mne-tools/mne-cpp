//=============================================================================================================
/**
 * @file     tmsidriver.h
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
 * @brief    Contains the declaration of the tmsidriver class. This class implements the basic communication between MNE Scan and a TMSI Refa device
 *
 */

#ifndef TMSIDRIVER_H
#define TMSIDRIVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <tchar.h>
#include <string.h>
#include <windows.h>
#include <Eigen/Core>

//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <qapplication.h>
#include <QVector>
#include <QSysInfo>

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// Structure Typedefs - structure define as used in the TMSiSDK.dll
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

typedef enum _TMSiConnectionEnum {
    TMSiConnectionUndefined = 0,    //Undefined connection, indicates programming error
    TMSiConnectionFiber,            //Obsolete, do not use
    TMSiConnectionBluetooth,        //Bluetooth connection with Microsoft driver
    TMSiConnectionUSB,              //USB 2.0 connection direct
    TMSiConnectionWifi,             //Network connection, Ip-adress and port needed, wireless
    TMSiConnectionNetwork           //Network connection, Ip-adress and port needed, wired
} TMSiConnectionType;

typedef struct _FRONTENDINFO
{	unsigned short NrOfChannels;        //Current number of channels used
    unsigned short SampleRateSetting;   //Current sample rate setting (a.k.a. base sample rate divider )
    unsigned short Mode;                //operating mode
    unsigned short maxRS232;
    unsigned long  Serial;              //Serial number
    unsigned short NrExg;               //Number of Exg channels in this device
    unsigned short NrAux;               //Number of Aux channels in this device
    unsigned short HwVersion;           //Version number for the hardware
    unsigned short SwVersion;           //Version number of the embedded software
    unsigned short RecBufSize;          //Used for debugging only
    unsigned short SendBufSize;         //Used for debugging only
    unsigned short NrOfSwChannels;      //Max. number of channels supported by this device
    unsigned short BaseSf;              //Max. sample frequency
    unsigned short Power;
    unsigned short Check;
}FRONTENDINFO,*PFRONTENDINFO;

//=============================================================================================================
// Method Typedefs - method defines as used in the RTINST.DLL
//=============================================================================================================

// define a pointer POPEN to a function which is taking a void pointe and a char and returns a bool value
typedef BOOLEAN         ( __stdcall * POPEN)            (void *Handle, const char *DeviceLocator);
typedef BOOL            ( __stdcall * PCLOSE)           (HANDLE hHandle);
typedef BOOLEAN         ( __stdcall * PSTART)           (IN HANDLE Handle);
typedef BOOLEAN         ( __stdcall * PSTOP)            (IN HANDLE Handle);
typedef PSIGNAL_FORMAT  ( __stdcall * PGETSIGNALFORMAT) (IN HANDLE Handle, IN OUT char* FrontEndName);
typedef BOOLEAN         ( __stdcall * PSETSIGNALBUFFER) (IN HANDLE Handle, IN OUT PULONG SampleRate, IN OUT PULONG BufferSize);
typedef LONG            ( __stdcall * PGETSAMPLES)      (IN HANDLE Handle, OUT PULONG SampleBuffer, IN ULONG Size);
typedef BOOLEAN         ( __stdcall * PGETBUFFERINFO)   (IN HANDLE Handle, OUT PULONG Overflow, OUT PULONG PercentFull);
typedef BOOL            ( __stdcall * PFREE)            (IN VOID *Memory);
typedef HANDLE          ( __stdcall * PLIBRARYINIT)     (IN TMSiConnectionType GivenConnectionType, IN OUT int *ErrorCode );
typedef int             ( __stdcall * PLIBRARYEXIT)     (IN HANDLE Handle);
typedef char**          ( __stdcall * PGETDEVICELIST)   (IN HANDLE Handle, IN OUT int *NrOfFrontEnds);
typedef BOOLEAN         ( __stdcall * PGETFRONTENDINFO) (IN HANDLE Handle, IN OUT FRONTENDINFO *FrontEndInfo );
typedef BOOLEAN         ( __stdcall * PSETREFCALCULATION) (IN HANDLE Handle, IN int OnOrOff );
typedef BOOLEAN         ( __stdcall * PSETMEASURINGMODE) (IN HANDLE Handle, IN ULONG Mode, IN int Value );
typedef BOOLEAN         ( __stdcall * PGETERRORCODE) (IN HANDLE Handle);

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace std;
using namespace Eigen;

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSIProducer;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define MAX_BUFFER_SIZE 0xFFFFFFFF

//DLL Function loader define
#define __load_dll_func__(var, type, name) \
    var = (type)::GetProcAddress(m_oLibHandle, name); \
    if(!var) \
        cout<< "Plugin TMSI - ERROR - Error loading method " << name << "\n"; \

// Measurement modes:
#define MEASURE_MODE_NORMAL			((ULONG)0x0)
#define MEASURE_MODE_IMPEDANCE		((ULONG)0x1)
#define MEASURE_MODE_CALIBRATION	((ULONG)0x2)

#define MEASURE_MODE_IMPEDANCE_EX	((ULONG)0x3)
#define MEASURE_MODE_CALIBRATION_EX	((ULONG)0x4)

// for MEASURE_MODE_IMPEDANCE:
#define IC_OHM_002	0 /*!< 2K Impedance limit */
#define IC_OHM_005	1 /*!<  5K Impedance limit */
#define IC_OHM_010	2 /*!<  10K Impedance limit */
#define IC_OHM_020	3 /*!<  20K Impedance limit */
#define IC_OHM_050	4 /*!<  50K Impedance limit */
#define IC_OHM_100	5 /*!<  100K Impedance limit */
#define IC_OHM_200	6 /*!<  200K Impedance limit */

// for MEASURE_MODE_CALIBRATION:
#define IC_VOLT_050 0	/*!< 50 uV t-t Calibration voltage */
#define IC_VOLT_100 1	/*!< 100 uV t-t Calibration voltage */
#define IC_VOLT_200 2	/*!< 200 uV t-t Calibration voltage */
#define IC_VOLT_500 3	/*!< 500 uV t-t Calibration voltage */

 // for Signat Format
#define SF_UNSIGNED 0x0   // Unsigned integer
#define SF_INTEGER  0x1	  // signed integer

// integer overflow value for analog channels
#define OVERFLOW_32BITS ((long) 0x80000000)
// Get Signal info

#define SIGNAL_NAME 40

// Unit defines
#define UNIT_UNKNOWN 0	// Used for digital inputs or if the driver cannot determine the units of a channel
#define UNIT_VOLT 1		// Channel measures voltage
#define UNIT_PERCENT 2	// Channel measures a percentage
#define UNIT_BPM 3		// Beats per minute
#define UNIT_BAR 4		// Pressure in bar
#define UNIT_PSI 5		// Pressure in psi
#define UNIT_MH20 6		// Pressure calibrated to meters water
#define UNIT_MHG 7		// Pressure calibrated to meters mercury
#define UNIT_BIT 8		// Used for digital inputs

// Check windows
//#if defined (TAKE_TMSISDK_DLL)
//#define TMSISDK true
//#define TMSISDK32 false
//#endif

//#if defined (TAKE_TMSISDK_32_DLL)
//#define TMSISDK32 true
//#define TMSISDK false
//#endif

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
     * @param[in] pTMSIProducer a pointer to the corresponding TMSI Producer class.
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
     * @param[in] MatrixXf the block sample values in form of a matrix.
     * @param[in, out] bool returns true if sample was successfully written to the input variable, false otherwise.
     */
    bool getSampleMatrixValue(MatrixXf& sampleMatrix);

    //=========================================================================================================
    /**
     * Initialise device.
     * @param[in] iNumberOfChannels number of channels specified by the user.
     * @param[in] iSamplingFrequency sampling frequency specified by the user.
     * @param[in] iSamplesPerBlock samples per block specified by the user.
     * @param[in] bUseChExponent Flag for using the channels exponent. Defined by the user via the GUI.
     * @param[in] bUseUnitGain Flag for using the channels unit gain. Defined by the user via the GUI.
     * @param[in] bUseUnitOffset Flag for using the channels unit offset. Defined by the user via the GUI.
     * @param[in] bWriteDriverDebugToFile Flag for writing driver debug information to a file. Defined by the user via the GUI.
     * @param[in, out] bool returns true if device was successfully initialised, false otherwise.
     * @param[in] bUseCommonAverage Flag for using common average when recording EEG data. Defined by the user via the GUI.
     * @param[in] bMeasureImpedance Flag for measuring impedances.
     */
    bool initDevice(int iNumberOfChannels,
                    int iSamplingFrequency,
                    int iSamplesPerBlock,
                    bool bUseChExponent,
                    bool bUseUnitGain,
                    bool bUseUnitOffset,
                    bool bWriteDriverDebugToFile,
                    bool bUseCommonAverage,
                    bool bMeasureImpedance);

    //=========================================================================================================
    /**
     * Uninitialise device.
     * @param[in, out] bool returns true if device was successfully uninitialised, false otherwise.
     */
    bool uninitDevice();

private:
    TMSIProducer*       m_pTMSIProducer;                /**< A pointer to the corresponding TMSIProducer class.*/

    //Flags
    bool                m_bInitDeviceSuccess;           /**< Flag which defines if the device initialisation was successfull.*/
    bool                m_bDllLoaded;                   /**< Flag which defines if the driver DLL was loaded successfully.*/

    //User definitons
    uint                m_uiNumberOfChannels;           /**< The number of channels defined by the user via the GUI.*/
    uint                m_uiSamplingFrequency;          /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    uint                m_uiSamplesPerBlock;            /**< The samples per block defined by the user via the GUI.*/
    bool                m_bUseChExponent;               /**< Flag for using the channels exponent. Defined by the user via the GUI.*/
    bool                m_bUseUnitGain;                 /**< Flag for using the channels unit gain. Defined by the user via the GUI.*/
    bool                m_bUseUnitOffset;               /**< Flag for using the channels unit offset. Defined by the user via the GUI.*/
    bool                m_bWriteDriverDebugToFile;      /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    bool                m_bUsePreprocessing;            /**< Flag for using preprocessing actions for the EEG data. Defined by the user via the GUI.*/
    bool                m_bUseCommonAverage;            /**< Flag for using common average.*/
    bool                m_bMeasureImpedances;           /**< Flag for impedance measuring mode.*/

    //Handler
    HANDLE              m_HandleMaster;                 /**< The handler used to communciate with the device.*/
    HINSTANCE           m_oLibHandle;                   /**< The handler used to load the driver dll/lib.*/

    //Device info
    WCHAR               m_wcDeviceName[40];             /**< Contains the connected device name.*/
    ULONG               m_ulSerialNumber;               /**< Contains the connected device serial number.*/
    PSP_DEVICE_PATH     m_PSPDPMasterDevicePath;        /**< Contains the connected devicePath (used to get/open the device handler).*/
    ULONG               m_uiNumberOfAvailableChannels;  /**< Holds the available number of channels offered by the device.*/

    //Signal info
    QVector <LONG>      m_vExponentChannel;             /**< Contains the exponents for every channel available by the device.*/
    QVector <FLOAT>     m_vUnitGain;                    /**< Contains the unit gain for every channel available by the device.*/
    QVector <FLOAT>     m_vUnitOffSet;                  /**< Contains the unit offset for every channel available by the device.*/
    LONG*               m_lSignalBuffer;                /**< Buffer in which the device can write the samples -> these values get read out by the getSampleMatrix(...) function.*/
    LONG                m_lSignalBufferSize;            /**< Size of m_ulSignalBuffer = (samples per block) * (number of channels) * 4 (4 because every signal value takes 4 bytes - see TMSi SDK documentation).*/
    ofstream            m_outputFileStream;             /**< fstream for writing the driver debug informations to a txt file.*/
    QVector <double>    m_vSampleBlockBuffer;           /**< Buffer to store all the incoming smaples. This is the buffer which is getting read from.*/

    //Variables used for loading the TMSiSDK.dll methods. Note: Not all functions are used by this class at the moment.
    POPEN               m_oFpOpen;
    PCLOSE              m_oFpClose;
    PSTART              m_oFpStart;
    PSTOP               m_oFpStop;
    PGETSIGNALFORMAT    m_oFpGetSignalFormat;
    PSETSIGNALBUFFER    m_oFpSetSignalBuffer;
    PGETSAMPLES         m_oFpGetSamples;
    PGETBUFFERINFO      m_oFpGetBufferInfo;
    PFREE               m_oFpFree;
    PLIBRARYINIT        m_oFpLibraryInit;
    PLIBRARYEXIT        m_oFpLibraryExit;
    PGETDEVICELIST      m_oFpGetDeviceList;
    PGETFRONTENDINFO    m_oFpGetFrontEndInfo;
    PSETREFCALCULATION  m_oFpSetRefCalculation;
    PSETMEASURINGMODE   m_oFpSetMeasuringMode;
    PGETERRORCODE       m_oFpGetErrorCode;
};
} // NAMESPACE

#endif // TMSIDRIVER_H
