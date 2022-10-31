//=============================================================================================================
/**
 * @file     brainampdriver.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the BrainAMPDriver class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainampdriver.h"
#include "brainampproducer.h"

#include <conio.h>
#include <stdio.h>
#include <io.h>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AmpTypes amplifiers[4] = { None, None, None, None };            /**< Connected amplifiers.*/

BrainAMPDriver::BrainAMPDriver(BrainAMPProducer* pBrainAmpProducer)
: m_pBrainAmpProducer(pBrainAmpProducer)
, m_bInitDeviceSuccess(false)
, m_uiSamplingFrequency(1000)
, m_uiSamplesPerBlock(200)
, DeviceAmp(INVALID_HANDLE_VALUE)
, UsbDevice(false)
, DriverVersion(0)
{
}

//=============================================================================================================

BrainAMPDriver::~BrainAMPDriver()
{
}

//=============================================================================================================

bool BrainAMPDriver::initDevice(int iSamplesPerBlock,
                                int iSamplingFrequency)
{
    m_uiSamplesPerBlock = iSamplesPerBlock;
    m_uiSamplingFrequency = iSamplingFrequency;
    m_uiDownsample = 1; //no downsample

    //fs is always 5000Hz
    if(5000 % m_uiSamplingFrequency == 0) {
        m_uiDownsample = 5000/m_uiSamplingFrequency;
    }

    // Open device
    if (!openDevice())
    {
        printf("No BrainAmp USB adapter and no ISA/PCI adapter found!\n");
        return false;
    }

    // Show version
    unsigned nModule = DriverVersion % 10000,
             nMinor = (DriverVersion % 1000000) / 10000,
             nMajor = DriverVersion / 1000000;

    printf("BrainAMPDriver::initDevice - %s Driver Found, Version %u.%02u.%04u\n", UsbDevice ? "USB" : "ISA/PCI", nMajor, nMinor, nModule);

    // Send simplest setup, 32 channels, one amp. AC, 1000Hz, 100nV
    Setup.nChannels = 32;
    Setup.nHoldValue = 0x0;                                     // Value without trigger
    Setup.nPoints = m_uiSamplesPerBlock * m_uiDownsample;       //40 * 5;		// 5 kHz = 5 points per ms -> 40 ms data block

    for (int i = 0; i < Setup.nChannels; i++) {
        Setup.nChannelList[i] = i;
        //Setup.n250Hertz[i] = 0;                 // Low pass 1000Hz
    }

    DWORD dwBytesReturned = 0;
    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_SETUP, &Setup, sizeof(Setup), NULL, 0, &dwBytesReturned, NULL))
    {
        printf("BrainAMPDriver::initDevice - Setup failed, error code: %u\n", ::GetLastError());
    }

    // Start acqusition process
    // Pulldown input resistors for trigger input, (active high)
    unsigned short pullup = 0;

    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_DIGITALINPUT_PULL_UP, &pullup, sizeof(pullup), NULL, 0, &dwBytesReturned, NULL))
    {
        printf("BrainAMPDriver::initDevice - Can't set pull up/down resistors, error code: %u\n", ::GetLastError());
    }

    // Make sure that amps exist, otherwise a long timeout will occur.
    int nHighestChannel = 0;
    for (int i = 0; i < Setup.nChannels; i++)
    {
        nHighestChannel = max(Setup.nChannelList[i], nHighestChannel);
    }

    int nRequiredAmps = (nHighestChannel + 1) / 32;
    int nAmps = findAmplifiers();

    if (nAmps < nRequiredAmps)
    {
        printf("BrainAMPDriver::initDevice - Required Amplifiers: %d, Connected Amplifiers: %d\n", nRequiredAmps, nAmps);
        return false;
    }

    // Start acquisition
    long acquisitionType = 1;

    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_START, &acquisitionType, sizeof(acquisitionType), NULL, 0, &dwBytesReturned, NULL))
    {
        printf("BrainAMPDriver::initDevice - Start failed, error code: %u\n", ::GetLastError());
    }

    // Set flag for successfull initialisation true
    m_bInitDeviceSuccess = true;

    return true;
}

//=============================================================================================================

bool BrainAMPDriver::openDevice()
{
    DWORD dwFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH;
    // First try USB box
    DeviceAmp = CreateFileA(DEVICE_USB, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL);

    if (DeviceAmp != INVALID_HANDLE_VALUE)
    {
        UsbDevice = true;
    }
    else
    {
        // USB box not found, try PCI host adapter
        UsbDevice = false;
        DeviceAmp = CreateFileA(DEVICE_PCI, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, dwFlags, NULL);
    }

    // Retrieve driver version
    if (DeviceAmp != INVALID_HANDLE_VALUE)
    {
        DriverVersion = 0;
        DWORD dwBytesReturned;
        DeviceIoControl(DeviceAmp, IOCTL_BA_DRIVERVERSION, NULL, 0, &DriverVersion, sizeof(DriverVersion), &dwBytesReturned, NULL);
    }

    return (DeviceAmp != INVALID_HANDLE_VALUE);
}

