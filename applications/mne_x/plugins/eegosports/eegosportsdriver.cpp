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
* @brief    Contains the implementation of the EEGoSportsDriver class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsdriver.h"
#include "eegosportsproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGoSportsPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsDriver::EEGoSportsDriver(EEGoSportsProducer* pEEGoSportsProducer)
: m_pEEGoSportsProducer(pEEGoSportsProducer)
, m_bDllLoaded(true)
, m_bInitDeviceSuccess(false)
, m_uiNumberOfChannels(64)
, m_uiSamplingFrequency(1024)
, m_uiSamplesPerBlock(16)
, m_bUseChExponent(false)
, m_bUseUnitGain(false)
, m_bUseUnitOffset(false)
, m_bWriteDriverDebugToFile(false)
, m_sOutputFilePath("/mne_x_plugins/resources/eegosports")
, m_bUseCommonAverage(false)
, m_bMeasureImpedances(false)
{
    //Initialise NULL pointers
    m_oLibHandle = NULL ;
    m_lSignalBuffer = NULL;

    //Check which driver dll to take: TMSiSDK.dll oder TMSiSDK32bit.dll
    #ifdef TAKE_EEGOSPORTSSDK_DLL //32 bit system & 64 bit (with 64 bit compiler)
            m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\System32\\eego.dll");
    #elif TAKE_EEGOSPORTSSDK_32_DLL //64 bit (with 32 bit compiler)
            m_oLibHandle = ::LoadLibrary(L"C:\\Windows\\SysWOW64\\eego.dll");
    #endif

    //If dll can't be open return
    if( m_oLibHandle == NULL)
    {
        cout << "Plugin EEGoSports - ERROR - Couldn't load DLL - Check if the driver for the TMSi USB Fiber Connector installed in the system dir" << endl;
        m_bDllLoaded = false;
        return;
    }

    //Load DLL methods for initializing the driver
    __load_dll_func__(m_oFpCreateAmplifier, CREATEAMPLIFIER, "CreateAmplifier");

    cout << "Plugin EEGoSports - INFO - EEGoSportsDriver() - Successfully loaded all DLL functions" << endl;
}


//*************************************************************************************************************

EEGoSportsDriver::~EEGoSportsDriver()
{
    //cout << "EEGoSportsDriver::~EEGoSportsDriver()" << endl;
}


//*************************************************************************************************************

