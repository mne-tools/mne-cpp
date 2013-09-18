//=============================================================================================================
/**
* @file     tmsidriver.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
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
* @brief    Contains the declaration of the tmsidriver class.
*
*/

#ifndef TMSIDRIVER_H
#define TMSIDRIVER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <map>
#include <tchar.h>
#include <string.h>
#include <windows.h>
#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// TMSi Driver INCLUDES
//=============================================================================================================

//#include "RtDevice.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace TMSIPlugin
{


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

class TMSIProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define MAX_DEVICE			1	//Max number of devices supported by this driver
#define MY_BUFFER_SIZE      2500


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
    * @param [in] pTMSIProducer a pointer to the corresponding TMSI Producer class.
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
    */
    MatrixXf getSampleMatrixValue();

    //=========================================================================================================
    /**
    * Initialise device .
    */
    void InitDevice();

protected:


private:
    TMSIProducer*     m_pTMSIProducer;            /**< A pointer to the corresponding TMSIProducer class.*/

    int               m_iNumberOfChannels;
    int               m_iSamplingFrequency;
    int               m_iSamplesPerblock;
};

} // NAMESPACE

#endif // TMSIDRIVER_H
