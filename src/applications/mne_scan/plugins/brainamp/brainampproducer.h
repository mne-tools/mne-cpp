//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     brainampproducer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Contains the declaration of the BrainAMPProducer class.
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
     * @param[in] pBrainAmp a pointer to the corresponding BrainAmp class.
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
     * @param[in] iSamplesPerBlock The samples per block defined by the user via the GUI.
     * @param[in] iSamplingFrequency The sampling frequency defined by the user via the GUI (in Hertz).
     */
    virtual void start(int iSamplesPerBlock,
                       int iSamplingFrequency);

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
