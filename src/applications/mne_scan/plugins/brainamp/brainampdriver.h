//=============================================================================================================
/**
 * @file     brainampdriver.h
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
 * @brief    Contains the declaration of the BrainAMPDriver class.
 *
 */

#ifndef BRAINAMPDRIVER_H
#define BRAINAMPDRIVER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <windows.h>
#include "BrainAmpIoCtl.h"

#include "brainamp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMPProducer;

//=============================================================================================================
// DEFINES
//=============================================================================================================

// Number of ELements
#define NEL(x)  (sizeof(x) / sizeof(x[0]))

// Device names, in case that more than one device is used
//    (PCI systems with more than 128 channels),
//    the second device is named "\\\\.\\BrainAmp2".
// #define DEVICE_PCI		"\\\\.\\BrainAmp1"		// ISA/PCI device
#define DEVICE_PCI		"\\\\.\\BrainAmp"			// ISA/PCI device
#define DEVICE_USB		"\\\\.\\BrainAmpUSB1"		// USB device

// Different amplifier types
enum AmpTypes
{
    None = 0, Standard = 1, MR = 2, DCMRplus = 3, ExG_8 = 4, ExG_16 = 5
};

//=============================================================================================================
/**
 * BrainAMPDriver
 *
 * @brief The BrainAMPDriver class provides real time data acquisition of EEG data with a Brain Amp device.
 */
class BRAINAMPSHARED_EXPORT BrainAMPDriver
{

public:
    //=========================================================================================================
    /**
     * Constructs a BrainAMPDriver.
     *
     * @param[in] pEEGoSportsProducer a pointer to the corresponding EEGoSports Producer class.
     */
    BrainAMPDriver(BrainAMPProducer* pBrainAmpProducer);

    //=========================================================================================================
    /**
     * Destroys the BrainAMPDriver.
     */
    ~BrainAMPDriver();

    //=========================================================================================================
    /**
     * Get sample from the device in form of a mtrix.
     * @param[in] MatrixXf the block sample values in form of a matrix.
     *
     * @return Returns true if sample was successfully written to the input variable, false otherwise.
     */
    bool getSampleMatrixValue(Eigen::MatrixXd& sampleMatrix);

    //=========================================================================================================
    /**
     * Initialise device.
     * @param[in] iSamplesPerBlock samples per block specified by the user.
     * @param[in] iSamplingFrequency sampling frequency specified by the user.
     */
    bool initDevice(int iSamplesPerBlock,
                    int iSamplingFrequency);

    //=========================================================================================================
    /**
     * Opens the device.
     *
     * @return returns true if device was successfully opened, false otherwise.
     */
    bool openDevice();

    //=========================================================================================================
    /**
     * Finds amplifiers.
     *
     * @return returns number of successive amplifiers starting from the first position.
     */
    int findAmplifiers();

    //=========================================================================================================
    /**
     * Uninitialise device.
     *
     * @return returns true if device was successfully uninitialised, false otherwise.
     */
    bool uninitDevice();

private:
    BrainAMPProducer*           m_pBrainAmpProducer;                /**< A pointer to the corresponding BrainAmpProducer class.*/

    bool                        m_bInitDeviceSuccess;               /**< Flag which defines if the device initialisation was successfull.*/
    bool                        m_bDllLoaded;                       /**< Flag which defines if the driver DLL was loaded successfully.*/

    uint                        m_uiDownsample;                     /**< The number of channels defined by the user via the GUI.*/
    uint                        m_uiSamplingFrequency;              /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    uint                        m_uiSamplesPerBlock;                /**< The samples per block defined by the user via the GUI.*/

    HANDLE                      DeviceAmp;                          /**< Amplifier device.*/

    bool                        UsbDevice;                          /**< If true, the connected device is an USB box, otherwise a PCI/ISA host adapter.*/

    int                         DriverVersion;                      /**< Driver version.*/

    BA_SETUP                    Setup;                              /**< Setup structure.*/
};
} // NAMESPACE

#endif // BRAINAMPDRIVER_H
