//=============================================================================================================
/**
* @file     gusbampdriver.h
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
* @brief    Contains the declaration of the GUSBAmpdriver class. This class implements the basic communication between MNE-X and a GUSBAmp Refa device
*
*/

#ifndef GUSBAMPDRIVER_H
#define GUSBAMPDRIVER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE GUSBAmpPlugin
//=============================================================================================================

namespace GUSBAmpPlugin
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

class GUSBAmpProducer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================


//=============================================================================================================
/**
* GUSBAmpDriver
*
* @brief The GUSBAmpDriver class provides real time data acquisition of EEG data with a GUSBAmp device.
*/
class GUSBAmpDriver
{

public:
    //=========================================================================================================
    /**
    * Constructs a GUSBAmpDriver.
    *
    * @param [in]   pGUSBAmpProducer a pointer to the corresponding GUSBAmp Producer class.
    */
    GUSBAmpDriver(GUSBAmpProducer* pGUSBAmpProducer);

    //=========================================================================================================
    /**
    * Destroys the GUSBAmpDriver.
    */
    ~GUSBAmpDriver();

    //=========================================================================================================
    /**
    * Get sample from the device in form of a mtrix.
    * @param [in]   MatrixXf the block sample values in form of a matrix.
    * @return       returns true if sample was successfully written to the input variable, false otherwise.
    */
    bool getSampleMatrixValue(MatrixXf& sampleMatrix);

    //=========================================================================================================
    /**
    * Initialise device.    
    */
    bool initDevice();

    //=========================================================================================================
    /**
    * Uninitialise device.
    * @return       returns true if device was successfully uninitialised, false otherwise.
    */
    bool uninitDevice();

protected:
    GUSBAmpProducer*       m_pGUSBAmpProducer;                /**< A pointer to the corresponding GUSBAmpProducer class.*/
};

} // NAMESPACE

#endif // GUSBAMPDRIVER_H
