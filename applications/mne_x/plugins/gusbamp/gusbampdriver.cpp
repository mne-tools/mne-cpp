//=============================================================================================================
/**
* @file     gusbampdriver.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the GUSBAmpDriver class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampdriver.h"
#include "gusbampproducer.h"
#include <iostream>
#include <Windows.h>
#include <deque>




//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAmpPlugin;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpDriver::GUSBAmpDriver(GUSBAmpProducer* pGUSBAmpProducer)
: m_pGUSBAmpProducer(pGUSBAmpProducer)
,SLAVE_SERIALS_SIZE(0)
,SAMPLE_RATE_HZ(128)
,NUMBER_OF_SCANS(8)
,NUMBER_OF_CHANNELS(3)
,TRIGGER(FALSE)
,_mode(M_NORMAL)
,_commonReference({ FALSE, FALSE, FALSE, FALSE })
,_commonGround({ FALSE, FALSE, FALSE, FALSE })
,BUFFER_SIZE_SECONDS(5)
,QUEUE_SIZE(4)
,nPoints(NUMBER_OF_SCANS * (NUMBER_OF_CHANNELS + TRIGGER))
,bufferSizeBytes(HEADER_SIZE + nPoints * sizeof(float))
,numBytesReceived(0)
{

    //Linking the specific API-library to the project
    #ifdef _WIN64
        #pragma comment(lib, __FILE__"\\..\\gUSBamp_x64.lib")
    #else
        #pragma comment(lib, __FILE__"\\..\\gUSBamp_x86.lib")
    #endif


    //initializing a deque-list of the serial numbers to be called (LPSTR)
    _masterSerial       = LPSTR("UB-2015.05.16");
    _slaveSerials[0]    = LPSTR("UB-2010.03.43");
    _slaveSerials[1]    = LPSTR("UB-2010.03.44");
    _slaveSerials[2]    = LPSTR("UB-2010.03.47");
    for (int i=0; i<SLAVE_SERIALS_SIZE; i++)
        callSequenceSerials.push_back(_slaveSerials[i]);
    //add the master device at the end of the list!
    callSequenceSerials.push_back(_masterSerial);


    //initializing UCHAR-list of channels to acquire
    for(int i = 0; i < NUMBER_OF_CHANNELS; i++)
        _channelsToAcquire[i] = UCHAR(i+1);

    float version = float(GT_GetDriverVersion());

    qDebug() <<endl<< "gUSBampdriver-contructor build successfully " << endl;
    qDebug() << "gUSBamp-driver version:" << version << "was found";


}


//*************************************************************************************************************

GUSBAmpDriver::~GUSBAmpDriver()
{

    //qDebug() << "GUSBAmpDriver::~GUSBAmpDriver()" << endl;
}


//*************************************************************************************************************

bool GUSBAmpDriver::initDevice()
{


    /*Space for setting Main-variables
    *
    *
    *
    */



    qDebug() << "starting initializing gUSBamp devices" << endl;



    try
    {
        for (deque<LPSTR>::iterator serialNumber = callSequenceSerials.begin(); serialNumber != callSequenceSerials.end(); serialNumber++)
        {
            //open the device
            HANDLE hDevice = GT_OpenDeviceEx(*serialNumber);

            if (hDevice == NULL)
            {
                qDebug() << "opening the device " << QString(*serialNumber)<< "failed" << endl;
                throw string("Error on GT_OpenDeviceEx: Couldn't open device ").append(*serialNumber);

            }
            //add the device handle to the list of opened devices
            openedDevicesHandles.push_back(hDevice);

            //set the channels from that data should be acquired
            if (!GT_SetChannels(hDevice, _channelsToAcquire, NUMBER_OF_CHANNELS))
                throw string("Error on GT_SetChannels: Couldn't set channels to acquire for device ").append(*serialNumber);

            //set the sample rate
            if (!GT_SetSampleRate(hDevice, SAMPLE_RATE_HZ))
                throw string("Error on GT_SetSampleRate: Couldn't set sample rate for device ").append(*serialNumber);

            //disable the trigger line
            if (!GT_EnableTriggerLine(hDevice, TRIGGER))
                throw string("Error on GT_EnableTriggerLine: Couldn't enable/disable trigger line for device ").append(*serialNumber);

            //set the number of scans that should be received simultaneously
            if (!GT_SetBufferSize(hDevice, NUMBER_OF_SCANS))
                throw string("Error on GT_SetBufferSize: Couldn't set the buffer size for device ").append(*serialNumber);

            //don't use bandpass and notch for each channel
            for (int i=0; i<NUMBER_OF_CHANNELS; i++)
            {
                //don't use a bandpass filter for any channel
                if (!GT_SetBandPass(hDevice, _channelsToAcquire[i], -1))
                    throw string("Error on GT_SetBandPass: Couldn't set no bandpass filter for device ").append(*serialNumber);

                //don't use a notch filter for any channel
                if (!GT_SetNotch(hDevice, _channelsToAcquire[i], -1))
                    throw string("Error on GT_SetNotch: Couldn't set no notch filter for device ").append(*serialNumber);
            }

            //determine master device as the last device in the list
            bool isSlave = (*serialNumber != callSequenceSerials.back());

            //set slave/master mode of the device
            if (!GT_SetSlave(hDevice, isSlave))
                throw string("Error on GT_SetSlave: Couldn't set slave/master mode for device ").append(*serialNumber);

            //disable shortcut function
            if (!GT_EnableSC(hDevice, false))
                throw string("Error on GT_EnableSC: Couldn't disable shortcut function for device ").append(*serialNumber);

            //set unipolar derivation
            _bipolarSettings.Channel1 = 0;
            _bipolarSettings.Channel2 = 0;
            _bipolarSettings.Channel3 = 0;
            _bipolarSettings.Channel4 = 0;
            _bipolarSettings.Channel5 = 0;
            _bipolarSettings.Channel6 = 0;
            _bipolarSettings.Channel7 = 0;
            _bipolarSettings.Channel8 = 0;
            _bipolarSettings.Channel9 = 0;
            _bipolarSettings.Channel10 = 0;
            _bipolarSettings.Channel11 = 0;
            _bipolarSettings.Channel12 = 0;
            _bipolarSettings.Channel13 = 0;
            _bipolarSettings.Channel14 = 0;
            _bipolarSettings.Channel15 = 0;
            _bipolarSettings.Channel16 = 0;

            if (!GT_SetBipolar(hDevice, _bipolarSettings))
                throw string("Error on GT_SetBipolar: Couldn't set unipolar derivation for device ").append(*serialNumber);

            if (_mode == M_COUNTER)
                if (!GT_SetMode(hDevice, M_NORMAL))
                    throw string("Error on GT_SetMode: Couldn't set mode M_NORMAL (before mode M_COUNTER) for device ").append(*serialNumber);

            //set acquisition mode
            if (!GT_SetMode(hDevice, _mode))
                throw string("Error on GT_SetMode: Couldn't set mode for device ").append(*serialNumber);

            //for g.USBamp devices set common ground and common reference
            if (strncmp(*serialNumber, "U", 1) == 0 && (_mode == M_NORMAL || _mode == M_COUNTER))
            {
                //don't connect the 4 groups to common reference
                if (!GT_SetReference(hDevice, _commonReference))
                    throw string("Error on GT_SetReference: Couldn't set common reference for device ").append(*serialNumber);

                //don't connect the 4 groups to common ground
                if (!GT_SetGround(hDevice, _commonGround))
                    throw string("Error on GT_SetGround: Couldn't set common ground for device ").append(*serialNumber);
            }

            printf("\tg.USBamp %s initialized as %s (#%d in the call sequence)!\n", *serialNumber, (isSlave) ? "slave" : "master", openedDevicesHandles.size());

        }



        //create _callSequenceHandles for the sequence of calling the devices (Master has to be the last device to be called!)
        _callSequenceHandles = openedDevicesHandles;
        numDevices = (int) _callSequenceHandles.size();

        buffers     = new BYTE**[numDevices];
        overlapped  = new OVERLAPPED*[numDevices];

        //for each device create a number of QUEUE_SIZE data buffers
        for (int deviceIndex=0; deviceIndex<numDevices; deviceIndex++)
        {
            buffers[deviceIndex] = new BYTE*[QUEUE_SIZE];
            overlapped[deviceIndex] = new OVERLAPPED[QUEUE_SIZE];

            //for each data buffer allocate a number of bufferSizeBytes bytes
            for (int queueIndex=0; queueIndex<QUEUE_SIZE; queueIndex++)
            {
                buffers[deviceIndex][queueIndex] = new BYTE[bufferSizeBytes];
                memset(&(overlapped[deviceIndex][queueIndex]), 0, sizeof(OVERLAPPED));

                //create a windows event handle that will be signalled when new data from the device has been received for each data buffer
                overlapped[deviceIndex][queueIndex].hEvent = CreateEvent(NULL, false, false, NULL);
            }
        }

        //start the devices (master device must be started at last)
        for (int deviceIndex=0; deviceIndex<numDevices; deviceIndex++)
        {
            HANDLE hDevice = _callSequenceHandles[deviceIndex];

            if (!GT_Start(hDevice))
            {
                //throw string("Error on GT_Start: Couldn't start data acquisition of device.");
                cout << "\tError on GT_Start: Couldn't start data acquisition of device.\n";
                return 0;

            }
            //queue-up the first batch of transfer requests
            for (int queueIndex=0; queueIndex<QUEUE_SIZE; queueIndex++)
            {
                if (!GT_GetData(hDevice, buffers[deviceIndex][queueIndex], bufferSizeBytes, &overlapped[deviceIndex][queueIndex]))
                {
                    cout << "\tError on GT_GetData.\n";
                    return 0;
                }
            }
        }


        qDebug() << "Plugin GUSBAmp - INFO - initDevice() - The device has been connected and initialised successfully" << endl;

        return true;

    }
    catch (string& exception)
    {
        qDebug() << "error occurred - catch{}-instruction was started" << endl;

        //in case an exception occurred, close all opened devices...
        while(!openedDevicesHandles.empty())
        {
            GT_CloseDevice(&openedDevicesHandles.front());
            qDebug() << "error occurred - Device " << &openedDevicesHandles.front()  << "was closed" << endl;
            openedDevicesHandles.pop_front();
        }



        cout << exception << '\n';


        return false;

    }
}