bool EEGoSportsDriver::initDevice(int iNumberOfChannels,
                            int iSamplingFrequency,
                            int iSamplesPerBlock,
                            bool bUseChExponent,
                            bool bUseUnitGain,
                            bool bUseUnitOffset,
                            bool bWriteDriverDebugToFile,
                            QString sOutpuFilePath,
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
    m_sOutputFilePath = sOutpuFilePath;
    m_bUseCommonAverage = bUseCommonAverage;
    m_bMeasureImpedances = bMeasureImpedance;

    //Open file to write to
    if(m_bWriteDriverDebugToFile)
        m_outputFileStream.open("mne_x_plugins/resources/eegosports/EEGoSports_Driver_Debug.txt", ios::trunc); //ios::trunc deletes old file data

    // Get device handler
    HRESULT hres = (*m_oFpCreateAmplifier)(&m_pAmplifier);
    if(FAILED(hres) || !m_pAmplifier)
    {
        cout << "Plugin EEGoSports - ERROR - Couldn't create amplifier! HRESULT: " << hres << endl;
        return false;
    }

    // Initialise device
    HRESULT hr;
    LPTSTR szDevString = NULL;

    UINT numDevices;
    if(FAILED(m_pAmplifier->EnumDevices(&numDevices)))
    {
        cout << "Plugin EEGoSports - ERROR - Couldn't get device numbers!" << endl;
        return false;
    }

    for(UINT i = 0; i < numDevices; i++)
    {
        LPTSTR szName;
        if(FAILED(m_pAmplifier->EnumDevices(i, &szName)))
        {
            cout << "Plugin EEGoSports - ERROR - Couldn't get device name!" << endl;
            return false;
        }

        cout<<"Found device: "<<szName<<endl;

        // Make sure to get no simulation device. This should be no problem with later versions
        QString temp;
        temp = temp.fromWCharArray(szName);
        if(!temp.contains("SIM"))
        {
            szDevString = szName;
            break;
        }

        delete szName;
    }

    if( !szDevString )
    {
        cout << "Plugin EEGoSports - ERROR - No connected device found!" << endl;
        return false;
    }

    // Make the USB handshake and setup internal states for communication with the hardware
    if( FAILED(m_pAmplifier->Connect(szDevString)))
    {
        cout << "Plugin EEGoSports - ERROR - Connect call failed!" << endl;
        return false;
    }

    // Better safe than sorry
    Sleep(100);

    // Set it to a defined state
    m_pAmplifier->Reset();

    // Better safe than sorry
    Sleep(100);

    // reset the overcurrent protection
    EEGO_CONFIG conf;
    memset(&conf,0,sizeof(EEGO_CONFIG));
    conf.BITS.bUnlockOCP = 1;
    m_pAmplifier->SetConfig(conf);

    // This takes a while. Better wait
    Sleep(100);

    // Aanything over 2kHZ is not supported and chances are high that they just don't work.
    if(FAILED(m_pAmplifier->SetSamplingRate((EEGO_RATE)m_uiSamplingFrequency)))
    {
        cout << "Plugin EEGoSports - ERROR - Can not set sampling frequency: " << m_uiSamplingFrequency << endl;
        return false;
    }

    // use this with mV or set gain directly. Again, look into eego.h for acceptable values
    //EEGO_GAIN gain = GetGainForSignalRange(1000);

    // It is possible to set those for each individually. Again: not tested, not supported
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_A);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_B);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_C);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_D);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_E);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_F);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_G);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_H);
//    m_pAmplifier->SetSignalGain( gain, EegoDriver::EEGO_ADC_S);

    Sleep(100);

    // We are measuring here so better leave the DAC off
    hr = m_pAmplifier->SetDriverAmplitude(0);
    hr |= m_pAmplifier->SetDriverPeriod(0);

    if(FAILED(hr))
        return false;

    // This takes a while. Better wait
    Sleep(100);

    USHORT firmwareVersion;
    m_pAmplifier->GetFirmwareVersion(&firmwareVersion);

    cout<<"Firmware version is: "<<firmwareVersion<<endl;

    // Start the sampling
    if(!m_pAmplifier)
        return false;

    EEGO_RATE rate;
    EEGO_GAIN gain;

    // You can get the set values from the device, too. We are using it here only for debug purposes
    m_pAmplifier->GetSamplingRate(&rate);
    m_pAmplifier->GetSignalGain(&gain, EEGO_ADC_A);

    cout << "Starting Device with sampling rate: " << m_uiSamplingFrequency << "hz and a gain of: " << gain << "\n";

    // With this call we tell the amplifier and driver stack to start streaming
    if(FAILED(m_pAmplifier->SetMode(EEGO_MODE_STREAMING)))
        return false;

    Sleep(100);

    cout << "Plugin EEGoSports - INFO - initDevice() - Successfully initialised the device" << endl;

    m_bInitDeviceSuccess = true;

    return true;
}


//*************************************************************************************************************

bool EEGoSportsDriver::uninitDevice()
{
    //Clear the buffer which is used to store the received samples
    m_vSampleBlockBuffer.clear();

    //Check if the device was initialised
    if(!m_bInitDeviceSuccess)
    {
        cout << "Plugin EEGoSports - ERROR - uninitDevice() - Device was not initialised - therefore can not be uninitialised" << endl;
        return false;
    }

    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
    {
        cout << "Plugin EEGoSports - ERROR - uninitDevice() - Driver DLL was not loaded" << endl;
        return false;
    }

    //Close the output stream/file
    if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
    {
        m_outputFileStream.close();
        m_outputFileStream.clear();
    }

    if(!m_pAmplifier)
        return false;

    if(FAILED(m_pAmplifier->SetMode(EEGO_MODE_IDLE)))
        return false;

    m_pAmplifier->Disconnect();
    m_pAmplifier->Release();

    //Reset to NULL pointers
    m_pAmplifier = NULL;
    m_lSignalBuffer = NULL;

    cout << "Plugin EEGoSports - INFO - uninitDevice() - Successfully uninitialised the device" << endl;
    return true;
}


//*************************************************************************************************************

 bool EEGoSportsDriver::getSampleMatrixValue(MatrixXf& sampleMatrix)
{
    //Check if the driver DLL was loaded
    if(!m_bDllLoaded)
        return false;

    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        cout << "Plugin EEGoSports - ERROR - getSampleMatrixValue() - Cannot start to get samples from device because device was not initialised correctly" << endl;
        return false;
    }

    sampleMatrix = MatrixXf::Zero(m_uiNumberOfChannels, m_uiSamplesPerBlock);

