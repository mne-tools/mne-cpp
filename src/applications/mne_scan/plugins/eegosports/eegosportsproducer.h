//=============================================================================================================
/**
 * @file     eegosportsproducer.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Christoph Dinh, Lorenz Esch, Viktor Klueber, Johannes Vorwerk. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE EEGOSPORTSPLUGIN
//=============================================================================================================

namespace EEGOSPORTSPLUGIN
{

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
class EEGoSportsProducer : public QThread
{

public:
    //=========================================================================================================
    /**
     * Constructs a EEGoSportsProducer.
     *
     * @param[in] pEEGoSports a pointer to the corresponding EEGoSports class.
     */
    EEGoSportsProducer(EEGoSports* pEEGoSports);

    //=========================================================================================================
    /**
     * Destroys the EEGoSportsProducer.
     */
    ~EEGoSportsProducer();

    //=========================================================================================================
    /**
    * Initializes the EEGoSportsProducer by initialising the device.
    *
    * @param[in] bWriteDriverDebugToFile Flag for writing the received samples to a file. Defined by the user via the GUI.
    * @param[in] bMeasureImpedance Flag for measuring impedances.
    */
    bool init(bool bWriteDriverDebugToFile,
              bool bMeasureImpedance);

    //=========================================================================================================
    /**
    * Starts the EEGoSportsProducer by starting the producer's thread and starting the stream from the device.
    *
    * @param[in] iSamplesPerBlock The samples per block defined by the user via the GUI.
    * @param[in] iSamplingFrequency The sampling frequency defined by the user via the GUI (in Hertz).
    * @param[in] bMeasureImpedance Flag for measuring impedances.
    */
    virtual void start(int iSamplesPerBlock,
                       int iSamplingFrequency,
                       bool bMeasureImpedance);

    //=========================================================================================================
    /**
     * Stops the EEGoSportsProducer by stopping the producer's thread.
     */
    void stop();

    //=========================================================================================================
    /**
    * Get list of channel types.
    */
    QList<uint> getChannellist();

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

    bool                                m_bMeasureImpedance;        /**< Whether it is an impedance measurement.*/
    bool                                m_bIsConnected;             /**< Whether EEGoSportsProducer is connected to device.*/
};
} // NAMESPACE

#endif // EEGOSPORTSPRODUCER_H
