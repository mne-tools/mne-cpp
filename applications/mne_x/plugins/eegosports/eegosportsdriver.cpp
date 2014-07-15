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
, m_bWriteDriverDebugToFile(false)
, m_sOutputFilePath("/mne_x_plugins/resources/eegosports")
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
                            bool bWriteDriverDebugToFile,
                            QString sOutpuFilePath,
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
    m_bWriteDriverDebugToFile = bWriteDriverDebugToFile;
    m_sOutputFilePath = sOutpuFilePath;
    m_bMeasureImpedances = bMeasureImpedance;

    //Open debug file to write to
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

    // Get number and names of connected devices
    UINT numDevices;
    if(FAILED(m_pAmplifier->EnumDevices(&numDevices)))
    {
        cout << "Plugin EEGoSports - ERROR - Couldn't get device numbers!" << endl;
        return false;
    }

    // Get name of first connected device in the device list which is not a simulated device
    for(UINT i = 0; i < numDevices; i++)
    {
        LPTSTR szName;
        if(FAILED(m_pAmplifier->EnumDevices(i, &szName)))
        {
            cout << "Plugin EEGoSports - ERROR - Couldn't get device name!" << endl;
            return false;
        }

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

    //cout<<"Found device: "<<szDevString<<endl;

    // Connect device - Make the USB handshake and setup internal states for communication with the hardware
    if( FAILED(m_pAmplifier->Connect(szDevString)))
    {
        cout << "Plugin EEGoSports - ERROR - Connect call failed!" << endl;
        return false;
    }

    // This takes a while. Better wait
    //Sleep(100);

    // Reset device - Set it to a defined state
    m_pAmplifier->Reset();

    // This takes a while. Better wait
    //Sleep(100);

    // reset the overcurrent protection
    EEGO_CONFIG conf;
    memset(&conf,0,sizeof(EEGO_CONFIG));
    conf.BITS.bUnlockOCP = 1;
    m_pAmplifier->SetConfig(conf);

    // This takes a while. Better wait
    //Sleep(100);

    // Set sampling frequency - Anything over 2kHZ is not supported and chances are high that they just don't work.
    if(FAILED(m_pAmplifier->SetSamplingRate((EEGO_RATE)m_uiSamplingFrequency)))
    {
        cout << "Plugin EEGoSports - ERROR - Can not set sampling frequency: " << m_uiSamplingFrequency << endl;
        return false;
    }

    // Set gain - use this with mV or set gain directly. Again, look into eego.h for acceptable values
    EEGO_GAIN gain = GetGainForSignalRange(1000);

    // It is possible to set those for each individually. Again: not tested, not supported
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_A);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_B);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_C);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_D);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_E);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_F);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_G);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_H);
    m_pAmplifier->SetSignalGain(gain, EEGO_ADC_S);

    // We are measuring here so better leave the DAC off
    hr = m_pAmplifier->SetDriverAmplitude(160);
    hr |= m_pAmplifier->SetDriverPeriod(500);

    if(FAILED(hr))
        return false;

    // This takes a while. Better wait
    //Sleep(100);

    // Get firmware version
    USHORT firmwareVersion;
    m_pAmplifier->GetFirmwareVersion(&firmwareVersion);

    //cout<<"Firmware version is: "<<firmwareVersion<<endl;

    // Start the sampling process
    if(!m_pAmplifier)
        return false;

    // You can get the set values from the device, too. We are using it here only for debug purposes
    EEGO_RATE rate;

    m_pAmplifier->GetSamplingRate(&rate);
    m_pAmplifier->GetSignalGain(&gain, EEGO_ADC_A);

    // cout << "Starting Device with sampling rate: " << m_uiSamplingFrequency << "hz and a gain of: " << gain << "\n";

    // With this call we tell the amplifier and driver stack to start streaming
    if(FAILED(m_pAmplifier->SetMode(EEGO_MODE_CALIBRATION))) //EEGO_MODE_CALIBRATION EEGO_MODE_STREAMING
        return false;

    //Sleep(100);

    cout << "Plugin EEGoSports - INFO - initDevice() - Successfully initialised the device" << endl;

    // Set flag for successfull initialisation true
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

    // If global device handle is not defined return false
    if(!m_pAmplifier)
        return false;

    // Stop device sampling - Set device to idle mode
    if(FAILED(m_pAmplifier->SetMode(EEGO_MODE_IDLE)))
        return false;

    // Disconnect and release handle
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

    if(!m_pAmplifier)
        return false;

    // Init stuff
    sampleMatrix.setZero(); // Clear matrix - set all elements to zero
    uint iSamplesWrittenToMatrix = 0;
    int channelMax = 0;
    int sampleMax = 0;
    int sampleIterator = 0;

    //get samples from device until the complete matrix is filled, i.e. the samples per block size is met
    while(iSamplesWrittenToMatrix < m_uiSamplesPerBlock)
    {
        // Fetch data from device/driver like this:
        IBuffer* pBuffer; // The data storage

        if(FAILED(m_pAmplifier->GetData(&pBuffer))) // Fill the storage with data. Data is delivered only once
        {
            cout << "Plugin EEGoSports - ERROR - Getting Data from device failed! " << endl;
            return false;
        }

        //Get sample and channel infos from device
        m_uiNumberOfAvailableChannels = pBuffer->GetChannelCount();
        int ulNumSamplesReceived = pBuffer->GetSampleCount();

        //Only do the next steps if there was at least one sample received, otherwise skip and wait until at least one sample was received
        if(ulNumSamplesReceived > 0)
        {
            //cout<<"Sample received"<<endl;
            int actualSamplesWritten = 0; //Holds the number of samples which are actually written to the matrix in this while procedure

            //If the number of available channels is smaller than the number defined by the user -> set the channelMax to the smaller number
            if(m_uiNumberOfAvailableChannels < m_uiNumberOfChannels)
                channelMax = m_uiNumberOfAvailableChannels;
            else
                channelMax = m_uiNumberOfChannels;

            //Write the received samples to an extra buffer, so that they are not getting lost if too many samples were received. These are then written to the next matrix (block)
            for(int sample = 0; sample < ulNumSamplesReceived; sample++)
                for(int channel = 0; channel < channelMax; channel++)
                    m_vSampleBlockBuffer.push_back(m_bUseChExponent ? pBuffer->GetBuffer(channel, sample) * 1e-6 : pBuffer->GetBuffer(channel, sample));

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
                    sampleMatrix(channelIterator, sampleIterator) = m_vSampleBlockBuffer.first();
                    m_vSampleBlockBuffer.pop_front();
                }

                actualSamplesWritten ++;
            }

            iSamplesWrittenToMatrix = iSamplesWrittenToMatrix + actualSamplesWritten;
        }

        // Memory cleanup
        pBuffer->Release();
        pBuffer = NULL;

        if(m_outputFileStream.is_open() && m_bWriteDriverDebugToFile)
        {
            m_outputFileStream << "ulNumSamplesReceived: " << ulNumSamplesReceived << endl;
            m_outputFileStream << "sampleMax: " << sampleMax << endl;
            m_outputFileStream << "sampleIterator: " << sampleIterator << endl;
            m_outputFileStream << "iSamplesWrittenToMatrix: " << iSamplesWrittenToMatrix << endl << endl;
        }
    }

    return true;

    //    //*************************************************************************************************************
    //    // Open Vibe solution
    //    sampleMatrix = MatrixXf::Zero(m_uiNumberOfChannels, m_uiSamplesPerBlock);

    //    // OpenVibe code
    //    HRESULT hr;

    //    if(!m_pAmplifier)
    //        return false;

    //    // Fetch data from device/driver like this:
    //    IBuffer* pBuffer; // The data storage
    //    if(FAILED(hr = m_pAmplifier->GetData(&pBuffer))) // Fill the storage with data. Data is delivered only once
    //    {
    //        cout << "Plugin EEGoSports - ERROR - Getting Data from device failed! HRESULT: " << hr << endl;
    //        return false;
    //    }

    //    // copy data from IBuffer to whatever structure you like to have.
    //    UINT nAmountOfSamples = pBuffer->GetSampleCount();
    //    //cout << "nAmountOfSamples: " << nAmountOfSamples << endl;
    //    UINT nAmountOfChannels = pBuffer->GetChannelCount();
    //    //cout << "nAmountOfChannels: " << nAmountOfChannels << endl;

    //    // The data is stored in µV, use this constant for conversion
    //    double dLSBToSi = 1e-6;

    //    // calculate start of unwritten data;
    //    for(UINT sample = 0; sample < nAmountOfSamples; sample++)
    //    {
    //        for(UINT channel = 0; channel < nAmountOfChannels; channel++)
    //        {
    //            int lValue = pBuffer->GetBuffer(channel, sample);

    //            float sample = float(m_bUseChExponent ? lValue * dLSBToSi : lValue); // Put the sample into whatever structure you want now.
    //            //cout << sample << " ";

    //            // check for triggers
    //            if(channel == EEGO_CHANNEL_TRG)
    //            {
    //                const uint currentTriggers = (uint)(lValue);
    //                const uint currentNewTriggers = currentTriggers & ~m_nLastTriggerValue; // Calculate which bits are new
    //                m_nLastTriggerValue = currentTriggers; // Save value for next trigger detection

    //                if(currentNewTriggers != 0)
    //                {
    //                    // Yay a trigger! Use it however you want
    //                }
    //            }
    //        }
    //    }

    //    // Memory cleanup
    //    pBuffer->Release();
    //    pBuffer = NULL;

    //    return true;
}