//    sampleMatrix.setZero(); // Clear matrix - set all elements to zero
//    uint iSamplesWrittenToMatrix = 0;
//    int channelMax = 0;
//    int sampleMax = 0;
//    int sampleIterator = 0;

//    //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
//    while(iSamplesWrittenToMatrix < m_uiSamplesPerBlock)
//    {
//        //Get sample block from device
//        LONG ulSizeSamples = m_oFpGetSamples(m_HandleMaster, (PULONG)m_lSignalBuffer, m_lSignalBufferSize);
//        LONG ulNumSamplesReceived = ulSizeSamples/(m_uiNumberOfAvailableChannels*4);

//        //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
//        if(ulNumSamplesReceived > 0)
//        {
//            int actualSamplesWritten = 0; //Holds the number of samples which are actually written to the matrix in this while procedure

//            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. These are then written to the next matrix (block)
//            for(int i=0; i<ulNumSamplesReceived; i++)
//            {
//                for(uint j=i*m_uiNumberOfAvailableChannels; j<(i*m_uiNumberOfAvailableChannels)+m_uiNumberOfChannels; j++)
//                    m_vSampleBlockBuffer.push_back((double)m_lSignalBuffer[j]);
//            }

//            //If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
//            if(m_uiNumberOfAvailableChannels < m_uiNumberOfChannels)
//                channelMax = m_uiNumberOfAvailableChannels;
//            else
//                channelMax = m_uiNumberOfChannels;

//            //If the number of the samples which were already written to the matrix plus the last received number of samples is larger then the defined block size
//            //-> only fill until the matrix is completeley filled with samples. The other (unused) samples are still stored in the vector buffer m_vSampleBlockBuffer and will be used in the next matrix which is to be sent to the circular buffer
//            if(iSamplesWrittenToMatrix + ulNumSamplesReceived > m_uiSamplesPerBlock)
//                sampleMax = m_uiSamplesPerBlock - iSamplesWrittenToMatrix + sampleIterator;
//            else
//                sampleMax = ulNumSamplesReceived + sampleIterator;

//            //Read the needed number of samples from the vector buffer to store them in the matrix
//            for(; sampleIterator < sampleMax; sampleIterator++)
//            {
//                for(int channelIterator = 0; channelIterator < channelMax; channelIterator++)
//                {
//                    sampleMatrix(channelIterator, sampleIterator) = ((m_vSampleBlockBuffer.first() * (m_bUseUnitGain ? m_vUnitGain[channelIterator] : 1)) + (m_bUseUnitOffset ? m_vUnitOffSet[channelIterator] : 0)) * (m_bUseChExponent ? pow(10., (double)m_vExponentChannel[channelIterator]) : 1);
//                    m_vSampleBlockBuffer.pop_front();
//                }

//                actualSamplesWritten ++;
//            }

//            iSamplesWrittenToMatrix = iSamplesWrittenToMatrix + actualSamplesWritten;
//        }

////        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
////        {
////            m_outputFileStream << "ulSizeSamples: " << ulSizeSamples << endl;
////            m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;
////            m_outputFileStream << "sampleMax: " << sampleMax << endl;
////            m_outputFileStream << "sampleIterator: " << sampleIterator << endl;
////            m_outputFileStream << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << endl << endl;
////        }
//    }

//    if(/*m_outputFileStream.is_open() &&*/ m_bWriteDriverDebugToFile)
//    {
//        //Get device buffer info
//        ULONG ulOverflow;
//        ULONG ulPercentFull;
//        m_oFpGetBufferInfo(m_HandleMaster, &ulOverflow, &ulPercentFull);

//        m_outputFileStream <<  "Unit offset: " << endl;
//        for(int w = 0; w<<m_vUnitOffSet.size(); w++)
//            cout << float(m_vUnitOffSet[w]) << "  ";
//        m_outputFileStream << endl << endl;

//        m_outputFileStream <<  "Unit gain: " << endl;
//        for(int w = 0; w<<m_vUnitGain.size(); w++)
//            m_outputFileStream << float(m_vUnitGain[w]) << "  ";
//        m_outputFileStream << endl << endl;

//        m_outputFileStream << "----------<See output file for sample matrix>----------" <<endl<<endl;
//        m_outputFileStream << "----------<Internal driver buffer is "<<ulPercentFull<<" full>----------"<<endl;
//        m_outputFileStream << "----------<Internal driver overflow is "<<ulOverflow<< ">----------"<<endl;
//    }

    return true;
}

