//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     eegosportsproducer.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Contains the declaration of the EEGoSportsProducer class.
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