//*************************************************************************************************************

bool GUSBAmpDriver::uninitDevice()
{


    cout << "Stopping devices and cleaning up..." << "\n";

    //clean up allocated resources for each device
    for (int i=0; i<numDevices; i++)
    {
        HANDLE hDevice = _callSequenceHandles[i];

        //clean up allocated resources for each queue per device
        for (int j=0; j<QUEUE_SIZE; j++)
        {
            WaitForSingleObject(overlapped[i][j].hEvent, 1000);
            CloseHandle(overlapped[i][j].hEvent);
            delete [] buffers[i][j];
            qDebug()<< "deleted queue buffer" << j << "successfully";
        }

        //stop device
        GT_Stop(hDevice);
        qDebug() << "stopped " << QString(callSequenceSerials[i])<< " successfully" << endl;

        //reset device
        GT_ResetTransfer(hDevice);
        qDebug() << "reseted Transfer of " << QString(callSequenceSerials[i])<< " successfully" << endl;

        delete [] overlapped[i];
        delete [] buffers[i];
    }

    delete [] buffers;
    delete [] overlapped;

    //closing all devices
    while (!_callSequenceHandles.empty())
    {
        //closes each opened device and removes it from the call sequence
        GT_CloseDevice(&_callSequenceHandles.front());
        _callSequenceHandles.pop_front();
    }

    //closes all openend Device Handles
    while(!openedDevicesHandles.empty())
    {
        GT_CloseDevice(&openedDevicesHandles.front());
        qDebug() << "error occurred - Device " << &openedDevicesHandles.front()  << "was closed" << endl;
        openedDevicesHandles.pop_front();
    }


    qDebug() << "Plugin GUSBAmp - INFO - uninitDevice() - Successfully uninitialised the device" << endl;

    return true;
}


