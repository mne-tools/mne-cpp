//=============================================================================================================
/**
 * @file     brainampproducer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
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
 * @brief    Contains the declaration of the BrainAMPProducer class.
 *
 */

#ifndef BRAINAMPPRODUCER_H
#define BRAINAMPPRODUCER_H


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp_global.h"

#include <utils/generics/circularbuffer.h>


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>


//=============================================================================================================
// DEFINE NAMESPACE BRAINAMPPLUGIN
//=============================================================================================================

namespace BRAINAMPPLUGIN
{


//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainAMP;
class BrainAMPDriver;


//=============================================================================================================
/**
 * DECLARE CLASS BrainAMPProducer
 *
 * @brief The BrainAMPProducer class provides a EEG data producer for a given sampling rate.
 */
class BRAINAMPSHARED_EXPORT BrainAMPProducer : public QThread
{
public:
    //=========================================================================================================
    /**
     * Constructs a BrainAMPProducer.
     *
     * @param [in] pBrainAmp a pointer to the corresponding BrainAmp class.
     */
    BrainAMPProducer(BrainAMP* pBrainAmp);

    //=========================================================================================================
    /**
     * Destroys the BrainAMPProducer.
     */
    ~BrainAMPProducer();

    //=========================================================================================================
    /**
     * Starts the BrainAMPProducer by starting the producer's thread and initialising the device.
     * @param [in] iSamplesPerBlock The samples per block defined by the user via the GUI.
     * @param [in] iSamplingFrequency The sampling frequency defined by the user via the GUI (in Hertz).
     * @param [in] sOutpuFilePath Holds the path for the output file. Defined by the user via the GUI.
     * @param [in] bMeasureImpedance Flag for measuring impedances.
     */
    virtual void start(int iSamplesPerBlock,
                       int iSamplingFrequency,
                       QString sOutputFilePath,
                       bool bMeasureImpedance);

    //=========================================================================================================
    /**
     * Stops the BrainAMPProducer by stopping the producer's thread.
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
    BrainAMP*                           m_pBrainAmp;            /**< A pointer to the corresponding BrainAmp class.*/
    QSharedPointer<BrainAMPDriver>      m_pBrainAmpDriver;      /**< A pointer to the corresponding BrainAmp driver class.*/

    bool                                m_bIsRunning;           /**< Whether BrainAMPProducer is running.*/
};

} // NAMESPACE

#endif // BRAINAMPPRODUCER_H
