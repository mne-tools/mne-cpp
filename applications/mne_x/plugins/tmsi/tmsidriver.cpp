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
{
}

TMSIDriver::~TMSIDriver()
{
}

MatrixXf TMSIDriver::getSampleMatrixValue()
{
    MatrixXf sampleValue;

//    int triggerChannel = -1;
//    int triggerStatus  = 0;

//    RTDeviceEx *Master;
//    ULONG SampleRate = MAX_SAMPLE_RATE;
//    ULONG BufferSize = MAX_BUFFER_SIZE;

//    ULONG PercentFull,Overflow;
//    ULONG BytesPerSample=0;
//    ULONG BytesReturned;
//    ULONG numHwChans;

//    // Buffer for storing the samples;
//    ULONG SignalBuffer[MY_BUFFER_SIZE];

//    Master=InitDevice(SampleRate);
//    if (Master == 0)
//    {
//        fprintf(stderr, "Cannot initialise device\n");
//        return sampleValue;
//    }
//    Master->SetSignalBuffer(&SampleRate,&BufferSize);

//    numHwChans = getTotalNumberOfChannels(Master, triggerChannel);
//    BytesPerSample = 4*numHwChans;

//    if( BytesPerSample == 0 )
//    {
//        fprintf(stderr, "Device returns no samples\n");
//        return sampleValue;
//    }

//    printf("Maximum sample rate   = %.3f Hz\n",(float) SampleRate / 1000.0);
//    printf("Maximum Buffer size   = %d Samples\n",(int) BufferSize);
//    printf("Number of HW channels = %d\n", (int) numHwChans);

//    /* these represent the acquisition system properties */
//    float fSample      = SampleRate/1000.0;
//    int nBufferSamp	   = 0;
//    int nTotalSamp	   = 0;

//    if (!Master->Start())
//    {
//        fprintf(stderr, "Unable to start the Device\n");
//        return sampleValue;
//    }

//    //Get Signal buffer information
//    Master->GetBufferInfo(&Overflow,&PercentFull);

//    if (PercentFull > 0)
//    {
//        // If there is data available, get samples from the device
//        // GetSamples returns the number of bytes written in the signal buffer
//        // This will always be a multiple op BytesPerSample.

//        // Divide the result by BytesPerSamples to get the number of samples returned
//        BytesReturned = Master->GetSamples((PULONG)SignalBuffer,sizeof(SignalBuffer));

//        if (BytesReturned != 0)
//        {
//            //loop on the channel
//            for(uint32 i=0; i<m_oHeader.getChannelCount(); i++)
//            {
//                //loop on the samples by channel
//                for(uint32 j=0; j<l_lmin; j++)
//                {
//                    m_pSample[m_ui32SampleIndex+j + i*m_ui32SampleCountPerSentBlock] =(float32)((((float32)m_ulSignalBuffer[(l_ui32IndexBuffer+j)*m_ui32NbTotalChannels +i])*m_vUnitGain[i]+m_vUnitOffSet[i])*pow(10.,(double)m_vExponentChannel[i]));
//                }
//            }
//        }
//    }
//    else
//    {
//        Sleep(1);
//    }

    return sampleValue;
}

RTDevice * TMSIDriver::SelectDevice( IN BOOLEAN Present )
{
    ULONG Count = 0;
    ULONG Max = 0;
    RTDevice *Device;

    Device = new RTDevice;

//    if( Device == NULL )
//        return NULL;

//    if( !Device->InitOk  )
//    {
//        delete Device;
//        return NULL;
//    }

//    PSP_DEVICE_PATH Id;

//    while(1)
//    {
//        TCHAR DeviceName[40] = _T("Unknown Device");
//        ULONG SerialNumber = 0;

//        HKEY hKey;

//        Id = Device->GetInstanceId( Count++ , Present , &Max );
//        if( !Id )
//            break;

//        hKey = Device->OpenRegistryKey( Id );

//        if( hKey != INVALID_HANDLE_VALUE )
//        {
//            ULONG Size;

//            Size = sizeof( SerialNumber );
//            RegQueryValueEx( hKey , _T("DeviceSerialNumber"), NULL , NULL , (PBYTE)&SerialNumber , &Size  );

//            Size = sizeof( DeviceName );
//            if( RegQueryValueEx( hKey , _T("DeviceDescription"), NULL , NULL , (PBYTE)&DeviceName[0] , &Size  )
//                == ERROR_SUCCESS )
//            {
//                _tprintf( "%lud . %s %lud\n" , Count , DeviceName , SerialNumber	);
//            }

//            RegCloseKey( hKey );
//        }

//        Device->Free( Id );
//    }

//    if( Max == 0 )
//    {
//        printf("There are no device connected to the PC\n");
//        return NULL;
//    }

//    if( Max == 1 )
//    {
//        Id = Device->GetInstanceId( 0 , Present );
//    }
//    else
//    {
//        printf("Please select device ...\n\n");
//        while( _kbhit() ){}
//        while( !_kbhit() ){}
//        int key = _getch() - '0';
//        Id = Device->GetInstanceId( key - 1 , Present );
//    }

//    if( !Device->Open( Id ) )
//    {
//        Device->Free( Id );
//        delete Device;
//        return NULL;
//    }

    return Device;
}

RTDevice * TMSIDriver::InitDevice( ULONG SampRate)
{
    ULONG Index;

    RTDevice *Device[MAX_DEVICE];
    for(Index=0;Index < MAX_DEVICE;Index++)
        Device[Index] = NULL;

    RTDevice *MasterL;

    Device[0] = SelectDevice( TRUE );

    MasterL = Device[0];

    if( MasterL == NULL )
    {
        _getch();
        return 0;
    }

    MasterL->Reset();

    return MasterL;
}

int TMSIDriver::getTotalNumberOfChannels(RTDevice *Master, int& triggerChannel)
{
    int numChan;
    PSIGNAL_FORMAT psf;

    psf = Master->GetSignalFormat(NULL);
    if (psf == NULL)
        return 0;

    // printf("%i x %i\n", (int) psf[0].Size, (int) psf[0].Elements);
    numChan = psf[0].Elements;
    triggerChannel = -1;

    for (int i=0;i<numChan;i++)
    {
        printf("Channel %i: %i %i ", i+1, (int) psf[i].Type, (int) psf[i].SubType);
        wprintf(psf[i].Name); // .Name field is of WCHAR type (unicode)
        printf("\n");

        // the documentation gives 0x13 for the type, but at least
        // for the Porti we need a "4".
        if (psf[i].Type == 4)
            triggerChannel = i;
    }
    // do we need to free this? or did we get static memory?
    // LocalFree(psf);
    return numChan;
}

