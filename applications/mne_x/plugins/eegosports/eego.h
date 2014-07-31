#pragma once

// To hinder min/max defines to clash with <limits>
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX

#include <wtypes.h>

#include <string>


namespace EEGoSportsPlugin
{


/////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// ENUMERATIONS
/////////////////////////////////////////////////////////////////////////

/** 
	* \brief List of special channel adresses
	* The indices of special channels are stored here.
	* Channel 0-63 which are the normal data channels are not explicit listed.
	*/
typedef enum
{
    EEGO_CHANNEL_VPWR33         = 64, //! = 3.3V power supply voltage (4.8uV/step)
    EEGO_CHANNEL_VPWR50         = 65, //! = 5.0V power supply voltage (4.8uV/step)
    EEGO_CHANNEL_VPWR55         = 66, //! = 5.5V power supply voltage (4.8uV/step)
    EEGO_CHANNEL_IPWO           = 67, //! = power-output current (??A/step)    	
    EEGO_CHANNEL_VREF           = 68, //! = reference voltage (76.8uV/step)
    EEGO_CHANNEL_VGND           = 69, //! = patient ground voltage (76.8uV/step)
    EEGO_CHANNEL_VDRV           = 70, //! = patient driver voltage (76.8uV/step)
    EEGO_CHANNEL_IDRV           = 71, //! = patient driver output current (??A/step) 
       	
    EEGO_CHANNEL_FRAME_INDEX    = 72, //! = frame index
	EEGO_CHANNEL_STATUS         = 73, //! = status (==0)
	EEGO_CHANNEL_CONTROL        = 74, //! = control settings (==0)
	EEGO_CHANNEL_PGND_DRIVER    = 75, //! = driver data value
	EEGO_CHANNEL_RTC_CTR        = 76, //! = real-time-clock counter (==32768)
	EEGO_CHANNEL_RDY_CTR        = 77, //! = A/D converter ready counter (==sampling rate)
    EEGO_CHANNEL_TRG			= 79, //! = triggers (TRG[7..0] = INCOMING, TRG[15..8] = OUTGOING, TRG[31..16] = Wireless)
} EEGO_CHANNEL;


//-----------------------------------------------------------------------
// EEGO_STATUS
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_STATUS_OK             = 0x00,
    EEGO_STATUS_BUSY           = 0x01,
    EEGO_STATUS_E_UNKNOWN      = 0x80,
    EEGO_STATUS_E_NOT_AVAIL    = 0xFF,
} EEGO_STATUS;


//-----------------------------------------------------------------------
// EEGO_COMMAND
//-----------------------------------------------------------------------
typedef enum
{
    // commands
    EEGO_COMMAND_RESET              = 0x01,

    // set values
    EEGO_COMMAND_SET_MODE           = 0x10,
    EEGO_COMMAND_SET_CONFIG         = 0x11,
    EEGO_COMMAND_SET_DRIVER_AMP     = 0x12,
    EEGO_COMMAND_SET_DRIVER_PERIOD  = 0x13,
    EEGO_COMMAND_SET_RATE           = 0x14,
    EEGO_COMMAND_SET_GAIN           = 0x15,
    EEGO_COMMAND_SET_MUX            = 0x16,
	// EEGO_COMMAND_SET_TRIGGER_CONFIG = 0x17, // as in 2011-04-01_EEGO64-Firmware-Lot4.4-release\IMO-USB2_Lot4.4\common.h
	// EEGO_COMMAND_SET_TRIGGER_PERIOD = 0x18, // as in 2011-04-01_EEGO64-Firmware-Lot4.4-release\IMO-USB2_Lot4.4\common.h

	// EEGO_COMMAND_GET_STATUS         = 0x80, // as in 2011-04-01_EEGO64-Firmware-Lot4.4-release\IMO-USB2_Lot4.4\common.h
	EEGO_COMMAND_GET_BATTERY_LEVEL	= 0x81,
	EEGO_COMMAND_GET_FIRMWARE_NUMBER = 0x82,
	EEGO_COMMAND_GET_BATTERY_CHARGING = 0x83,

    // get values
    EEGO_COMMAND_GET_MODE           = 0x90,
    EEGO_COMMAND_GET_CONFIG         = 0x91,
    EEGO_COMMAND_GET_DRIVER_AMP     = 0x92,
    EEGO_COMMAND_GET_DRIVER_PERIOD  = 0x93,
    EEGO_COMMAND_GET_RATE           = 0x94,
    EEGO_COMMAND_GET_GAIN           = 0x95,
    EEGO_COMMAND_GET_MUX            = 0x96
	// EEGO_COMMAND_GET_TRIGGER_CONFIG = 0x97, // as in 2011-04-01_EEGO64-Firmware-Lot4.4-release\IMO-USB2_Lot4.4\common.h
	// EEGO_COMMAND_GET_TRIGGER_PERIOD = 0x98, // as in 2011-04-01_EEGO64-Firmware-Lot4.4-release\IMO-USB2_Lot4.4\common.h

  // note to self: do not define command with value of 0x100
  // because the commands are represented as one byte of type
  // uint8_t. 0x100 overflows the unsigned byte and very nasty
  // nastiness happens.
} EEGO_COMMAND;