//*************************************************************************************************************


bool GUSBAmpDriver::getSampleMatrixValue(MatrixXf& sampleMatrix)
{
    sampleMatrix.setZero(); // Clear matrix - set all elements to zero


    for(int queueIndex=0; queueIndex<QUEUE_SIZE; queueIndex++)
    {

        //receive data from each device
        for (int deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            HANDLE hDevice = _callSequenceHandles[deviceIndex];

            //wait for notification from the system telling that new data is available
            if (WaitForSingleObject(overlapped[deviceIndex][queueIndex].hEvent, 1000) == WAIT_TIMEOUT)
            {
                //throw string("Error on data transfer: timeout occurred.");
                cout << "Error on data transfer: timeout occurred." << "\n";
                return 0;
            }
            //qDebug() << "Waiting procedure succeeded ";

            //get number of received bytes...
            GetOverlappedResult(hDevice, &overlapped[deviceIndex][queueIndex], &numBytesReceived, false);

            //...and check if we lost something (number of received bytes must be equal to the previously allocated buffer size)
            if (numBytesReceived != bufferSizeBytes)
            {
                //throw string("Error on data transfer: samples lost.");
                cout << "Error on data transfer: samples lost." << "\n";
                return 0;
            }
        }



        //store received data from each device in the correct order (that is scan-wise, where one scan includes all channels of all devices) ignoring the header
        //Data is aligned as follows: element at position destBuffer[scanIndex * (numberOfChannelsPerDevice * numDevices) + channelIndex] is sample of channel channelIndex (zero-based) of the scan with zero-based scanIndex.
        //channelIndex ranges from 0..numDevices*numChannelsPerDevices where numDevices equals the number of recorded devices and numChannelsPerDevice the number of channels from each of those devices.
        //It is assumed that all devices provide the same number of channels.
        for (int scanIndex = 0; scanIndex < NUMBER_OF_SCANS; scanIndex++)
        {
            for (int deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
            {
                for(int channelIndex = 0; channelIndex<NUMBER_OF_CHANNELS; channelIndex++)
                {
                    BYTE ByteValue[sizeof(float)];
                    float FloatValue;

                    for(int i=0;i<sizeof(float);i++)
                    {
                        ByteValue[i] = buffers[deviceIndex][queueIndex][(scanIndex * (NUMBER_OF_CHANNELS + TRIGGER) + channelIndex) * sizeof(float) + HEADER_SIZE + i];
                    }
                    memcpy(&FloatValue, &ByteValue, sizeof(FloatValue));
                    cout << FloatValue<< "\t";
                }
            }
            cout << "\n";
        }







        //add new GetData call to the queue replacing the currently received one
        for (int deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
            if (!GT_GetData(_callSequenceHandles[deviceIndex], buffers[deviceIndex][queueIndex], bufferSizeBytes, &overlapped[deviceIndex][queueIndex]))
            {
                cout << "\tError on GT_GetData.\n";
                return 0;
            }

    }

    return true;
}








