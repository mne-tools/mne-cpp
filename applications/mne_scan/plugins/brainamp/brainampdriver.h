//=============================================================================================================
/**
* @file     brainampdriver.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch, Viktor Klüber and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the BrainAMPDriver class. This class implements the basic communication between MNE-X and a ANT BrainAMP device
*
*/

#ifndef BRAINAMPDRIVER_H
#define BRAINAMPDRIVER_H


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
#include <eemagine/sdk/factory.h> // SDK header


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <qapplication.h>
#include <QVector>
#include <QSysInfo>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace BRAINAMPPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMPProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define EEGO_SDK_BIND_DYNAMIC // How to bind


//=============================================================================================================
/**
* BrainAMPDriver
*
* @brief The BrainAMPDriver class provides real time data acquisition of EEG data with a TMSi Refa device.
*/
class BrainAMPDriver
{
public:
    //=========================================================================================================
    /**
    * Constructs a BrainAMPDriver.
    *
    * @param [in] pBrainAMPProducer a pointer to the corresponding BrainAMP Producer class.
    */
    BrainAMPDriver(BrainAMPProducer* pBrainAMPProducer);

    //=========================================================================================================
    /**
    * Destroys the BrainAMPDriver.
    */
    ~BrainAMPDriver();

    //=========================================================================================================
    /**
    * Get sample from the device in form of a mtrix.
    * @param [in] MatrixXf the block sample values in form of a matrix.
    * @param [out] bool returns true if sample was successfully written to the input variable, false otherwise.
    */
    bool getSampleMatrixValue(Eigen::MatrixXd& sampleMatrix);

    //=========================================================================================================
    /**
    * Initialise device.
    * @param [in] iNumberOfChannels number of channels specified by the user.
    * @param [in] iSamplesPerBlock samples per block specified by the user.
    * @param [in] iSamplingFrequency sampling frequency specified by the user.
    * @param [in] bWriteDriverDebugToFile Flag for writing driver debug information to a file. Defined by the user via the GUI.
    * @param [in] sOutpuFilePath Holds the path for the output file. Defined by the user via the GUI.
    * @param [in] bMeasureImpedance Flag for measuring impedances.
    */
    bool initDevice(int iNumberOfChannels,
                    int iSamplesPerBlock,
                    int iSamplingFrequency,
                    bool bWriteDriverDebugToFile,
                    QString sOutpuFilePath,
                    bool bMeasureImpedance);

    //=========================================================================================================
    /**
    * Uninitialise device.
    * @param [out] bool returns true if device was successfully uninitialised, false otherwise.
    */
    bool uninitDevice();

private:
    BrainAMPProducer*           m_pBrainAMPProducer;          /**< A pointer to the corresponding BrainAMPProducer class.*/

    bool                        m_bInitDeviceSuccess;           /**< Flag which defines if the device initialisation was successfull.*/
    bool                        m_bDllLoaded;                   /**< Flag which defines if the driver DLL was loaded successfully.*/

    uint                        m_uiNumberOfChannels;           /**< The number of channels defined by the user via the GUI.*/
    uint                        m_uiSamplingFrequency;          /**< The sampling frequency defined by the user via the GUI (in Hertz).*/
    uint                        m_uiSamplesPerBlock;            /**< The samples per block defined by the user via the GUI.*/
    bool                        m_bWriteDriverDebugToFile;      /**< Flag for for writing driver debug informstions to a file. Defined by the user via the GUI.*/
    QString                     m_sOutputFilePath;              /**< Holds the path for the output file. Defined by the user via the GUI.*/
    bool                        m_bMeasureImpedances;           /**< Flag for impedance measuring mode.*/

    QVector<Eigen::VectorXd>    m_vecSampleBlockBuffer;         /**< Buffer to store all the incoming smaples. This is the buffer which is getting read from.*/

    eemagine::sdk::stream*      m_pDataStream;                  /**< The EEG/Impedance data stream.*/
    eemagine::sdk::amplifier*   m_pAmplifier;                   /**< Interface to the amplifier.*/

    std::ofstream               m_outputFileStream;             /**< fstream for writing the driver debug informations to a txt file.*/
};

} // NAMESPACE

#endif // BRAINAMPDRIVER_H