//-----------------------------------------------------------------------
// EEGO_CONFIG
//-----------------------------------------------------------------------
typedef union
{
    USHORT VALUE;
    struct {     
    USHORT    bEnableDAC  : 1;
    USHORT    bEnablePWO  : 1;
    USHORT    bUnlockOCP  : 1;
    USHORT    bRawValues  : 1;
    USHORT    uDebugEEGO  : 2;
    USHORT    uDebugDACQ  : 2;
    USHORT    reserved    : 8;
    } BITS;
} EEGO_CONFIG;


//-----------------------------------------------------------------------
// EEGO_MODE
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_MODE_IDLE            =  1,
    EEGO_MODE_STREAMING       =  2,
    EEGO_MODE_CALIBRATION     =  3,
    EEGO_MODE_IMPEDANCE_CHA   =  4,
    EEGO_MODE_IMPEDANCE_REF   =  5,
} EEGO_MODE;


//-----------------------------------------------------------------------
// EEGO_RATE
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_RATE_500HZ    =   500,
    EEGO_RATE_512HZ    =   512,
    EEGO_RATE_1000HZ   =  1000,
    EEGO_RATE_1024HZ   =  1024,
    EEGO_RATE_2000HZ   =  2000,
    EEGO_RATE_2048HZ   =  2048,
    EEGO_RATE_4000HZ   =  4000,
    EEGO_RATE_4096HZ   =  4096,
    EEGO_RATE_8000HZ   =  8000,
    EEGO_RATE_8192HZ   =  8192,
    EEGO_RATE_16000HZ  = 16000,
    EEGO_RATE_16384HZ  = 16384
} EEGO_RATE;


//-----------------------------------------------------------------------
// EEGO_GAIN
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_GAIN_1X      =     1,
    EEGO_GAIN_2X      =     2,
    EEGO_GAIN_3X      =     3,
    EEGO_GAIN_4X      =     4,
    EEGO_GAIN_6X      =     6,
    EEGO_GAIN_8X      =     8,
    EEGO_GAIN_12X     =    12,
} EEGO_GAIN;


//-----------------------------------------------------------------------
// EEGO_MUX
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_MUX_NORMAL    = 1,
    EEGO_MUX_SHORTED   = 2,
    EEGO_MUX_RLD_CONJ  = 3,
    EEGO_MUX_MVDD      = 4,
    EEGO_MUX_TEMP      = 5,
    EEGO_MUX_TEST      = 6,
    EEGO_MUX_RLD_DRP   = 7,
    EEGO_MUX_RLD_DRN   = 8,
} EEGO_MUX;