//*************************************************************************************************************

EEGoSportsPlugin::EEGO_GAIN EEGoSportsDriver::GetGainForSignalRange(int range)
{
    cout<<range<<endl;
    using namespace EEGoSportsPlugin;
    double idealGain = 1800. / range; // 1800 == MAX RANGE with only 1X amplification
    int realGain = 0; // The minimal gain

    if(EEGO_GAIN_1X <= idealGain && realGain <= EEGO_GAIN_1X)
        realGain = EEGO_GAIN_1X;

    if(EEGO_GAIN_2X <= idealGain && realGain <= EEGO_GAIN_2X)
        realGain = EEGO_GAIN_2X;

    if(EEGO_GAIN_3X <= idealGain && realGain <= EEGO_GAIN_3X)
        realGain = EEGO_GAIN_3X;

    if(EEGO_GAIN_4X <= idealGain && realGain <= EEGO_GAIN_4X)
        realGain = EEGO_GAIN_4X;

    if(EEGO_GAIN_6X <= idealGain && realGain <= EEGO_GAIN_6X)
        realGain = EEGO_GAIN_6X;

    if(EEGO_GAIN_8X <= idealGain && realGain <= EEGO_GAIN_8X)
        realGain = EEGO_GAIN_8X;

    if(EEGO_GAIN_12X <= idealGain && realGain <= EEGO_GAIN_12X)
        realGain = EEGO_GAIN_12X;

    return (EEGO_GAIN)realGain;
}
