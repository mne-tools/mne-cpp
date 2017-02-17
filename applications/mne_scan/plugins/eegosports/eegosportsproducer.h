//=============================================================================================================
/**
* @file     eegosportsproducer.h
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
* @brief    Contains the declaration of the EEGoSportsProducer class.
*
*/

#ifndef EEGOSPORTSPRODUCER_H
#define EEGOSPORTSPRODUCER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosports_global.h"

#include <generics/circularbuffer.h>
#include <Eigen/Eigen>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class EEGoSports;
class EEGoSportsDriver;


//=============================================================================================================
/**
* DECLARE CLASS EEGoSportsProducer
*
* @brief The EEGoSportsProducer class provides a EEG data producer for a given sampling rate.
*/
class EEGOSPORTSSHARED_EXPORT EEGoSportsProducer : public QThread
{

public:
    //=========================================================================================================
    /**
    * Constructs a EEGoSportsProducer.
    *
    * @param [in] pEEGoSports a pointer to the corresponding EEGoSports class.
    */
    EEGoSportsProducer(EEGoSports* pEEGoSports);

    //=========================================================================================================
    /**
    * Destroys the EEGoSportsProducer.
    */
    ~EEGoSportsProducer();

    //=========================================================================================================
    /**
    * Starts the EEGoSportsProducer by starting the producer's thread and initialising the device.
    * @param [in] iNumberOfChannels The number of channels defined by the user via the GUI.
    * @param [in] iSamplesPerBlock The samples per block defined by the user via the GUI.
    * @param [in] iSamplingFrequency The sampling frequency defined by the user via the GUI (in Hertz).
    * @param [in] bWriteDriverDebugToFile Flag for writing the received samples to a file. Defined by the user via the GUI.
    * @param [in] sOutpuFilePath Holds the path for the output file. Defined by the user via the GUI.
    * @param [in] bMeasureImpedance Flag for measuring impedances.
    */
    virtual void start(int iNumberOfChannels,
                       int iSamplesPerBlock,
                       int iSamplingFrequency,
                       bool bWriteDriverDebugToFile,
                       QString sOutputFilePath,
                       bool bMeasureImpedance);

    //=========================================================================================================
    /**
    * Stops the EEGoSportsProducer by stopping the producer's thread.
    */
    void stop();

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    EEGoSports*                         m_pEEGoSports;              /**< A pointer to the corresponding EEGoSports class.*/
    QSharedPointer<EEGoSportsDriver>    m_pEEGoSportsDriver;        /**< A pointer to the corresponding EEGoSportsDriver class.*/

    bool                                m_bIsRunning;               /**< Whether EEGoSportsProducer is running.*/
};

} // NAMESPACE

#endif // EEGOSPORTSPRODUCER_H