//-----------------------------------------------------------------------
// EEGO_ADC
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_ADC_A                 = 0x00,
    EEGO_ADC_B                 = 0x01,
    EEGO_ADC_C                 = 0x02,
    EEGO_ADC_D                 = 0x03,
    EEGO_ADC_E                 = 0x04,
    EEGO_ADC_F                 = 0x05,
    EEGO_ADC_G                 = 0x06,
    EEGO_ADC_H                 = 0x07,
    EEGO_ADC_S                 = 0x08,
} EEGO_ADC;


//-----------------------------------------------------------------------
// EEGO_DEBUG_EEGO
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_DEBUG_EEGO_NONE     = 0,
    EEGO_DEBUG_EEGO_STREAMER = 1,
    EEGO_DEBUG_EEGO_RESERVED = 2,
    EEGO_DEBUG_EEGO_TOGGLE   = 3,
} EEGO_DEBUG_EEGO;


//-----------------------------------------------------------------------
// EEGO_DEBUG_DACQ
//-----------------------------------------------------------------------
typedef enum
{
    EEGO_DEBUG_DACQ_NONE     = 0,
    EEGO_DEBUG_DACQ_INDICES  = 1,
    EEGO_DEBUG_DACQ_SAMPLES  = 2,
    EEGO_DEBUG_DACQ_TOGGLE   = 3,
} EEGO_DEBUG_DACQ;


//-----------------------------------------------------------------------
// EEGO_REQUEST
//-----------------------------------------------------------------------
typedef SHORT  EEGO_REQVAL;
typedef BYTE   EEGO_REQRESV;
typedef BYTE   EEGO_REQLEN;
typedef BYTE   EEGO_REQDATA;

typedef struct
{
    EEGO_COMMAND   eCommand;    // 1 byte
    EEGO_REQRESV   uRerserved;  // 1 byte
    EEGO_REQVAL    uValue;      // 2 bytes
    EEGO_ADC       eAdc;        // 1 byte
    EEGO_REQLEN    uLength;     // 1 bytes
    EEGO_REQDATA   uData[48];
} EEGO_REQUEST;


/////////////////////////////////////////////////////////////////////////
// STRUCTURES
/////////////////////////////////////////////////////////////////////////
struct EEGO_FEATURE_REPORT
{
    BYTE buffer[6];
};

 
/////////////////////////////////////////////////////////////////////////////
// 
// Forward declarations
//
/////////////////////////////////////////////////////////////////////////////
class IAmplifier;
class IBuffer;


/////////////////////////////////////////////////////////////////////////////
// 
// class IAmplifier
//
/////////////////////////////////////////////////////////////////////////////

/**
* @name IAmplifier
*
*/
class IAmplifier
{
public:
    virtual ULONG AddRef( )=NULL;
    virtual ULONG Release( )=NULL;

public:
    //-------------------------------------------------------------------
    // PUBLISHED METHODS
    //-------------------------------------------------------------------

    /**
    * @name EnumDevices
    */
    virtual HRESULT EnumDevices( UINT* puiDeviceCount )=NULL;
    virtual HRESULT EnumDevices( UINT uiDeviceIndex, LPTSTR* pszDeviceName )=NULL;

    /**
    * @name Connect
    */
    virtual HRESULT Connect( UINT uiDeviceIndex=0 )=NULL;
    virtual HRESULT Connect( LPCTSTR szSerial )=NULL;

    /**
    * @name Disconnect
    */
    virtual HRESULT Disconnect( )=NULL;

    /**
    * @name Reset
    */
    virtual HRESULT Reset( )=NULL;

    /**
    * @name GetData
    */
    virtual HRESULT GetData( IBuffer** ppBuffer )=NULL;

    //-------------------------------------------------------------------
    // PUBLISHED PROPERTIES
    //-------------------------------------------------------------------

