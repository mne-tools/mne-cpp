//=============================================================================================================
/**
* @file		ecgproducer.h
* @author	Christoph Dinh <christoph.dinh@live.de>;
* @version	1.0
* @date		October, 2010
*
* @section	LICENSE
*
* Copyright (C) 2010 Christoph Dinh. All rights reserved.
*
* No part of this program may be photocopied, reproduced,
* or translated to another program language without the
* prior written consent of the author.
*
*
* @brief	Contains the declaration of the ECGProducer class.
*
*/

#ifndef ECGPRODUCER_H
#define ECGPRODUCER_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../comp/rtmeas/IOBuffer/circularbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ECGSimulatorModule
//=============================================================================================================

namespace ECGSimulatorModule
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ECGSimulator;


//=============================================================================================================
/**
* DECLARE CLASS ECGProducer
*
* @brief The ECGProducer class provides a ECG data producer for a given sampling rate.
*/
class ECGProducer : public QThread
{
public:

    //=========================================================================================================
    /**
    * Constructs a ECGProducer.
    *
    * @param [in] simulator a pointer to the corresponding ECGSimulator.
    * @param [in] buffer_I a pointer to the buffer to which the ECGProducer should write the generated data for ECG I.
    * @param [in] buffer_II a pointer to the buffer to which the ECGProducer should write the generated data for ECG II.
    * @param [in] buffer_III a pointer to the buffer to which the ECGProducer should write the generated data for ECG III.
    */
    ECGProducer(ECGSimulator* simulator, ECGBuffer* buffer_I, ECGBuffer* buffer_II, ECGBuffer* buffer_III);

    //=========================================================================================================
    /**
    * Destroys the ECGProducer.
    */
    ~ECGProducer();

    //=========================================================================================================
    /**
    * Stops the ECGProducer by stopping the producer's thread.
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
    ECGSimulator*           m_pECGSimulator;	/**< Holds a pointer to corresponding ECGSimulator.*/
    ECGBuffer*              m_pdBuffer_I;		/**< Holds a pointer to the buffer where the simulated data of ECG I should be written to.*/
    ECGBuffer*              m_pdBuffer_II;		/**< Holds a pointer to the buffer where the simulated data of ECG II should be written to.*/
    ECGBuffer*              m_pdBuffer_III;		/**< Holds a pointer to the buffer where the simulated data of ECG III should be written to.*/
    bool                    m_bIsRunning;		/**< Holds whether ECGProducer is running.*/
};

} // NAMESPACE

#endif // ECGPRODUCER_H
