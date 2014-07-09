//=============================================================================================================
/**
* @file     eegosportsdriver.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
* @brief    Contains the declaration of the EEGoSportsDriver class. This class implements the basic communication between MNE-X and a ANT EEGoSports device
*
*/

#ifndef EEGOSPORTSDRIVER_H
#define EEGOSPORTSDRIVER_H


//*************************************************************************************************************
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

#include <eego.h>


//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <qapplication.h>
#include <QVector>
#include <QSysInfo>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace EEGoSportsPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// Method Typedefs - method defines as used in the RTINST.DLL
//=============================================================================================================

// define a pointer CREATEAMPLIFIER to a function which is taking a void HANDLE and returns a HRESULT value
typedef HRESULT         ( __stdcall * CREATEAMPLIFIER)            (IAmplifier** ppObject);


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

class EEGoSportsProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define MAX_BUFFER_SIZE 0xFFFFFFFF

//DLL Function loader define
#define __load_dll_func__(var, type, name) \
    var = (type)::GetProcAddress(m_oLibHandle, name); \
    if(!var) \
        cout<< "Plugin EEGoSports - ERROR - Error loading method " << name << "\n"; \


//=============================================================================================================
/**
* EEGoSportsDriver
*
* @brief The EEGoSportsDriver class provides real time data acquisition of EEG data with a TMSi Refa device.
*/
class EEGoSportsDriver
{
public:
    //=========================================================================================================
    /**
    * Constructs a EEGoSportsDriver.
    *
    * @param [in] pEEGoSportsProducer a pointer to the corresponding EEGoSports Producer class.
    */
    EEGoSportsDriver(EEGoSportsProducer* pEEGoSportsProducer);

    //=========================================================================================================
    /**
    * Destroys the EEGoSportsDriver.
    */
    ~EEGoSportsDriver();

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
    * @param [in] bUseChExponent Flag for using the channels exponent. Defined by the user via the GUI.
    * @param [in] bUseUnitGain Flag for using the channels unit gain. Defined by the user via the GUI.
    * @param [in] bUseUnitOffset Flag for using the channels unit offset. Defined by the user via the GUI.
    * @param [in] bWriteDriverDebugToFile Flag for writing driver debug information to a file. Defined by the user via the GUI.
    * @param [in] sOutpuFilePath Holds the path for the output file. Defined by the user via the GUI.
    * @param [out] bool returns true if device was successfully initialised, false otherwise.
    * @param [in] bUseCommonAverage Flag for using common average when recording EEG data. Defined by the user via the GUI.
    * @param [in] bMeasureImpedance Flag for measuring impedances.
    */
    bool initDevice(int iNumberOfChannels,
                    int iSamplingFrequency,
                    int iSamplesPerBlock,
                    bool bUseChExponent,
                    bool bUseUnitGain,
                    bool bUseUnitOffset,
                    bool bWriteDriverDebugToFile,
                    QString sOutpuFilePath,
                    bool bUseCommonAverage,
                    bool bMeasureImpedance);

    //=========================================================================================================
    /**
    * Uninitialise device.
    * @param [out] bool returns true if device was successfully uninitialised, false otherwise.
    */
    bool uninitDevice();

private:
    EEGoSportsProducer*       m_pEEGoSportsProducer;                /**< A pointer to the corresponding EEGoSportsProducer class.*/

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
    QString             m_sOutputFilePath;              /**< Holds the path for the output file. Defined by the user via the GUI.*/
    bool                m_bUseCommonAverage;            /**< Flag for using common average.*/
    bool                m_bMeasureImpedances;           /**< Flag for impedance measuring mode.*/

    //Handler
    HANDLE              m_HandleMaster;                 /**< The handler used to communciate with the device.*/
    HINSTANCE           m_oLibHandle;                   /**< The handler used to load the driver dll/lib.*/

    //Device info
    WCHAR               m_wcDeviceName[40];             /**< Contains the connected device name.*/
    ULONG               m_ulSerialNumber;               /**< Contains the connected device serial number.*/
    //PSP_DEVICE_PATH     m_PSPDPMasterDevicePath;        /**< Contains the connected devicePath (used to get/open the device handler).*/
    ULONG               m_uiNumberOfAvailableChannels;  /**< Holds the available number of channels offered by the device.*/

    //Signal info
    QVector <LONG>      m_vExponentChannel;             /**< Contains the exponents for every channel available by the device.*/
    QVector <FLOAT>     m_vUnitGain;                    /**< Contains the unit gain for every channel available by the device.*/
    QVector <FLOAT>     m_vUnitOffSet;                  /**< Contains the unit offset for every channel available by the device.*/
    LONG*               m_lSignalBuffer;                /**< Buffer in which the device can write the samples -> these values get read out by the getSampleMatrix(...) function.*/
    LONG                m_lSignalBufferSize;            /**< Size of m_ulSignalBuffer = (samples per block) * (number of channels) * 4 (4 because every signal value takes 4 bytes - see TMSi SDK documentation).*/
    ofstream            m_outputFileStream;             /**< fstream for writing the driver debug informations to a txt file.*/
    QVector <double>    m_vSampleBlockBuffer;           /**< Buffer to store all the incoming smaples. This is the buffer which is getting read from.*/

    IAmplifier*         m_pAmplifier;                   /**< Interface to the driver.*/

    //Variables used for loading the TMSiSDK.dll methods. Note: Not all functions are used by this class at the moment.
    CREATEAMPLIFIER       m_oFpCreateAmplifier;
//    POPEN               m_oFpOpen;
//    PCLOSE              m_oFpClose;
//    PSTART              m_oFpStart;
//    PSTOP               m_oFpStop;
//    PGETSIGNALFORMAT    m_oFpGetSignalFormat;
//    PSETSIGNALBUFFER    m_oFpSetSignalBuffer;
//    PGETSAMPLES         m_oFpGetSamples;
//    PGETBUFFERINFO      m_oFpGetBufferInfo;
//    PFREE               m_oFpFree;
//    PLIBRARYINIT        m_oFpLibraryInit;
//    PLIBRARYEXIT        m_oFpLibraryExit;
//    PGETDEVICELIST      m_oFpGetDeviceList;
//    PGETFRONTENDINFO    m_oFpGetFrontEndInfo;
//    PSETREFCALCULATION  m_oFpSetRefCalculation;
//    PSETMEASURINGMODE   m_oFpSetMeasuringMode;
//    PGETERRORCODE       m_oFpGetErrorCode;
};

} // NAMESPACE

#endif // EEGOSPORTSDRIVER_H