    /**
    * @name Connected
    */
    virtual BOOL Connected( )=NULL;

    /**
    * @name Get/Set Mode
    */
    virtual HRESULT GetMode( EEGO_MODE* peMode )=NULL;
    virtual HRESULT SetMode( EEGO_MODE eMode )=NULL;

    /**
    * @name Get/Set Config
    */
    virtual HRESULT GetConfig( EEGO_CONFIG* psConfig )=NULL;
    virtual HRESULT SetConfig( EEGO_CONFIG sConfig )=NULL;

    /**
    * @name Get/Set DriverAmplitude
	* @param nAmplitude In steps of 62.5uV, so 160 is 160x62.5uV = 10mV
    */
    virtual HRESULT GetDriverAmplitude( SHORT* pnAmplitude )=NULL;
    virtual HRESULT SetDriverAmplitude( SHORT nAmplitude )=NULL;

    /**
    * @name Get/Set DriverPeriod
    */
    virtual HRESULT GetDriverPeriod( USHORT* puPeriod )=NULL;
    virtual HRESULT SetDriverPeriod( USHORT uPeriod )=NULL;

    /**
    * @name Get/Set SamplingRate
    */
    virtual HRESULT GetSamplingRate( EEGO_RATE* peSamplingRate )=NULL;
    virtual HRESULT SetSamplingRate( EEGO_RATE eSamplingRate )=NULL;

    /**
    * @name Get/Set SignalGain
    */
    virtual HRESULT GetSignalGain( EEGO_GAIN* peSignalGain, EEGO_ADC eAdc )=NULL;
    virtual HRESULT SetSignalGain( EEGO_GAIN eSignalGain, EEGO_ADC eAdc )=NULL;

    /**
    * @name Get/Set SignalMux
    */
    virtual HRESULT GetSignalMux( EEGO_MUX* peSignalMux, EEGO_ADC eAdc )=NULL;
    virtual HRESULT SetSignalMux( EEGO_MUX eSignalMux, EEGO_ADC eAdc )=NULL;

    /**
    * @name Get BatteryLevel
    */
	virtual HRESULT GetBatteryLevel( USHORT* puLevel )=NULL;
	
    /**
    * @name Get FirmwareVersion
    */
	virtual HRESULT GetFirmwareVersion( USHORT* puVersion )=NULL;
	
    /**
    * @name Get BatteryCharging
    */
	virtual HRESULT GetBatteryCharging( USHORT* puVersion )=NULL;

	virtual void DumpData(const std::string& fileName, bool bDump)=NULL;
};



/////////////////////////////////////////////////////////////////////////////
// 
// class IBuffer
//
/////////////////////////////////////////////////////////////////////////////

/**
* @name IBuffer
*/
class IBuffer
{
public:
	enum ErrorStates : int
	{
		NoError = 0,
		BadPacketStatus = 1,
		InternalError = 2,
		UnknownPacket = 4,
		UnknownReport = 8,
		ErrorBits = 16,
		SkippedReport = 32
	};

    virtual ULONG AddRef( )=NULL;
    virtual ULONG Release( )=NULL;

public:
    //-------------------------------------------------------------------
    // PUBLISHED PROPERTIES
    //-------------------------------------------------------------------

    /**
    *
    */
    virtual LONG* GetBuffer( )=NULL;
    virtual LONG  GetBuffer( UINT i )=NULL;
    virtual LONG  GetBuffer( UINT ci, UINT si )=NULL;

    /**
    *
    */
    virtual UINT GetChannelCount( )=NULL;

    /**
    *
    */
    virtual UINT GetSampleCount( )=NULL;

	virtual int GetErrors() const = NULL;
	virtual void ClearErrors() = NULL;
};

/////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
/////////////////////////////////////////////////////////////////////////
HRESULT CreateAmplifierObject( IAmplifier** ppAmplifier );


} // namespaces