//=============================================================================================================

int BrainAMPDriver::findAmplifiers()
{
    USHORT amps[4];
    DWORD dwBytesReturned;

    DeviceIoControl(DeviceAmp, IOCTL_BA_AMPLIFIER_TYPE, NULL, 0, amps, sizeof(amps), &dwBytesReturned, NULL);

    int nAmps = 4;
    for (int i = 0; i < 4; i++)
    {
        amplifiers[i] = (AmpTypes)amps[i];
        if (amplifiers[i] == None && i < nAmps)
        {
            nAmps = i;
        }
    }
    return nAmps;
}

//=============================================================================================================

bool BrainAMPDriver::uninitDevice()
{
    //Check if the device was initialised
    if(!m_bInitDeviceSuccess)
    {
        printf("Plugin BrainAmp - ERROR - uninitDevice() - Device was not initialised - therefore can not be uninitialised\n");
        return false;
    }

    // Stop acquisition
    DWORD dwBytesReturned;
    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_STOP, NULL, 0, NULL, 0, &dwBytesReturned, NULL))
    {
        printf("Stop failed, error code: %u\n", ::GetLastError());
    }

    return true;
}

//=============================================================================================================

bool BrainAMPDriver::getSampleMatrixValue(Eigen::MatrixXd &sampleMatrix)
{
    //Check if device was initialised and connected correctly
    if(!m_bInitDeviceSuccess)
    {
        printf("BrainAMPDriver::getSampleMatrixValue - getSampleMatrixValue() - Cannot start to get samples from device because device was not initialised correctly");
        return false;
    }

    //printf("BrainAMPDriver::getSampleMatrixValue iDownsample: %d\n", m_uiDownsample);

    sampleMatrix = Eigen::MatrixXd(Setup.nChannels + 1, m_uiSamplesPerBlock);
    sampleMatrix.setZero(); // Clear matrix - set all elements to zero

    // Get the data
    // Data including marker channel
    vector<short>pnData((Setup.nChannels + 1) * Setup.nPoints);

    // Pure data
    // Check for error
    int nTemp = 0;
    DWORD dwBytesReturned;
    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_ERROR_STATE, NULL, 0, &nTemp, sizeof(nTemp), &dwBytesReturned, NULL))
    {
        printf("Acquisition Error, GetLastError(): %d\n", ::GetLastError());
        return false;
    }
    if (nTemp != 0)
    {
        printf("Acquisition Error %d\n", nTemp);
        return false;
    }

    // Receive data
    bool bBlockReceived = false;
    int nTransferSize = (int)pnData.size() * sizeof(short);

    while(!bBlockReceived) {
        if (!ReadFile(DeviceAmp, &pnData[0], nTransferSize, &dwBytesReturned, NULL))
        {
            printf("Acquisition Error, GetLastError(): %d\n", ::GetLastError());
            return false;
        }

        //printf("BrainAMPDriver::getSampleMatrixValue - nTransferSize %u\n", nTransferSize);
        //printf("BrainAMPDriver::getSampleMatrixValue - dwBytesReturned %u \n", dwBytesReturned);

        if (!dwBytesReturned)
        {
            //Sleep(1);
            continue; //jumps to end of while statement
        }

        //Transform into matrix structure
        int counter = 0;
        for (int i = 0; i < m_uiSamplesPerBlock; ++i) {
            for (int n = 0; n < Setup.nChannels + 1; ++n) {
                sampleMatrix(n,i) = pnData[counter] * 100e-09;
                counter++;
            }

            if(counter + ((m_uiDownsample - 1) * (Setup.nChannels + 1)) < pnData.size()) {
                counter += ((m_uiDownsample - 1) * (Setup.nChannels + 1));
            }
        }

        bBlockReceived = true;
    }

    //Receive info about buffer status
    long nState = 0;

    if (!DeviceIoControl(DeviceAmp, IOCTL_BA_BUFFERFILLING_STATE, NULL, 0, &nState, sizeof(nState), &dwBytesReturned, NULL))
    {
        printf("BrainAMPDriver::initDevice - Buffer state failed, error code: %u\n", ::GetLastError());
    }

    printf("BrainAMPDriver::initDevice - Buffer state: %u\n", nState);

    //Sleep(10);

    return true;
}
